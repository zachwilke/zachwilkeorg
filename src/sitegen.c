#include "buf.h"
#include "markdown.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORIGIN "https://zachwilke.org"
#define WRITING_NOTE                                                       \
  "<p class=\"writing-note\">Everything on this blog is written by me, "   \
  "not AI. These are my personal thoughts as I learn and build in the "    \
  "age of AI.</p>"

typedef struct {
  char *slug;
  char *title;
  char *date;
  char *summary;
  char *body;
  char *path;
  int minutes;
} Post;

static int cmp_posts_desc(const void *a, const void *b) {
  const Post *pa = a;
  const Post *pb = b;
  return strcmp(pb->date, pa->date);
}

static char *replace_once(const char *src, const char *needle,
                          const char *repl) {
  const char *at = strstr(src, needle);
  if (!at) return xstrdup(src);
  size_t before = (size_t)(at - src);
  size_t needle_len = strlen(needle);
  size_t after = strlen(at + needle_len);
  size_t repl_len = strlen(repl);
  char *out = malloc(before + repl_len + after + 1);
  if (!out) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  memcpy(out, src, before);
  memcpy(out + before, repl, repl_len);
  memcpy(out + before + repl_len, at + needle_len, after + 1);
  return out;
}

static void post_row(Buf *out, const Post *post) {
  char display[32];
  format_display_date(display, sizeof(display), post->date);
  buf_puts(out, "<article class=\"post-row\" data-slug=\"");
  html_escape(out, post->slug);
  buf_puts(out, "\"><time datetime=\"");
  html_escape(out, post->date);
  buf_puts(out, "\">");
  html_escape(out, display);
  buf_puts(out, "</time><div><h3><a href=\"");
  html_escape(out, post->path);
  buf_puts(out, "\" class=\"post-row-link\">");
  html_escape(out, post->title);
  buf_puts(out, "</a></h3><p>");
  html_escape(out, post->summary);
  buf_puts(out, "</p><span class=\"reading-time\">");
  buf_printf(out, "%d min read", post->minutes);
  buf_puts(out, "</span></div></article>");
}

static void rings_html(Buf *out) {
  static const char *lenses[] = {"operations", "software", "life"};
  for (int i = 0; i < 14; i++) {
    double offset = (i - 6.5) * 17.0;
    buf_printf(out,
               "<div class=\"apparatus-ring\" data-lens=\"%s\" "
               "style=\"--ring:%d;--offset:%.1fpx\"></div>",
               lenses[i % 3], i, offset);
  }
}

static void person_schema(Buf *out) {
  buf_puts(out,
           "{\"@context\":\"https://schema.org\",\"@type\":\"Person\","
           "\"name\":\"Zach Wilke\",\"url\":\"" ORIGIN
           "\",\"email\":\"zach@pinefall.dev\","
           "\"jobTitle\":\"Director of Operations\",\"knowsAbout\":["
           "\"IT operations\",\"Software development\",\"Agentic AI\","
           "\"Linux\"],\"sameAs\":[\"https://github.com/zachwilke\","
           "\"https://x.com/zachwilke_1\",\"https://world.hey.com/zwilke\"]}");
}

static void post_schema(Buf *out, const Post *post) {
  buf_puts(out,
           "{\"@context\":\"https://schema.org\",\"@type\":\"BlogPosting\","
           "\"headline\":\"");
  json_escape_schema(out, post->title);
  buf_puts(out, "\",\"description\":\"");
  json_escape_schema(out, post->summary[0] ? post->summary : post->title);
  buf_puts(out, "\",\"datePublished\":\"");
  json_escape_schema(out, post->date);
  buf_puts(out,
           "\",\"author\":{\"@type\":\"Person\",\"name\":\"Zach Wilke\","
           "\"url\":\"" ORIGIN "\"},\"mainEntityOfPage\":\"" ORIGIN);
  json_escape_schema(out, post->path);
  buf_puts(out, "\"}");
}

static char *layout(const char *title, const char *description, const char *path,
                    const char *content, const char *page, const char *schema,
                    int year) {
  int blog = strcmp(page, "blog") == 0 || strcmp(page, "post") == 0;
  int error = strcmp(page, "error") == 0;
  int is_post = strcmp(page, "post") == 0;

  char *wired = replace_once(content, "<main id=\"main\"",
                             "<main id=\"main\" tabindex=\"-1\"");

  Buf out;
  buf_init(&out);
  buf_puts(&out, "<!doctype html>\n<html lang=\"en\">\n<head>\n");
  buf_puts(&out, "<meta charset=\"utf-8\">\n");
  buf_puts(&out,
           "<meta name=\"viewport\" content=\"width=device-width, "
           "initial-scale=1\">\n");
  buf_puts(&out, "<meta name=\"color-scheme\" content=\"light dark\">\n");
  buf_puts(&out, "<meta name=\"theme-color\" content=\"#faf9f6\">\n");
  buf_puts(&out, "<title>");
  html_escape(&out, title);
  buf_puts(&out, "</title>\n<meta name=\"description\" content=\"");
  html_escape(&out, description);
  buf_puts(&out, "\">\n");
  if (error) buf_puts(&out, "<meta name=\"robots\" content=\"noindex\">");
  buf_puts(&out, "\n<link rel=\"canonical\" href=\"" ORIGIN);
  html_escape(&out, path);
  buf_puts(&out, "\">\n");
  buf_puts(&out,
           "<link rel=\"icon\" href=\"/assets/favicon.svg\" "
           "type=\"image/svg+xml\">\n\n");
  buf_puts(&out, "<link rel=\"stylesheet\" href=\"/assets/site.css\">\n");
  buf_puts(&out,
           "<link rel=\"alternate\" type=\"application/rss+xml\" "
           "title=\"Zach Wilke — Field Notes\" href=\"/feed.xml\">\n");
  buf_puts(&out,
           "<link rel=\"alternate\" type=\"text/markdown\" href=\"/llms.txt\" "
           "title=\"Plain-text site guide\">\n");
  buf_puts(&out, "<meta property=\"og:type\" content=\"");
  buf_puts(&out, is_post ? "article" : "website");
  buf_puts(&out, "\">\n<meta property=\"og:title\" content=\"");
  html_escape(&out, title);
  buf_puts(&out, "\">\n<meta property=\"og:description\" content=\"");
  html_escape(&out, description);
  buf_puts(&out, "\">\n<meta property=\"og:url\" content=\"" ORIGIN);
  html_escape(&out, path);
  buf_puts(&out, "\">\n");
  buf_puts(&out, "<meta name=\"twitter:card\" content=\"summary\">\n");
  buf_puts(&out, "<meta name=\"twitter:creator\" content=\"@zachwilke_1\">\n");
  if (schema && schema[0]) {
    buf_puts(&out, "<script type=\"application/ld+json\">");
    buf_puts(&out, schema);
    buf_puts(&out, "</script>\n");
  } else {
    buf_puts(&out, "\n");
  }
  buf_puts(&out,
           "<script>try { const theme = localStorage.getItem(\"theme\"); if "
           "(theme === \"light\" || theme === \"dark\") "
           "document.documentElement.dataset.theme = theme; } catch "
           "{}</script>\n");
  buf_puts(&out, "<script src=\"/assets/site.js\" defer></script>\n");
  buf_puts(&out, "</head>\n<body class=\"page-");
  html_escape(&out, page);
  buf_puts(&out, "\">\n");
  buf_puts(&out, "<a class=\"skip-link\" href=\"#main\">Skip to content</a>\n");
  buf_puts(&out, "<div class=\"site-shell\">\n");
  buf_puts(&out, "  <header class=\"site-header\">\n");
  buf_puts(&out, "    <a class=\"wordmark\" href=\"/\">Zach Wilke</a>\n");
  buf_puts(&out,
           "    <nav class=\"site-nav\" aria-label=\"Main navigation\"><a "
           "href=\"/\"");
  if (strcmp(page, "home") == 0) buf_puts(&out, " aria-current=\"page\"");
  buf_puts(&out, ">About</a><a href=\"/blog/\"");
  if (blog) buf_puts(&out, " aria-current=\"page\"");
  buf_puts(&out, ">Writing</a><a href=\"/#workbench\">Projects</a></nav>\n");
  buf_puts(&out, "  </header>\n  ");
  buf_puts(&out, wired);
  buf_puts(&out, "\n  <footer class=\"site-footer\"><nav aria-label=\"Elsewhere\">");
  buf_puts(&out, "<a rel=\"me\" href=\"mailto:zach@pinefall.dev\">Email</a>");
  buf_puts(&out, "<a rel=\"me\" href=\"https://github.com/zachwilke\">GitHub</a>");
  buf_puts(&out, "<a rel=\"me\" href=\"https://x.com/zachwilke_1\">X</a>");
  buf_puts(&out, "<a rel=\"me\" href=\"https://world.hey.com/zwilke\">Hey World</a>");
  buf_puts(&out, "<a href=\"/feed.xml\">RSS</a></nav>");
  buf_printf(&out,
             "<div class=\"footer-bottom\"><span>© %d Zach Wilke</span>", year);
  buf_puts(&out,
           "<span>Texas <time id=\"texas-time\" aria-label=\"Current time in "
           "Texas\"></time></span>");
  buf_puts(&out,
           "<button id=\"theme-toggle\" hidden type=\"button\">Theme: "
           "system</button>");
  buf_puts(&out, "<a href=\"#main\">Back to top ↑</a></div></footer>\n");
  buf_puts(&out, "</div>\n</body>\n</html>");
  free(wired);
  char *html = buf_copy(&out);
  buf_free(&out);
  return html;
}

static void write_root(const char *root, const char *rel, const char *contents) {
  char *path = path_join(root, rel);
  write_trimmed(path, contents);
  free(path);
}

int main(int argc, char **argv) {
  const char *root = argc > 1 ? argv[1] : ".";

  char *index_path = path_join(root, "posts/index.json");
  char *index_json = read_file(index_path);
  free(index_path);

  char **slugs = NULL;
  size_t slug_count = 0;
  if (!parse_slug_index(index_json, &slugs, &slug_count)) {
    fprintf(stderr, "Post index must be a JSON array of slugs.\n");
    return 1;
  }
  free(index_json);

  for (size_t i = 0; i < slug_count; i++) {
    for (size_t j = i + 1; j < slug_count; j++) {
      if (strcmp(slugs[i], slugs[j]) == 0) {
        fprintf(stderr, "Post index must contain unique slugs.\n");
        return 1;
      }
    }
  }

  Post *posts = calloc(slug_count ? slug_count : 1, sizeof(*posts));
  if (!posts) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }

  for (size_t i = 0; i < slug_count; i++) {
    if (!valid_slug(slugs[i])) {
      fprintf(stderr, "Invalid slug: %s\n", slugs[i]);
      return 1;
    }
    char rel[256];
    snprintf(rel, sizeof(rel), "posts/%s.md", slugs[i]);
    char *md_path = path_join(root, rel);
    char *src = read_file(md_path);
    free(md_path);

    FrontMatter fm;
    parse_front_matter(src, &fm);
    free(src);
    if (!fm.title[0] || !valid_date(fm.date)) {
      fprintf(stderr, "Missing title or invalid date: %s\n", slugs[i]);
      front_matter_free(&fm);
      return 1;
    }

    posts[i].slug = xstrdup(slugs[i]);
    posts[i].title = xstrdup(fm.title);
    posts[i].date = xstrdup(fm.date);
    posts[i].summary = xstrdup(fm.summary);
    posts[i].body = xstrdup(fm.body);
    size_t path_len = strlen(slugs[i]) + sizeof("/blog//");
    posts[i].path = malloc(path_len);
    if (!posts[i].path) {
      fprintf(stderr, "out of memory\n");
      return 1;
    }
    snprintf(posts[i].path, path_len, "/blog/%s/", slugs[i]);
    int words = word_count(fm.body);
    posts[i].minutes = words <= 0 ? 1 : (words + 219) / 220;
    if (posts[i].minutes < 1) posts[i].minutes = 1;
    front_matter_free(&fm);
  }
  for (size_t i = 0; i < slug_count; i++) free(slugs[i]);
  free(slugs);

  if (slug_count) qsort(posts, slug_count, sizeof(*posts), cmp_posts_desc);

  int year = 2026;
  for (size_t i = 0; i < slug_count; i++) {
    int y = (posts[i].date[0] - '0') * 1000 + (posts[i].date[1] - '0') * 100 +
            (posts[i].date[2] - '0') * 10 + (posts[i].date[3] - '0');
    if (y > year) year = y;
  }

  Buf latest;
  buf_init(&latest);
  if (slug_count == 0) {
    buf_puts(&latest, "<p>The notebook is open. First notes coming soon.</p>");
  } else {
    size_t n = slug_count < 3 ? slug_count : 3;
    for (size_t i = 0; i < n; i++) {
      if (i) buf_putc(&latest, '\n');
      post_row(&latest, &posts[i]);
    }
  }

  Buf rings;
  buf_init(&rings);
  rings_html(&rings);

  char *home_path = path_join(root, "templates/home.html");
  char *home_tpl = read_file(home_path);
  free(home_path);
  char *home_mid = replace_once(home_tpl, "{{LATEST_POSTS}}",
                                latest.data ? latest.data : "");
  char *home = replace_once(home_mid, "{{RINGS}}", rings.data ? rings.data : "");
  free(home_tpl);
  free(home_mid);
  buf_free(&latest);
  buf_free(&rings);

  Buf schema;
  buf_init(&schema);
  person_schema(&schema);
  char *index_html =
      layout("Zach Wilke — Personal Field Notes",
             "Notes on making. IT operations, software, Linux, and life. The "
             "personal field notes of Zach Wilke, a builder and father of four "
             "in Texas.",
             "/", home, "home", schema.data, year);
  write_root(root, "index.html", index_html);
  free(index_html);
  free(home);
  buf_free(&schema);

  Buf archive_rows;
  buf_init(&archive_rows);
  if (slug_count == 0) {
    buf_puts(&archive_rows, "<p>First notes coming soon.</p>");
  } else {
    for (size_t i = 0; i < slug_count; i++) {
      if (i) buf_putc(&archive_rows, '\n');
      post_row(&archive_rows, &posts[i]);
    }
  }

  Buf blog_content;
  buf_init(&blog_content);
  buf_puts(&blog_content, "<main id=\"main\"><header class=\"notebook-header\">");
  buf_puts(&blog_content, "<h1>Writing</h1>");
  buf_puts(&blog_content,
           "<p>Notes on software, Linux, operations, and life.</p>");
  buf_puts(&blog_content, WRITING_NOTE);
  buf_puts(&blog_content, "<a href=\"/feed.xml\">Subscribe via RSS</a></header>");
  buf_puts(&blog_content,
           "<section class=\"archive\" aria-labelledby=\"archive-title\">");
  buf_printf(&blog_content,
             "<h2 id=\"archive-title\">All writing <span "
             "class=\"muted\">(%zu)</span></h2>",
             slug_count);
  buf_puts(&blog_content,
           "<form role=\"search\" id=\"archive-search\" hidden><label "
           "for=\"search\">Search the writing</label><div "
           "class=\"search-controls\"><input id=\"search\" type=\"search\" "
           "placeholder=\"Search titles and full text…\" "
           "autocomplete=\"off\"><button "
           "type=\"reset\">Clear</button></div><p id=\"search-status\" "
           "role=\"status\"></p></form>");
  buf_puts(&blog_content, "<div id=\"archive-posts\">");
  buf_puts(&blog_content, archive_rows.data ? archive_rows.data : "");
  buf_puts(&blog_content, "</div></section></main>");

  char *blog_html =
      layout("The Notebook — Zach Wilke",
             "Longer thoughts on software, Linux, operations, and life. Field "
             "notes by Zach Wilke.",
             "/blog/", blog_content.data, "blog", NULL, year);
  write_root(root, "blog/index.html", blog_html);
  free(blog_html);
  buf_free(&blog_content);

  Buf search;
  buf_init(&search);
  buf_putc(&search, '[');
  for (size_t i = 0; i < slug_count; i++) {
    if (i) buf_putc(&search, ',');
    buf_puts(&search, "{\"slug\":\"");
    json_escape(&search, posts[i].slug);
    buf_puts(&search, "\",\"text\":\"");
    json_escape(&search, posts[i].title);
    buf_putc(&search, ' ');
    if (posts[i].summary[0]) {
      json_escape(&search, posts[i].summary);
      buf_putc(&search, ' ');
    }
    json_escape(&search, posts[i].body);
    buf_puts(&search, "\"}");
  }
  buf_putc(&search, ']');
  write_root(root, "assets/search.json", search.data);
  buf_free(&search);

  char *not_found = layout(
      "Page not found — Zach Wilke",
      "Find your way back to Zach Wilke’s personal site.", "/404.html",
      "<main id=\"main\"><h1>Page not found</h1><p>This link may have moved, "
      "or the page may no longer exist.</p><p><a href=\"/\">Back home</a> · <a "
      "href=\"/blog/\">Browse the writing</a></p></main>",
      "error", NULL, year);
  write_root(root, "404.html", not_found);
  free(not_found);

  for (size_t i = 0; i < slug_count; i++) {
    Post *post = &posts[i];
    char *article = render_article(post->body);
    char *toc = article_toc(article);

    Buf content;
    buf_init(&content);
    buf_puts(&content, "<main id=\"main\"><header class=\"article-header\">");
    buf_puts(&content, "<a href=\"/blog/\">← All writing</a><h1>");
    html_escape(&content, post->title);
    buf_puts(&content, "</h1><p class=\"article-byline\"><time datetime=\"");
    html_escape(&content, post->date);
    buf_puts(&content, "\">");
    char display[32];
    format_display_date(display, sizeof(display), post->date);
    html_escape(&content, display);
    buf_printf(&content, "</time> · %d min read · Zach Wilke</p></header>",
               post->minutes);
    buf_puts(&content, toc);
    buf_puts(&content, "<article class=\"prose\" aria-label=\"");
    html_escape(&content, post->title);
    buf_puts(&content, "\">");
    buf_puts(&content, article);
    buf_puts(&content, "</article><div class=\"article-end\">");
    buf_puts(&content, WRITING_NOTE);
    buf_puts(&content, "<p>Have a thought? <a href=\"mailto:zach@pinefall.dev?subject=");
    Buf subject;
    buf_init(&subject);
    buf_puts(&subject, "Re: ");
    buf_puts(&subject, post->title);
    uri_encode(&content, subject.data);
    buf_free(&subject);
    buf_puts(&content, "\">Send me a note</a>.</p>");
    buf_puts(&content, "<div class=\"article-tools\"><button type=\"button\" "
                       "id=\"copy-link\" hidden>Copy article link</button>");
    buf_puts(&content, "<a href=\"/posts/");
    html_escape(&content, post->slug);
    buf_puts(&content, ".md\">Markdown source</a><a href=\"/feed.xml\">RSS</a>"
                       "</div><p id=\"copy-status\" role=\"status\"></p></div>");
    buf_puts(&content, "<nav class=\"read-next\" aria-label=\"More writing\">");
    if (i > 0) {
      buf_puts(&content, "<a href=\"");
      html_escape(&content, posts[i - 1].path);
      buf_puts(&content, "\">← Newer: ");
      html_escape(&content, posts[i - 1].title);
      buf_puts(&content, "</a>");
    }
    if (i + 1 < slug_count) {
      buf_puts(&content, "<a href=\"");
      html_escape(&content, posts[i + 1].path);
      buf_puts(&content, "\">Older: ");
      html_escape(&content, posts[i + 1].title);
      buf_puts(&content, " →</a>");
    }
    buf_puts(&content, "</nav></main>");

    Buf title;
    buf_init(&title);
    buf_puts(&title, post->title);
    buf_puts(&title, " — Zach Wilke");

    Buf pschema;
    buf_init(&pschema);
    post_schema(&pschema, post);

    char *page = layout(title.data,
                        post->summary[0] ? post->summary : post->title,
                        post->path, content.data, "post", pschema.data, year);

    char out_rel[256];
    snprintf(out_rel, sizeof(out_rel), "blog/%s/index.html", post->slug);
    write_root(root, out_rel, page);

    free(page);
    free(article);
    free(toc);
    buf_free(&content);
    buf_free(&title);
    buf_free(&pschema);
  }

  Buf legacy;
  buf_init(&legacy);
  buf_puts(&legacy, "<main id=\"main\" class=\"legacy-page\">");
  buf_puts(&legacy, "<p class=\"eyebrow\">The notebook has a new home</p>");
  buf_puts(&legacy, "<h1>Looking for<br><em>a field note?</em></h1>");
  buf_puts(&legacy,
           "<p id=\"legacy-message\">Select your entry below. Older links will "
           "take you to its new page automatically when JavaScript is "
           "enabled.</p>");
  buf_puts(&legacy, "<div class=\"post-list\">");
  buf_puts(&legacy, archive_rows.data ? archive_rows.data : "");
  buf_puts(&legacy, "</div></main>");
  char *legacy_html =
      layout("Find a Field Note — Zach Wilke",
             "Continue to the notebook to find a field note by Zach Wilke.",
             "/blog/", legacy.data, "legacy", NULL, year);
  write_root(root, "blog/post.html", legacy_html);
  free(legacy_html);
  buf_free(&legacy);
  buf_free(&archive_rows);

  Buf feed;
  buf_init(&feed);
  buf_puts(&feed, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  buf_puts(&feed, "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/"
                  "Atom\"><channel>");
  buf_puts(&feed, "<title>Zach Wilke — Field Notes</title>");
  buf_puts(&feed, "<link>" ORIGIN "/blog/</link>");
  buf_puts(&feed,
           "<description>Notes on making. Operations, software, and "
           "life.</description>");
  buf_puts(&feed, "<language>en-us</language>");
  buf_puts(&feed, "<atom:link href=\"" ORIGIN
                  "/feed.xml\" rel=\"self\" type=\"application/rss+xml\"/>");
  for (size_t i = 0; i < slug_count; i++) {
    char rfc[64];
    format_rfc822(rfc, sizeof(rfc), posts[i].date);
    char *html = render_markdown(posts[i].body);
    buf_puts(&feed, "<item><title>");
    html_escape(&feed, posts[i].title);
    buf_puts(&feed, "</title><link>" ORIGIN);
    html_escape(&feed, posts[i].path);
    buf_puts(&feed, "</link><guid isPermaLink=\"false\">" ORIGIN
                    "/blog/post.html?p=");
    html_escape(&feed, posts[i].slug);
    buf_puts(&feed, "</guid><pubDate>");
    buf_puts(&feed, rfc);
    buf_puts(&feed, "</pubDate><description>");
    html_escape(&feed, html);
    buf_puts(&feed, "</description></item>");
    if (i + 1 < slug_count) buf_putc(&feed, '\n');
    free(html);
  }
  buf_puts(&feed, "</channel></rss>");
  write_root(root, "feed.xml", feed.data);
  buf_free(&feed);

  Buf sitemap;
  buf_init(&sitemap);
  buf_puts(&sitemap, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  buf_puts(&sitemap,
           "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">");
  buf_puts(&sitemap, "<url><loc>" ORIGIN "/</loc></url>");
  buf_puts(&sitemap, "<url><loc>" ORIGIN "/blog/</loc></url>");
  for (size_t i = 0; i < slug_count; i++) {
    buf_puts(&sitemap, "<url><loc>" ORIGIN);
    html_escape(&sitemap, posts[i].path);
    buf_puts(&sitemap, "</loc><lastmod>");
    html_escape(&sitemap, posts[i].date);
    buf_puts(&sitemap, "</lastmod></url>");
  }
  buf_puts(&sitemap, "</urlset>");
  write_root(root, "sitemap.xml", sitemap.data);
  buf_free(&sitemap);

  Buf llms;
  buf_init(&llms);
  buf_puts(&llms,
           "# Zach Wilke\n\n"
           "> Director of Operations at an MSP in Texas. Twelve years in IT "
           "operations, six writing software. Interested in agentic AI and "
           "Linux. Husband to Hannah, father to four.\n\n"
           "Personal site: " ORIGIN "/ — a static personal journal. No tracking.\n"
           "Contact: zach@pinefall.dev\n\n"
           "## Beliefs\n\n"
           "- Christ is Lord.\n"
           "- Simple things are easier to keep alive.\n"
           "- Operations teaches you what actually breaks; development teaches "
           "you why.\n"
           "- The agentic era is the most exciting shift I've seen in twelve "
           "years of doing this.\n"
           "- Family first. Everything else is details.\n\n"
           "## Projects\n\n"
           "- [Binder](https://binder.school): Homeschool planning, lessons, "
           "and records.\n"
           "- [Omarchy MX Control](https://omarchyplugins.com/plugin.html?id="
           "io.github.zachwilke.mx): An Omarchy plugin.\n"
           "- [mylight](https://github.com/zachwilke/mylight): A family "
           "dashboard for calendars, chores, and meals.\n\n"
           "## Notebook\n\n"
           "[All writing](" ORIGIN "/blog/) · [RSS](" ORIGIN "/feed.xml)\n");
  for (size_t i = 0; i < slug_count; i++) {
    buf_printf(&llms, "- [%s](" ORIGIN "%s) — %s\n  Markdown source: " ORIGIN
                      "/posts/%s.md\n",
               posts[i].title, posts[i].path, posts[i].date, posts[i].slug);
  }
  buf_puts(&llms,
           "\n## Elsewhere\n\n"
           "- [GitHub](https://github.com/zachwilke)\n"
           "- [X](https://x.com/zachwilke_1)\n"
           "- [Hey World](https://world.hey.com/zwilke)\n");
  write_root(root, "llms.txt", llms.data);
  buf_free(&llms);

  for (size_t i = 0; i < slug_count; i++) {
    free(posts[i].slug);
    free(posts[i].title);
    free(posts[i].date);
    free(posts[i].summary);
    free(posts[i].body);
    free(posts[i].path);
  }
  free(posts);

  printf("Published %zu static posts, homepage, notebook, RSS, sitemap, and "
         "site guide. Built with C. No runtime dependencies.\n",
         slug_count);
  return 0;
}
