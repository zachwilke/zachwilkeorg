# Publish a field note

1. Create `posts/your-slug.md` with `title`, an ISO `date`, and an optional `summary` in front matter:

   ```md
   ---
   title: Your post title
   date: 2026-09-05
   summary: A short description for the notebook.
   ---

   Your writing goes here.
   ```

2. Add `"your-slug"` to `posts/index.json`. Slugs use lowercase letters, digits, and single hyphens between words.
3. Run `node scripts/build.mjs` from the repository root.
4. Preview the site and commit the source and generated files together.

Newest posts are listed first based on the date field. The publishing helper updates the homepage's three latest entries, full notebook index, individual static posts, RSS feed, sitemap, and `llms.txt` automatically. There is no deployment build or runtime Markdown fetching.

The canonical URL is `https://zachwilke.org/blog/your-slug/`. Old `/blog/post.html?p=your-slug` and `/blog/?p=your-slug` links are preserved with a JavaScript redirect; the compatibility page also provides ordinary links without JavaScript.

When deleting or renaming a previously published post, decide whether to keep its old static page or add a redirect; the helper intentionally does not delete old output directories.
