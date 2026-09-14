#ifndef BUF_H
#define BUF_H

#include <stdarg.h>
#include <stddef.h>

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} Buf;

void buf_init(Buf *b);
void buf_free(Buf *b);
void buf_clear(Buf *b);
void buf_reserve(Buf *b, size_t extra);
void buf_putc(Buf *b, char c);
void buf_putn(Buf *b, const char *s, size_t n);
void buf_puts(Buf *b, const char *s);
void buf_printf(Buf *b, const char *fmt, ...);
void buf_vprintf(Buf *b, const char *fmt, va_list ap);
char *buf_copy(const Buf *b);

#endif
