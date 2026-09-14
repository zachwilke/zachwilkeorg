#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir((path), 0755)
#endif

char *xstrdup(const char *s) {
  if (!s) s = "";
  size_t n = strlen(s);
  char *out = malloc(n + 1);
  if (!out) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  memcpy(out, s, n + 1);
  return out;
}

char *path_join(const char *root, const char *rel) {
  size_t a = strlen(root);
  size_t b = strlen(rel);
  int slash = a > 0 && root[a - 1] != '/';
  char *out = malloc(a + b + (slash ? 2 : 1));
  if (!out) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  memcpy(out, root, a);
  if (slash) out[a++] = '/';
  memcpy(out + a, rel, b + 1);
  return out;
}

char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "cannot read %s: %s\n", path, strerror(errno));
    exit(1);
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "cannot seek %s\n", path);
    exit(1);
  }
  long sz = ftell(f);
  if (sz < 0) {
    fprintf(stderr, "cannot size %s\n", path);
    exit(1);
  }
  rewind(f);
  char *buf = malloc((size_t)sz + 1);
  if (!buf) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  size_t n = fread(buf, 1, (size_t)sz, f);
  fclose(f);
  buf[n] = '\0';
  return buf;
}

static int is_trim_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
         c == '\f';
}

const char *trim_bounds(const char *s, size_t *len) {
  if (!s) {
    *len = 0;
    return "";
  }
  while (*s && is_trim_space((unsigned char)*s)) s++;
  size_t n = strlen(s);
  while (n && is_trim_space((unsigned char)s[n - 1])) n--;
  *len = n;
  return s;
}

void mkdir_p(const char *path) {
  char *tmp = xstrdup(path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (MKDIR(tmp) != 0 && errno != EEXIST) {
        fprintf(stderr, "cannot mkdir %s: %s\n", tmp, strerror(errno));
        exit(1);
      }
      *p = '/';
    }
  }
  if (MKDIR(tmp) != 0 && errno != EEXIST) {
    fprintf(stderr, "cannot mkdir %s: %s\n", tmp, strerror(errno));
    exit(1);
  }
  free(tmp);
}

static void mkdir_parent(const char *path) {
  const char *slash = strrchr(path, '/');
  if (!slash || slash == path) return;
  char *dir = xstrdup(path);
  dir[slash - path] = '\0';
  mkdir_p(dir);
  free(dir);
}

void write_trimmed(const char *path, const char *contents) {
  size_t n = 0;
  const char *start = trim_bounds(contents, &n);
  mkdir_parent(path);
  FILE *f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "cannot write %s: %s\n", path, strerror(errno));
    exit(1);
  }
  if (n && fwrite(start, 1, n, f) != n) {
    fprintf(stderr, "short write: %s\n", path);
    exit(1);
  }
  if (fputc('\n', f) == EOF) {
    fprintf(stderr, "cannot finish %s\n", path);
    exit(1);
  }
  fclose(f);
}

void html_escape(Buf *out, const char *s) {
  if (!s) return;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    switch (*p) {
      case '&':
        buf_puts(out, "&amp;");
        break;
      case '<':
        buf_puts(out, "&lt;");
        break;
      case '>':
        buf_puts(out, "&gt;");
        break;
      case '"':
        buf_puts(out, "&quot;");
        break;
      case '\'':
        buf_puts(out, "&#39;");
        break;
      default:
        buf_putc(out, (char)*p);
        break;
    }
  }
}

static void json_escape_impl(Buf *out, const char *s, int schema) {
  if (!s) return;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    unsigned char c = *p;
    if (schema && c == '<') {
      buf_puts(out, "\\u003c");
      continue;
    }
    switch (c) {
      case '"':
        buf_puts(out, "\\\"");
        break;
      case '\\':
        buf_puts(out, "\\\\");
        break;
      case '\b':
        buf_puts(out, "\\b");
        break;
      case '\f':
        buf_puts(out, "\\f");
        break;
      case '\n':
        buf_puts(out, "\\n");
        break;
      case '\r':
        buf_puts(out, "\\r");
        break;
      case '\t':
        buf_puts(out, "\\t");
        break;
      default:
        if (c < 0x20) {
          buf_printf(out, "\\u%04x", c);
        } else {
          buf_putc(out, (char)c);
        }
        break;
    }
  }
}

void json_escape(Buf *out, const char *s) { json_escape_impl(out, s, 0); }

void json_escape_schema(Buf *out, const char *s) { json_escape_impl(out, s, 1); }

void uri_encode(Buf *out, const char *s) {
  static const char *unreserved =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!~*'()";
  if (!s) return;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if (strchr(unreserved, (char)*p)) {
      buf_putc(out, (char)*p);
    } else {
      buf_printf(out, "%%%02X", *p);
    }
  }
}

int word_count(const char *s) {
  size_t n = 0;
  const char *t = trim_bounds(s, &n);
  if (!n) return 0;
  int words = 0;
  int in_word = 0;
  for (size_t i = 0; i < n; i++) {
    if (is_trim_space((unsigned char)t[i])) {
      in_word = 0;
    } else if (!in_word) {
      in_word = 1;
      words++;
    }
  }
  return words;
}

static int is_slug_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

int valid_slug(const char *s) {
  if (!s || !*s || !is_slug_char(*s)) return 0;
  const char *p = s;
  while (*p) {
    if (is_slug_char(*p)) {
      p++;
      continue;
    }
    if (*p == '-' && p[1] && is_slug_char(p[1])) {
      p++;
      continue;
    }
    return 0;
  }
  return 1;
}

static int is_leap(int y) {
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(int y, int m) {
  static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m == 2) return days[2] + is_leap(y);
  if (m < 1 || m > 12) return 0;
  return days[m];
}

int valid_date(const char *s) {
  if (!s || strlen(s) != 10) return 0;
  for (int i = 0; i < 10; i++) {
    if (i == 4 || i == 7) {
      if (s[i] != '-') return 0;
    } else if (!isdigit((unsigned char)s[i])) {
      return 0;
    }
  }
  int y = (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 +
          (s[3] - '0');
  int m = (s[5] - '0') * 10 + (s[6] - '0');
  int d = (s[8] - '0') * 10 + (s[9] - '0');
  int dim = days_in_month(y, m);
  return dim && d >= 1 && d <= dim;
}

static const char *MONTHS_SHORT[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void format_display_date(char *out, size_t out_sz, const char *iso) {
  int m = (iso[5] - '0') * 10 + (iso[6] - '0');
  snprintf(out, out_sz, "%s %c%c, %c%c%c%c", MONTHS_SHORT[m - 1], iso[8],
           iso[9], iso[0], iso[1], iso[2], iso[3]);
}

/* 0 = Sunday. Sakamoto's method. */
static int weekday(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

void format_rfc822(char *out, size_t out_sz, const char *iso) {
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  int y = (iso[0] - '0') * 1000 + (iso[1] - '0') * 100 + (iso[2] - '0') * 10 +
          (iso[3] - '0');
  int m = (iso[5] - '0') * 10 + (iso[6] - '0');
  int d = (iso[8] - '0') * 10 + (iso[9] - '0');
  snprintf(out, out_sz, "%s, %02d %s %04d 12:00:00 GMT", days[weekday(y, m, d)],
           d, MONTHS_SHORT[m - 1], y);
}

static void skip_ws(const char **p) {
  while (**p && is_trim_space((unsigned char)**p)) (*p)++;
}

static char *parse_json_string(const char **p) {
  if (**p != '"') return NULL;
  (*p)++;
  Buf b;
  buf_init(&b);
  while (**p && **p != '"') {
    if (**p == '\\') {
      (*p)++;
      char c = **p;
      if (!c) {
        buf_free(&b);
        return NULL;
      }
      switch (c) {
        case '"':
        case '\\':
        case '/':
          buf_putc(&b, c);
          break;
        case 'b':
          buf_putc(&b, '\b');
          break;
        case 'f':
          buf_putc(&b, '\f');
          break;
        case 'n':
          buf_putc(&b, '\n');
          break;
        case 'r':
          buf_putc(&b, '\r');
          break;
        case 't':
          buf_putc(&b, '\t');
          break;
        default:
          buf_putc(&b, c);
          break;
      }
      (*p)++;
    } else {
      buf_putc(&b, **p);
      (*p)++;
    }
  }
  if (**p != '"') {
    buf_free(&b);
    return NULL;
  }
  (*p)++;
  char *out = buf_copy(&b);
  buf_free(&b);
  return out;
}

int parse_slug_index(const char *json, char ***slugs, size_t *count) {
  const char *p = json;
  skip_ws(&p);
  if (*p != '[') return 0;
  p++;
  char **list = NULL;
  size_t n = 0;
  size_t cap = 0;
  skip_ws(&p);
  if (*p == ']') {
    p++;
    skip_ws(&p);
    if (*p != '\0') {
      free(list);
      return 0;
    }
    *slugs = list;
    *count = 0;
    return 1;
  }
  while (*p) {
    skip_ws(&p);
    char *s = parse_json_string(&p);
    if (!s) {
      for (size_t i = 0; i < n; i++) free(list[i]);
      free(list);
      return 0;
    }
    if (n == cap) {
      cap = cap ? cap * 2 : 8;
      char **next = realloc(list, cap * sizeof(*list));
      if (!next) {
        fprintf(stderr, "out of memory\n");
        exit(1);
      }
      list = next;
    }
    list[n++] = s;
    skip_ws(&p);
    if (*p == ',') {
      p++;
      continue;
    }
    if (*p == ']') {
      p++;
      skip_ws(&p);
      if (*p != '\0') {
        for (size_t i = 0; i < n; i++) free(list[i]);
        free(list);
        return 0;
      }
      *slugs = list;
      *count = n;
      return 1;
    }
    for (size_t i = 0; i < n; i++) free(list[i]);
    free(list);
    return 0;
  }
  for (size_t i = 0; i < n; i++) free(list[i]);
  free(list);
  return 0;
}
