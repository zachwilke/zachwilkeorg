# Zach Wilke — Personal Field Notes

A static personal site with the Field Apparatus visual direction: olive paper, editorial typography, technical drawings, and an interactive CSS 3D apparatus. No frameworks, tracking, remote fonts, or runtime dependencies.

## Work on the site

- Edit homepage content in `templates/home.html`.
- Edit shared layout, archive, and article markup in `scripts/build.mjs`.
- Edit styling and progressive enhancements in `assets/site.css` and `assets/site.js`.
- Run `node scripts/build.mjs` after content or layout changes. This small publishing helper uses only Node built-ins and the existing Markdown renderer. It writes ready-to-serve HTML, RSS, sitemap, and the agent-readable site guide.
- Preview with `python3 -m http.server 8080`, then open `http://localhost:8080`.

Generated files are committed. Hosting serves them directly; no npm install or deployment build is needed. The `.assetsignore` file keeps authoring files out of Cloudflare's static assets upload.

## Write a post

See `posts/README.md`. Posts are Markdown with front matter. The publishing helper generates `/blog/<slug>/index.html`, and older `/blog/post.html?p=<slug>` links continue to work through a compatibility page. All articles and the index are readable with JavaScript disabled. A full-text RSS feed is at `/feed.xml`.

## Motion and accessibility

The homepage apparatus uses CSS 3D rings and the native Web Animations API. It responds to a fine pointer and has keyboard-accessible disassembly and pause controls. Rotation pauses when offscreen or the browser tab is hidden, and reduced-motion preferences disable continuous movement and pointer tilt. No WebGL canvas or animation library is loaded. Posts have a quiet reading layout, semantic HTML, skip navigation, and print styles.

## Content provenance

Biography, beliefs, and current interests are adapted from the original homepage and posts. Project descriptions are grounded in https://binder.school/ and https://github.com/zachwilke/mylight. No project metrics, testimonials, unpublished articles, or personal history have been invented. The MX Control entry uses the existing project name and Omarchy context.

## Type

IBM Plex Mono Regular, Latin subset, is self-hosted under the SIL Open Font License. See `assets/fonts/OFL.txt`. Body text uses system fonts and Georgia.
