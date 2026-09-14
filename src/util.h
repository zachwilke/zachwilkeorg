#ifndef UTIL_H
#define UTIL_H

#include "buf.h"

#include <stddef.h>

char *xstrdup(const char *s);
char *path_join(const char *root, const char *rel);
char *read_file(const char *path);
void write_trimmed(const char *path, const char *contents);
void mkdir_p(const char *path);

void html_escape(Buf *out, const char *s);
void json_escape(Buf *out, const char *s);
void json_escape_schema(Buf *out, const char *s);
void uri_encode(Buf *out, const char *s);

const char *trim_bounds(const char *s, size_t *len);
int word_count(const char *s);
int valid_slug(const char *s);
int valid_date(const char *s);
void format_display_date(char *out, size_t out_sz, const char *iso);
void format_rfc822(char *out, size_t out_sz, const char *iso);

int parse_slug_index(const char *json, char ***slugs, size_t *count);

#endif
