# Zach Wilke — Personal Field Notes

A personal notebook about operations, software, Linux, and life. A quiet, text-first design with a narrow reading column, simple navigation, and optional tools that stay out of the way.

[Visit the site](https://zachwilke.org/) · [Read the notebook](https://zachwilke.org/blog/) · [Subscribe via RSS](https://zachwilke.org/feed.xml)

## What’s here

- An optional interactive CSS 3D study with pointer response, disassembly, and pause controls.
- Recent writing, project links, current interests, and personal principles.
- Full-text archive search with shareable query URLs and useful empty/error states.
- Article pages with reading times, newer/older navigation, section links, automatic contents for longer posts, Markdown source links, and copy buttons for code and article URLs.
- A generated 404 page with routes back to the site.
- System, light, and dark themes with a saved preference, mobile layouts, reduced-motion support, keyboard navigation, and print styles.
- A full-text RSS feed, sitemap, article metadata, and a [plain-text site guide](llms.txt).

The site uses plain HTML, CSS, and JavaScript. There are no frontend frameworks, analytics, remote font requests, or runtime Markdown fetches. Articles and navigation work with JavaScript disabled. The interface uses system fonts; the 3D apparatus uses the browser’s native animation API without WebGL or an animation library.

## Preview locally

Use Node.js to generate pages and Python 3 to serve them locally. No `npm install` is required.

```sh
git clone https://github.com/zachwilke/zachwilkeorg.git
cd zachwilkeorg
node scripts/build.mjs
python3 -m http.server 8080 --bind 127.0.0.1
```

Open [http://127.0.0.1:8080](http://127.0.0.1:8080). Refresh after editing CSS or JavaScript. Run the publishing helper again after editing Markdown, templates, or the shared layout.

## Publish a post

### 1. Write the Markdown

Create a file such as `posts/my-next-post.md`:

```markdown
---
title: My Next Post
date: 2026-09-05
summary: A short description for the notebook.
---

Your writing goes here. Links, images, headings, lists, quotes,
and code blocks are supported.
```

`title` and `date` are required. Use a real date in `YYYY-MM-DD` format. `summary` is optional. Filenames use lowercase letters, digits, and single hyphens between words.

### 2. Register the post

Add the filename without `.md` to [posts/index.json](posts/index.json):

```json
["my-next-post", "one-week-on-omarcy-quattro", "the-beginning"]
```

Entries are sorted newest first by their date, regardless of their order in this array. Every listed post is published when generated files are deployed; a future date does not schedule publication.

### 3. Generate and preview

```sh
node scripts/build.mjs
```

This updates the homepage’s three latest entries, notebook archive, individual article pages, search index, 404 page, RSS feed, sitemap, and `llms.txt`. Preview the result locally before pushing.

The example post’s URL will be `https://zachwilke.org/blog/my-next-post/`.

### 4. Commit and push

Commit both the Markdown source and generated output. For the example above:

```sh
git add posts/my-next-post.md posts/index.json \
  blog/my-next-post/index.html blog/index.html blog/post.html \
  index.html 404.html assets/search.json feed.xml sitemap.xml llms.txt
git commit -m "Add My Next Post"
git push origin main
```

The updated files must then be deployed through the site’s Cloudflare setup. A Git push updates the repository; whether it also triggers deployment depends on the Cloudflare Git integration settings.

To edit an existing post, change its Markdown, regenerate, and commit the updated source and output. For deletion or a slug change, handle the previous URL deliberately: the helper does not delete old article directories automatically. See [the post authoring guide](posts/README.md) for additional details.

## Where to make changes

| File                                       | Purpose                                                                                           |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------- |
| [templates/home.html](templates/home.html) | Homepage copy, projects, and optional mechanical study                                            |
| [scripts/build.mjs](scripts/build.mjs)     | Shared layout, archive, article pages, and publishing helper                                      |
| [assets/site.css](assets/site.css)         | Typography, colors, layouts, and responsive styles                                                |
| [assets/site.js](assets/site.js)           | Search, theme preference, copy buttons, apparatus interaction, Texas clock, and legacy post links |
| [posts/](posts/)                           | Markdown posts and the post index                                                                 |
| [vendor/markdown.js](vendor/markdown.js)   | Local Markdown renderer used during generation                                                    |
| [wrangler.jsonc](wrangler.jsonc)           | Cloudflare Worker static assets configuration                                                     |
| [.assetsignore](.assetsignore)             | Files excluded from the static assets upload                                                      |

`index.html`, `404.html`, `assets/search.json`, the HTML under `blog/`, `feed.xml`, `sitemap.xml`, and `llms.txt` are generated files. Make lasting changes in their sources and run the publishing helper rather than editing those outputs directly.

## Hosting and compatibility

Cloudflare serves the committed files as static assets through the `zachwilkeorg` Worker. Generation happens before committing, so hosting needs no dependency installation or build step.

Older `/blog/post.html?p=slug` and `/blog/?p=slug` links redirect to the matching published article when JavaScript is enabled. The compatibility page also provides ordinary article links without JavaScript.

The apparatus pauses while offscreen or when the tab is hidden. Reduced-motion preferences disable continuous rotation and pointer tilt; reading and navigation remain available.

## Typography

The site uses system sans-serif and monospace fonts with no font requests. The previously used IBM Plex Mono subset remains available under the [SIL Open Font License](assets/fonts/OFL.txt).
