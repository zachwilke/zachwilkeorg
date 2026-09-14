#include "markdown.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    failures++;
  }
}

static void expect_str(const char *got, const char *want, const char *msg) {
  if (!got) got = "(null)";
  if (strcmp(got, want) != 0) {
    fprintf(stderr, "FAIL: %s\n  got:  [%s]\n  want: [%s]\n", msg, got, want);
    failures++;
  }
}

int main(void) {
  FrontMatter fm;
  parse_front_matter(
      "---\ntitle: Hello\ndate: 2026-09-01\nsummary: A note.\n---\n\nBody *here*.\n",
      &fm);
  expect_str(fm.title, "Hello", "front matter title");
  expect_str(fm.date, "2026-09-01", "front matter date");
  expect_str(fm.summary, "A note.", "front matter summary");
  expect_str(fm.body, "\nBody *here*.\n", "front matter body");
  front_matter_free(&fm);

  char *html = render_markdown("Hello *world* and **bold**.");
  expect_str(html, "<p>Hello <em>world</em> and <strong>bold</strong>.</p>",
             "emphasis");
  free(html);

  html = render_markdown("See [X](https://x.com/zachwilke_1).");
  expect_str(html,
             "<p>See <a href=\"https://x.com/zachwilke_1\" rel=\"noopener "
             "noreferrer\">X</a>.</p>",
             "external link");
  free(html);

  html = render_markdown("# Title\n\nParagraph.\n");
  expect_str(html, "<h1>Title</h1>\n<p>Paragraph.</p>", "heading and paragraph");
  free(html);

  html = render_markdown("- one\n- two");
  expect_str(html, "<ul>\n<li>one</li>\n<li>two</li>\n</ul>", "unordered list");
  free(html);

  html = render_markdown("1. one\n2. two");
  expect_str(html, "<ol>\n<li>one</li>\n<li>two</li>\n</ol>", "ordered list");
  free(html);

  html = render_markdown("```c\nint x = 1;\n```");
  expect_str(html, "<pre><code class=\"language-c\">int x = 1;</code></pre>",
             "fenced code");
  free(html);

  html = render_markdown("before\n\n---\n\nafter");
  expect_str(html, "<p>before</p>\n<hr>\n<p>after</p>", "thematic break");
  free(html);

  html = render_markdown("> quoted *text*");
  expect_str(html, "<blockquote><p>quoted <em>text</em></p></blockquote>",
             "blockquote");
  free(html);

  html = render_article("## First\n\n## Second\n");
  char *toc = article_toc(html);
  expect(strstr(html, "id=\"section-1\"") != NULL, "article heading id");
  expect(strstr(toc, "On this page") != NULL, "toc for multiple headings");
  free(html);
  free(toc);

  expect(valid_slug("the-beginning"), "valid slug");
  expect(!valid_slug("Bad Slug"), "invalid slug");
  expect(valid_date("2026-09-01"), "valid date");
  expect(!valid_date("2026-02-29"), "non-leap Feb 29");
  expect(!valid_date("2026-13-01"), "invalid month");

  char display[32];
  format_display_date(display, sizeof(display), "2026-09-01");
  expect_str(display, "Sep 01, 2026", "display date");
  format_rfc822(display, sizeof(display), "2026-09-01");
  expect_str(display, "Tue, 01 Sep 2026 12:00:00 GMT", "rfc822 date");
  format_rfc822(display, sizeof(display), "2026-08-27");
  expect_str(display, "Thu, 27 Aug 2026 12:00:00 GMT", "rfc822 thursday");

  expect(word_count("one two three") == 3, "word count");

  char **slugs = NULL;
  size_t n = 0;
  expect(parse_slug_index("[\"one-week-on-omarcy-quattro\", \"the-beginning\"]",
                          &slugs, &n),
         "parse slug index");
  expect(n == 2, "slug count");
  if (n == 2) {
    expect_str(slugs[0], "one-week-on-omarcy-quattro", "first slug");
    expect_str(slugs[1], "the-beginning", "second slug");
  }
  for (size_t i = 0; i < n; i++) free(slugs[i]);
  free(slugs);

  if (failures) {
    fprintf(stderr, "%d test(s) failed\n", failures);
    return 1;
  }
  printf("All markdown and helper tests passed.\n");
  return 0;
}
