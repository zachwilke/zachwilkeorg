// Dependency-free publishing helper. Generated HTML is committed and served as-is.
import { readFile, writeFile, mkdir } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import { resolve, dirname } from "node:path";
import vm from "node:vm";

const root = fileURLToPath(new URL("../", import.meta.url));
const read = (path) => readFile(resolve(root, path), "utf8");
const write = async (path, value) => {
  const target = resolve(root, path);
  await mkdir(dirname(target), { recursive: true });
  await writeFile(target, value.trim() + "\n");
};
const markdown = vm.createContext({});
vm.runInContext(await read("vendor/markdown.js"), markdown);
const escape = (value) =>
  String(value).replace(
    /[&<>"']/g,
    (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[
        c
      ],
  );
const origin = "https://zachwilke.org";
const slugs = JSON.parse(await read("posts/index.json"));
if (!Array.isArray(slugs) || new Set(slugs).size !== slugs.length)
  throw new Error("Post index must contain unique slugs.");
const posts = await Promise.all(
  slugs.map(async (slug) => {
    if (typeof slug !== "string" || !/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(slug))
      throw new Error(`Invalid slug: ${slug}`);
    const { meta, body } = markdown.parseFrontMatter(
      await read(`posts/${slug}.md`),
    );
    if (
      !meta.title ||
      !/^\d{4}-\d{2}-\d{2}$/.test(meta.date || "") ||
      Number.isNaN(Date.parse(meta.date)) ||
      new Date(meta.date).toISOString().slice(0, 10) !== meta.date
    )
      throw new Error(`Missing title or invalid date: ${slug}`);
    return {
      ...meta,
      slug,
      body,
      path: `/blog/${slug}/`,
      minutes: Math.max(1, Math.ceil(body.trim().split(/\s+/).length / 220)),
    };
  }),
);
posts.sort((a, b) => b.date.localeCompare(a.date));
const date = (iso) =>
  new Date(`${iso}T00:00:00Z`).toLocaleDateString("en-US", {
    month: "short",
    day: "2-digit",
    year: "numeric",
    timeZone: "UTC",
  });
const year = Math.max(2026, ...posts.map((p) => Number(p.date.slice(0, 4))));

function layout({ title, description, path, content, page = "home", schema }) {
  const blog = page === "blog" || page === "post";
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light dark">
<meta name="theme-color" content="#faf9f6">
<title>${escape(title)}</title>
<meta name="description" content="${escape(description)}">
${page === "error" ? '<meta name="robots" content="noindex">' : ""}
<link rel="canonical" href="${origin}${path}">
<link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">

<link rel="stylesheet" href="/assets/site.css">
<link rel="alternate" type="application/rss+xml" title="Zach Wilke — Field Notes" href="/feed.xml">
<link rel="alternate" type="text/markdown" href="/llms.txt" title="Plain-text site guide">
<meta property="og:type" content="${page === "post" ? "article" : "website"}">
<meta property="og:title" content="${escape(title)}">
<meta property="og:description" content="${escape(description)}">
<meta property="og:url" content="${origin}${path}">
<meta name="twitter:card" content="summary">
<meta name="twitter:creator" content="@zachwilke_1">
${schema ? `<script type="application/ld+json">${JSON.stringify(schema).replace(/</g, "\\u003c")}</script>` : ""}
<script>try { const theme = localStorage.getItem("theme"); if (theme === "light" || theme === "dark") document.documentElement.dataset.theme = theme; } catch {}</script>
<script src="/assets/site.js" defer></script>
</head>
<body class="page-${page}">
<a class="skip-link" href="#main">Skip to content</a>
<div class="site-shell">
  <header class="site-header">
    <a class="wordmark" href="/">Zach Wilke</a>
    <nav class="site-nav" aria-label="Main navigation"><a href="/"${page === "home" ? ' aria-current="page"' : ""}>About</a><a href="/blog/"${blog ? ' aria-current="page"' : ""}>Writing</a><a href="/#workbench">Projects</a></nav>
  </header>
  ${content.replace('<main id="main"', '<main id="main" tabindex="-1"')}
  <footer class="site-footer"><nav aria-label="Elsewhere"><a rel="me" href="mailto:zach@pinefall.dev">Email</a><a rel="me" href="https://github.com/zachwilke">GitHub</a><a rel="me" href="https://x.com/zachwilke_1">X</a><a rel="me" href="https://world.hey.com/zwilke">Hey World</a><a href="/feed.xml">RSS</a></nav><div class="footer-bottom"><span>© ${year} Zach Wilke</span><span>Texas <time id="texas-time" aria-label="Current time in Texas"></time></span><button id="theme-toggle" hidden type="button">Theme: system</button><a href="#main">Back to top ↑</a></div></footer>
</div>
</body>
</html>`;
}

function postRow(post) {
  return `<article class="post-row" data-slug="${post.slug}"><time datetime="${post.date}">${date(post.date)}</time><div><h3><a href="${post.path}" class="post-row-link">${escape(post.title)}</a></h3><p>${escape(post.summary || "")}</p><span class="reading-time">${post.minutes} min read</span></div></article>`;
}

let home = await read("templates/home.html");
home = home.replace(
  "{{LATEST_POSTS}}",
  posts.length
    ? posts.slice(0, 3).map(postRow).join("\n")
    : "<p>The notebook is open. First notes coming soon.</p>",
);
home = home.replace(
  "{{RINGS}}",
  Array.from(
    { length: 14 },
    (_, i) =>
      `<div class="apparatus-ring" data-lens="${["operations", "software", "life"][i % 3]}" style="--ring:${i};--offset:${(i - 6.5) * 17}px"></div>`,
  ).join(""),
);
await write(
  "index.html",
  layout({
    title: "Zach Wilke — Personal Field Notes",
    description:
      "Notes on making. IT operations, software, Linux, and life. The personal field notes of Zach Wilke, a builder and father of four in Texas.",
    path: "/",
    content: home,
    schema: {
      "@context": "https://schema.org",
      "@type": "Person",
      name: "Zach Wilke",
      url: origin,
      email: "zach@pinefall.dev",
      jobTitle: "Director of Operations",
      knowsAbout: [
        "IT operations",
        "Software development",
        "Agentic AI",
        "Linux",
      ],
      sameAs: [
        "https://github.com/zachwilke",
        "https://x.com/zachwilke_1",
        "https://world.hey.com/zwilke",
      ],
    },
  }),
);

await write(
  "blog/index.html",
  layout({
    title: "The Notebook — Zach Wilke",
    description:
      "Longer thoughts on software, Linux, operations, and life. Field notes by Zach Wilke.",
    path: "/blog/",
    page: "blog",
    content: `<main id="main"><header class="notebook-header"><h1>Writing</h1><p>Notes on software, Linux, operations, and life.</p><a href="/feed.xml">Subscribe via RSS</a></header><section class="archive" aria-labelledby="archive-title"><h2 id="archive-title">All writing <span class="muted">(${posts.length})</span></h2><form role="search" id="archive-search" hidden><label for="search">Search the writing</label><div class="search-controls"><input id="search" type="search" placeholder="Search titles and full text…" autocomplete="off"><button type="reset">Clear</button></div><p id="search-status" role="status"></p></form><div id="archive-posts">${posts.length ? posts.map(postRow).join("\n") : "<p>First notes coming soon.</p>"}</div></section></main>`,
  }),
);

function renderArticle(body) {
  let index = 0;
  return markdown
    .renderMarkdown(body)
    .replace(
      /<h([1-6])>([\s\S]*?)<\/h\1>/g,
      (_, level, text) =>
        `<h${level} id="section-${++index}">${text} <a class="heading-anchor" href="#section-${index}" aria-label="Link to section ${index}">#</a></h${level}>`,
    );
}
function articleContents(body) {
  const headings = [
    ...renderArticle(body).matchAll(
      /<h([1-6]) id="([^"]+)">([\s\S]*?) <a class="heading-anchor"/g,
    ),
  ];
  return headings.length > 1
    ? `<details class="toc"><summary>On this page</summary><ul>${headings.map((h) => `<li><a href="#${h[2]}">${h[3].replace(/<[^>]*>/g, "")}</a></li>`).join("")}</ul></details>`
    : "";
}
await write(
  "assets/search.json",
  JSON.stringify(
    posts.map(({ slug, title, summary, body }) => ({
      slug,
      text: [title, summary, body].filter(Boolean).join(" "),
    })),
  ),
);
await write(
  "404.html",
  layout({
    title: "Page not found — Zach Wilke",
    description: "Find your way back to Zach Wilke’s personal site.",
    path: "/404.html",
    page: "error",
    content: `<main id="main"><h1>Page not found</h1><p>This link may have moved, or the page may no longer exist.</p><p><a href="/">Back home</a> · <a href="/blog/">Browse the writing</a></p></main>`,
  }),
);

for (let i = 0; i < posts.length; i++) {
  const post = posts[i];
  await write(
    `blog/${post.slug}/index.html`,
    layout({
      title: `${post.title} — Zach Wilke`,
      description: post.summary || post.title,
      path: post.path,
      page: "post",
      schema: {
        "@context": "https://schema.org",
        "@type": "BlogPosting",
        headline: post.title,
        description: post.summary || post.title,
        datePublished: post.date,
        author: { "@type": "Person", name: "Zach Wilke", url: origin },
        mainEntityOfPage: origin + post.path,
      },
      content: `<main id="main"><header class="article-header"><a href="/blog/">← All writing</a><h1>${escape(post.title)}</h1><p class="article-byline"><time datetime="${post.date}">${date(post.date)}</time> · ${post.minutes} min read · Zach Wilke</p></header>${articleContents(post.body)}<article class="prose" aria-label="${escape(post.title)}">${renderArticle(post.body)}</article><div class="article-end"><p>Have a thought? <a href="mailto:zach@pinefall.dev?subject=${encodeURIComponent(`Re: ${post.title}`)}">Send me a note</a>.</p><div class="article-tools"><button type="button" id="copy-link" hidden>Copy article link</button><a href="/posts/${post.slug}.md">Markdown source</a><a href="/feed.xml">RSS</a></div><p id="copy-status" role="status"></p></div><nav class="read-next" aria-label="More writing">${posts[i - 1] ? `<a href="${posts[i - 1].path}">← Newer: ${escape(posts[i - 1].title)}</a>` : ""}${posts[i + 1] ? `<a href="${posts[i + 1].path}">Older: ${escape(posts[i + 1].title)} →</a>` : ""}</nav></main>`,
    }),
  );
}

await write(
  "blog/post.html",
  layout({
    title: "Find a Field Note — Zach Wilke",
    description: "Continue to the notebook to find a field note by Zach Wilke.",
    path: "/blog/",
    page: "legacy",
    content: `<main id="main" class="legacy-page"><p class="eyebrow">The notebook has a new home</p><h1>Looking for<br><em>a field note?</em></h1><p id="legacy-message">Select your entry below. Older links will take you to its new page automatically when JavaScript is enabled.</p><div class="post-list">${posts.map(postRow).join("\n")}</div></main>`,
  }),
);
await write(
  "feed.xml",
  `<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0" xmlns:atom="http://www.w3.org/2005/Atom"><channel><title>Zach Wilke — Field Notes</title><link>${origin}/blog/</link><description>Notes on making. Operations, software, and life.</description><language>en-us</language><atom:link href="${origin}/feed.xml" rel="self" type="application/rss+xml"/>${posts.map((post) => `<item><title>${escape(post.title)}</title><link>${origin}${post.path}</link><guid isPermaLink="false">${origin}/blog/post.html?p=${post.slug}</guid><pubDate>${new Date(post.date + "T12:00:00Z").toUTCString()}</pubDate><description>${escape(markdown.renderMarkdown(post.body))}</description></item>`).join("\n")}</channel></rss>`,
);
await write(
  "sitemap.xml",
  `<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"><url><loc>${origin}/</loc></url><url><loc>${origin}/blog/</loc></url>${posts.map((post) => `<url><loc>${origin}${post.path}</loc><lastmod>${post.date}</lastmod></url>`).join("")}</urlset>`,
);
await write(
  "llms.txt",
  `# Zach Wilke

> Director of Operations at an MSP in Texas. Twelve years in IT operations, six writing software. Interested in agentic AI and Linux. Husband to Hannah, father to four.

Personal site: ${origin}/ — a static personal journal. No tracking.
Contact: zach@pinefall.dev

## Beliefs

- Christ is Lord.
- Simple things are easier to keep alive.
- Operations teaches you what actually breaks; development teaches you why.
- The agentic era is the most exciting shift I've seen in twelve years of doing this.
- Family first. Everything else is details.

## Projects

- [Binder](https://binder.school): Homeschool planning, lessons, and records.
- [Omarchy MX Control](https://omarchyplugins.com/plugin.html?id=io.github.zachwilke.mx): An Omarchy plugin.
- [mylight](https://github.com/zachwilke/mylight): A family dashboard for calendars, chores, and meals.

## Notebook

[All writing](${origin}/blog/) · [RSS](${origin}/feed.xml)
${posts.map((p) => `- [${p.title}](${origin}${p.path}) — ${p.date}\n  Markdown source: ${origin}/posts/${p.slug}.md`).join("\n")}

## Elsewhere

- [GitHub](https://github.com/zachwilke)
- [X](https://x.com/zachwilke_1)
- [Hey World](https://world.hey.com/zwilke)
`,
);
console.log(
  `Published ${posts.length} static posts, homepage, notebook, RSS, sitemap, and site guide. No runtime dependencies.`,
);
