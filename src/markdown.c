#include "markdown.h"

#include "buf.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void md_html_escape(Buf *out, const char *s, size_t n) {
  for (size_t i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    switch (c) {
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
      default:
        buf_putc(out, (char)c);
        break;
    }
  }
}

static int starts_with_ci(const char *s, const char *prefix) {
  while (*prefix) {
    if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
    s++;
    prefix++;
  }
  return 1;
}

static int safe_url(const char *href, size_t n, Buf *out) {
  while (n && (href[0] == ' ' || href[0] == '\t')) {
    href++;
    n--;
  }
  while (n && (href[n - 1] == ' ' || href[n - 1] == '\t')) n--;
  if (!n) return 0;
  if (starts_with_ci(href, "https:") || starts_with_ci(href, "http:") ||
      starts_with_ci(href, "mailto:") || href[0] == '/' || href[0] == '#') {
    buf_putn(out, href, n);
    return 1;
  }
  return 0;
}

static int is_http_url(const char *href) {
  return starts_with_ci(href, "https:") || starts_with_ci(href, "http:");
}

static void replace_strong(Buf *out, const char *s, size_t n) {
  size_t i = 0;
  while (i < n) {
    if (i + 1 < n && s[i] == '*' && s[i + 1] == '*') {
      size_t j = i + 2;
      while (j < n && s[j] != '*') j++;
      if (j + 1 < n && s[j] == '*' && s[j + 1] == '*' && j > i + 2) {
        buf_puts(out, "<strong>");
        buf_putn(out, s + i + 2, j - (i + 2));
        buf_puts(out, "</strong>");
        i = j + 2;
        continue;
      }
    }
    buf_putc(out, s[i]);
    i++;
  }
}

static void replace_em(Buf *out, const char *s, size_t n) {
  size_t i = 0;
  while (i < n) {
    if (s[i] == '*') {
      size_t j = i + 1;
      while (j < n && s[j] != '*') j++;
      if (j < n && j > i + 1) {
        buf_puts(out, "<em>");
        buf_putn(out, s + i + 1, j - (i + 1));
        buf_puts(out, "</em>");
        i = j + 1;
        continue;
      }
    }
    buf_putc(out, s[i]);
    i++;
  }
}

static void emph(Buf *out, const char *s, size_t n) {
  Buf strong;
  buf_init(&strong);
  replace_strong(&strong, s, n);
  replace_em(out, strong.data ? strong.data : "", strong.len);
  buf_free(&strong);
}

static void inline_md(Buf *out, const char *raw) {
  size_t n = strlen(raw);
  size_t i = 0;
  Buf escaped;
  buf_init(&escaped);
  while (i < n) {
    int found = 0;
    size_t at = 0;
    enum { KIND_CODE, KIND_IMG, KIND_LINK } kind = KIND_CODE;
    const char *g1 = NULL, *g2 = NULL, *g3 = NULL, *g4 = NULL, *g5 = NULL;
    size_t n1 = 0, n2 = 0, n3 = 0, n4 = 0, n5 = 0;
    size_t match_end = 0;

    for (size_t p = i; p < n; p++) {
      if (raw[p] == '`') {
        size_t q = p + 1;
        while (q < n && raw[q] != '`') q++;
        if (q < n && q > p + 1) {
          found = 1;
          kind = KIND_CODE;
          at = p;
          g1 = raw + p + 1;
          n1 = q - (p + 1);
          match_end = q + 1;
          break;
        }
      }
      if (raw[p] == '!' && p + 1 < n && raw[p + 1] == '[') {
        size_t q = p + 2;
        while (q < n && raw[q] != ']') q++;
        if (q < n && q + 1 < n && raw[q + 1] == '(') {
          size_t r = q + 2;
          while (r < n && raw[r] != ')') r++;
          if (r < n && r > q + 2) {
            found = 1;
            kind = KIND_IMG;
            at = p;
            g2 = raw + p + 2;
            n2 = q - (p + 2);
            g3 = raw + q + 2;
            n3 = r - (q + 2);
            match_end = r + 1;
            break;
          }
        }
      }
      if (raw[p] == '[') {
        size_t q = p + 1;
        while (q < n && raw[q] != ']') q++;
        if (q < n && q > p + 1 && q + 1 < n && raw[q + 1] == '(') {
          size_t r = q + 2;
          while (r < n && raw[r] != ')') r++;
          if (r < n && r > q + 2) {
            found = 1;
            kind = KIND_LINK;
            at = p;
            g4 = raw + p + 1;
            n4 = q - (p + 1);
            g5 = raw + q + 2;
            n5 = r - (q + 2);
            match_end = r + 1;
            break;
          }
        }
      }
    }

    if (!found) {
      buf_clear(&escaped);
      md_html_escape(&escaped, raw + i, n - i);
      emph(out, escaped.data ? escaped.data : "", escaped.len);
      break;
    }

    buf_clear(&escaped);
    md_html_escape(&escaped, raw + i, at - i);
    emph(out, escaped.data ? escaped.data : "", escaped.len);

    if (kind == KIND_CODE) {
      buf_puts(out, "<code>");
      md_html_escape(out, g1, n1);
      buf_puts(out, "</code>");
    } else if (kind == KIND_IMG) {
      Buf url;
      buf_init(&url);
      if (safe_url(g3, n3, &url)) {
        buf_puts(out, "<img src=\"");
        md_html_escape(out, url.data, url.len);
        buf_puts(out, "\" alt=\"");
        md_html_escape(out, g2, n2);
        buf_puts(out, "\">");
      } else {
        md_html_escape(out, g2, n2);
      }
      buf_free(&url);
    } else {
      Buf url;
      buf_init(&url);
      buf_clear(&escaped);
      md_html_escape(&escaped, g4, n4);
      if (!safe_url(g5, n5, &url)) {
        emph(out, escaped.data ? escaped.data : "", escaped.len);
      } else {
        buf_puts(out, "<a href=\"");
        md_html_escape(out, url.data, url.len);
        buf_puts(out, "\"");
        if (is_http_url(url.data)) buf_puts(out, " rel=\"noopener noreferrer\"");
        buf_puts(out, ">");
        emph(out, escaped.data ? escaped.data : "", escaped.len);
        buf_puts(out, "</a>");
      }
      buf_free(&url);
    }
    i = match_end;
  }
  buf_free(&escaped);
}

static void strip_crlf(char *s) {
  char *r = s, *w = s;
  while (*r) {
    if (r[0] == '\r' && r[1] == '\n') {
      *w++ = '\n';
      r += 2;
    } else if (*r == '\r') {
      *w++ = '\n';
      r++;
    } else {
      *w++ = *r++;
    }
  }
  *w = '\0';
}

static void rtrim_newlines(char *s) {
  size_t n = strlen(s);
  while (n && s[n - 1] == '\n') s[--n] = '\0';
}

static int is_ws_line(const char *s) {
  while (*s) {
    if (*s != ' ' && *s != '\t' && *s != '\r') return 0;
    s++;
  }
  return 1;
}

static int match_fence(const char *line, const char **lang) {
  if (line[0] != '`' || line[1] != '`' || line[2] != '`') return 0;
  *lang = line + 3;
  return 1;
}

static int match_hr(const char *line) {
  while (*line == ' ' || *line == '\t') line++;
  char c = *line;
  if (c != '*' && c != '_' && c != '-') return 0;
  int n = 0;
  while (*line == c) {
    n++;
    line++;
  }
  while (*line == ' ' || *line == '\t') line++;
  return n >= 3 && *line == '\0';
}

static int match_heading(const char *line, int *level, const char **text,
                         size_t *text_len) {
  int n = 0;
  while (n < 6 && line[n] == '#') n++;
  if (n == 0 || (line[n] != ' ' && line[n] != '\t')) return 0;
  const char *start = line + n;
  while (*start == ' ' || *start == '\t') start++;
  if (!*start) return 0;
  const char *end = start + strlen(start);
  while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;
  while (end > start && end[-1] == '#') end--;
  while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;
  if (end <= start) return 0;
  *level = n;
  *text = start;
  *text_len = (size_t)(end - start);
  return 1;
}

static const char *quote_body(const char *line) {
  if (line[0] != '>') return line;
  if (line[1] == ' ' || line[1] == '\t') return line + 2;
  return line + 1;
}

static const char *skip_ws_chars(const char *s) {
  while (*s == ' ' || *s == '\t') s++;
  return s;
}

static int match_ul(const char *line) {
  const char *s = skip_ws_chars(line);
  return (*s == '-' || *s == '*' || *s == '+') &&
         (s[1] == ' ' || s[1] == '\t');
}

static const char *ul_body(const char *line) {
  const char *s = skip_ws_chars(line);
  if ((*s == '-' || *s == '*' || *s == '+') && (s[1] == ' ' || s[1] == '\t'))
    return skip_ws_chars(s + 2);
  return line;
}

static int match_ol(const char *line) {
  const char *s = skip_ws_chars(line);
  if (!isdigit((unsigned char)*s)) return 0;
  while (isdigit((unsigned char)*s)) s++;
  return *s == '.' && (s[1] == ' ' || s[1] == '\t');
}

static const char *ol_body(const char *line) {
  const char *s = skip_ws_chars(line);
  while (isdigit((unsigned char)*s)) s++;
  if (*s == '.' && (s[1] == ' ' || s[1] == '\t')) return skip_ws_chars(s + 2);
  return line;
}

static char **split_lines(char *src, size_t *count) {
  size_t n = 1;
  for (char *p = src; *p; p++) {
    if (*p == '\n') n++;
  }
  char **lines = malloc(n * sizeof(*lines));
  if (!lines) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  size_t i = 0;
  lines[i++] = src;
  for (char *p = src; *p; p++) {
    if (*p == '\n') {
      *p = '\0';
      lines[i++] = p + 1;
    }
  }
  *count = i;
  return lines;
}

char *render_markdown(const char *src) {
  char *copy = xstrdup(src ? src : "");
  strip_crlf(copy);
  rtrim_newlines(copy);
  size_t nlines = 0;
  char **lines = split_lines(copy, &nlines);

  Buf out;
  buf_init(&out);
  Buf para;
  buf_init(&para);
  size_t i = 0;

  while (i < nlines) {
    const char *line = lines[i];
    const char *lang = NULL;
    if (match_fence(line, &lang)) {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      while (*lang == ' ' || *lang == '\t') lang++;
      const char *lang_end = lang + strlen(lang);
      while (lang_end > lang &&
             (lang_end[-1] == ' ' || lang_end[-1] == '\t'))
        lang_end--;
      Buf code;
      buf_init(&code);
      i++;
      while (i < nlines &&
             !(lines[i][0] == '`' && lines[i][1] == '`' && lines[i][2] == '`')) {
        if (code.len) buf_putc(&code, '\n');
        buf_puts(&code, lines[i]);
        i++;
      }
      if (i < nlines) i++;
      buf_puts(&out, "<pre><code");
      if (lang_end > lang) {
        buf_puts(&out, " class=\"language-");
        md_html_escape(&out, lang, (size_t)(lang_end - lang));
        buf_puts(&out, "\"");
      }
      buf_puts(&out, ">");
      md_html_escape(&out, code.data ? code.data : "", code.len);
      buf_puts(&out, "</code></pre>\n");
      buf_free(&code);
      continue;
    }

    if (match_hr(line)) {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      buf_puts(&out, "<hr>\n");
      i++;
      continue;
    }

    int level = 0;
    const char *htext = NULL;
    size_t hlen = 0;
    if (match_heading(line, &level, &htext, &hlen)) {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      char *ht = malloc(hlen + 1);
      if (!ht) {
        fprintf(stderr, "out of memory\n");
        exit(1);
      }
      memcpy(ht, htext, hlen);
      ht[hlen] = '\0';
      buf_printf(&out, "<h%d>", level);
      inline_md(&out, ht);
      buf_printf(&out, "</h%d>\n", level);
      free(ht);
      i++;
      continue;
    }

    if (line[0] == '>') {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      Buf quoted;
      buf_init(&quoted);
      while (i < nlines && lines[i][0] == '>') {
        if (quoted.len) buf_putc(&quoted, '\n');
        buf_puts(&quoted, quote_body(lines[i]));
        i++;
      }
      char *inner = render_markdown(quoted.data ? quoted.data : "");
      buf_puts(&out, "<blockquote>");
      buf_puts(&out, inner);
      buf_puts(&out, "</blockquote>\n");
      free(inner);
      buf_free(&quoted);
      continue;
    }

    if (match_ul(line)) {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      buf_puts(&out, "<ul>\n");
      while (i < nlines && match_ul(lines[i])) {
        buf_puts(&out, "<li>");
        inline_md(&out, ul_body(lines[i]));
        buf_puts(&out, "</li>\n");
        i++;
      }
      buf_puts(&out, "</ul>\n");
      continue;
    }

    if (match_ol(line)) {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      buf_puts(&out, "<ol>\n");
      while (i < nlines && match_ol(lines[i])) {
        buf_puts(&out, "<li>");
        inline_md(&out, ol_body(lines[i]));
        buf_puts(&out, "</li>\n");
        i++;
      }
      buf_puts(&out, "</ol>\n");
      continue;
    }

    if (is_ws_line(line)) {
      if (para.len) {
        buf_puts(&out, "<p>");
        inline_md(&out, para.data);
        buf_puts(&out, "</p>\n");
        buf_clear(&para);
      }
      i++;
      continue;
    }

    if (para.len) buf_putc(&para, '\n');
    buf_puts(&para, line);
    i++;
  }

  if (para.len) {
    buf_puts(&out, "<p>");
    inline_md(&out, para.data);
    buf_puts(&out, "</p>\n");
  }

  if (out.len && out.data[out.len - 1] == '\n') out.data[--out.len] = '\0';

  char *result = buf_copy(&out);
  buf_free(&out);
  buf_free(&para);
  free(lines);
  free(copy);
  return result;
}

static int parse_kv_line(const char *line, char **key, char **val) {
  const char *colon = strchr(line, ':');
  if (!colon) return 0;
  size_t klen = (size_t)(colon - line);
  while (klen && (line[klen - 1] == ' ' || line[klen - 1] == '\t')) klen--;
  size_t kstart = 0;
  while (kstart < klen && (line[kstart] == ' ' || line[kstart] == '\t'))
    kstart++;
  if (kstart >= klen) return 0;
  klen -= kstart;
  const char *v = colon + 1;
  while (*v == ' ' || *v == '\t') v++;
  size_t vlen = strlen(v);
  while (vlen && (v[vlen - 1] == ' ' || v[vlen - 1] == '\t')) vlen--;
  if (vlen >= 2 && ((v[0] == '"' && v[vlen - 1] == '"') ||
                    (v[0] == '\'' && v[vlen - 1] == '\''))) {
    v++;
    vlen -= 2;
  }
  *key = malloc(klen + 1);
  *val = malloc(vlen + 1);
  if (!*key || !*val) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  memcpy(*key, line + kstart, klen);
  (*key)[klen] = '\0';
  memcpy(*val, v, vlen);
  (*val)[vlen] = '\0';
  return 1;
}

int parse_front_matter(const char *src, FrontMatter *out) {
  memset(out, 0, sizeof(*out));
  const char *text = src ? src : "";
  if ((unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
      (unsigned char)text[2] == 0xBF) {
    text += 3;
  }

  const char *body = text;
  if (text[0] == '-' && text[1] == '-' && text[2] == '-') {
    const char *p = text + 3;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\r') p++;
    if (*p == '\n') {
      p++;
      const char *meta_start = p;
      const char *q = p;
      while (*q) {
        if (q[0] == '\n' && q[1] == '-' && q[2] == '-' && q[3] == '-') {
          const char *r = q + 4;
          while (*r == ' ' || *r == '\t') r++;
          if (*r == '\0' || *r == '\n' ||
              (*r == '\r' && (r[1] == '\0' || r[1] == '\n'))) {
            const char *meta_end = q;
            if (meta_end > meta_start && meta_end[-1] == '\r') meta_end--;
            if (*r == '\r') r++;
            if (*r == '\n') r++;
            body = r;
            char *meta = malloc((size_t)(meta_end - meta_start) + 1);
            if (!meta) {
              fprintf(stderr, "out of memory\n");
              exit(1);
            }
            memcpy(meta, meta_start, (size_t)(meta_end - meta_start));
            meta[meta_end - meta_start] = '\0';
            char *save = meta;
            char *line = meta;
            while (*line) {
              char *nl = line;
              while (*nl && *nl != '\n' && *nl != '\r') nl++;
              char saved = *nl;
              *nl = '\0';
              char *key = NULL, *val = NULL;
              if (parse_kv_line(line, &key, &val)) {
                if (strcmp(key, "title") == 0) {
                  free(out->title);
                  out->title = val;
                  val = NULL;
                } else if (strcmp(key, "date") == 0) {
                  free(out->date);
                  out->date = val;
                  val = NULL;
                } else if (strcmp(key, "summary") == 0) {
                  free(out->summary);
                  out->summary = val;
                  val = NULL;
                }
                free(key);
                free(val);
              }
              if (!saved) break;
              line = nl + 1;
              if (saved == '\r' && *line == '\n') line++;
            }
            free(save);
            break;
          }
        }
        q++;
      }
    }
  }

  out->body = xstrdup(body);
  if (!out->title) out->title = xstrdup("");
  if (!out->date) out->date = xstrdup("");
  if (!out->summary) out->summary = xstrdup("");
  return 1;
}

void front_matter_free(FrontMatter *fm) {
  if (!fm) return;
  free(fm->title);
  free(fm->date);
  free(fm->summary);
  free(fm->body);
  memset(fm, 0, sizeof(*fm));
}

static int is_heading_open(const char *s, int *level) {
  if (s[0] != '<' || s[1] != 'h' || s[2] < '1' || s[2] > '6' || s[3] != '>')
    return 0;
  *level = s[2] - '0';
  return 1;
}

char *render_article(const char *src) {
  char *html = render_markdown(src);
  Buf out;
  buf_init(&out);
  const char *p = html;
  int index = 0;
  while (*p) {
    int level = 0;
    if (is_heading_open(p, &level)) {
      char close[6] = {'<', '/', 'h', (char)('0' + level), '>', 0};
      const char *start = p + 4;
      const char *end = strstr(start, close);
      if (!end) {
        buf_putc(&out, *p);
        p++;
        continue;
      }
      index++;
      buf_printf(&out, "<h%d id=\"section-%d\">", level, index);
      buf_putn(&out, start, (size_t)(end - start));
      buf_printf(&out,
                 " <a class=\"heading-anchor\" href=\"#section-%d\" "
                 "aria-label=\"Link to section %d\">#</a></h%d>",
                 index, index, level);
      p = end + 5;
      continue;
    }
    buf_putc(&out, *p);
    p++;
  }
  char *result = buf_copy(&out);
  buf_free(&out);
  free(html);
  return result;
}

static void strip_tags(Buf *out, const char *s, size_t n) {
  int in_tag = 0;
  for (size_t i = 0; i < n; i++) {
    if (s[i] == '<') {
      in_tag = 1;
      continue;
    }
    if (s[i] == '>') {
      in_tag = 0;
      continue;
    }
    if (!in_tag) buf_putc(out, s[i]);
  }
}

char *article_toc(const char *article_html) {
  typedef struct {
    char id[32];
    char *text;
  } Heading;
  Heading heads[128];
  size_t n = 0;
  const char *p = article_html ? article_html : "";
  while (*p && n < 128) {
    const char *open = strstr(p, "<h");
    if (!open) break;
    if (open[2] < '1' || open[2] > '6') {
      p = open + 2;
      continue;
    }
    const char *id_attr = strstr(open, " id=\"");
    if (!id_attr || id_attr > open + 16) {
      p = open + 2;
      continue;
    }
    id_attr += 5;
    const char *id_end = strchr(id_attr, '"');
    if (!id_end) break;
    const char *text_start = strchr(id_end, '>');
    if (!text_start) break;
    text_start++;
    const char *anchor = strstr(text_start, " <a class=\"heading-anchor\"");
    if (!anchor) {
      p = text_start;
      continue;
    }
    size_t id_len = (size_t)(id_end - id_attr);
    if (id_len >= sizeof(heads[n].id)) id_len = sizeof(heads[n].id) - 1;
    memcpy(heads[n].id, id_attr, id_len);
    heads[n].id[id_len] = '\0';
    Buf text;
    buf_init(&text);
    strip_tags(&text, text_start, (size_t)(anchor - text_start));
    heads[n].text = buf_copy(&text);
    buf_free(&text);
    n++;
    p = anchor;
  }

  Buf out;
  buf_init(&out);
  if (n > 1) {
    buf_puts(&out, "<details class=\"toc\"><summary>On this page</summary><ul>");
    for (size_t i = 0; i < n; i++) {
      buf_puts(&out, "<li><a href=\"#");
      buf_puts(&out, heads[i].id);
      buf_puts(&out, "\">");
      buf_puts(&out, heads[i].text);
      buf_puts(&out, "</a></li>");
    }
    buf_puts(&out, "</ul></details>");
  }
  for (size_t i = 0; i < n; i++) free(heads[i].text);
  char *result = buf_copy(&out);
  buf_free(&out);
  return result;
}
