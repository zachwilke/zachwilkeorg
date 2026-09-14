#ifndef MARKDOWN_H
#define MARKDOWN_H

typedef struct {
  char *title;
  char *date;
  char *summary;
  char *body;
} FrontMatter;

int parse_front_matter(const char *src, FrontMatter *out);
void front_matter_free(FrontMatter *fm);

char *render_markdown(const char *src);
char *render_article(const char *src);
char *article_toc(const char *article_html);

#endif
