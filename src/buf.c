#include "buf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void buf_init(Buf *b) {
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

void buf_free(Buf *b) {
  free(b->data);
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

void buf_clear(Buf *b) { b->len = 0; }

void buf_reserve(Buf *b, size_t extra) {
  size_t need = b->len + extra + 1;
  if (need <= b->cap) return;
  size_t cap = b->cap ? b->cap : 64;
  while (cap < need) {
    if (cap > (size_t)-1 / 2) {
      cap = need;
      break;
    }
    cap *= 2;
  }
  char *data = realloc(b->data, cap);
  if (!data) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  b->data = data;
  b->cap = cap;
}

void buf_putc(Buf *b, char c) {
  buf_reserve(b, 1);
  b->data[b->len++] = c;
  b->data[b->len] = '\0';
}

void buf_putn(Buf *b, const char *s, size_t n) {
  if (!s || !n) return;
  buf_reserve(b, n);
  memcpy(b->data + b->len, s, n);
  b->len += n;
  b->data[b->len] = '\0';
}

void buf_puts(Buf *b, const char *s) {
  if (!s) return;
  buf_putn(b, s, strlen(s));
}

void buf_vprintf(Buf *b, const char *fmt, va_list ap) {
  va_list copy;
  va_copy(copy, ap);
  int n = vsnprintf(NULL, 0, fmt, copy);
  va_end(copy);
  if (n < 0) return;
  buf_reserve(b, (size_t)n);
  vsnprintf(b->data + b->len, (size_t)n + 1, fmt, ap);
  b->len += (size_t)n;
}

void buf_printf(Buf *b, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  buf_vprintf(b, fmt, ap);
  va_end(ap);
}

char *buf_copy(const Buf *b) {
  char *out = malloc(b->len + 1);
  if (!out) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  if (b->data && b->len) memcpy(out, b->data, b->len);
  out[b->len] = '\0';
  return out;
}
