/* Tiny in-repo Markdown subset + YAML-ish front matter. No CDN, no tracking.
   Headings, paragraphs, emphasis, strong, inline code, fenced code, links,
   images, lists, blockquotes, thematic breaks. HTML in the source is escaped. */
(function (root) {
  function escapeHtml(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function parseFrontMatter(src) {
    var text = String(src).replace(/^\uFEFF/, "");
    var m = text.match(/^---[ \t]*\r?\n([\s\S]*?)\r?\n---[ \t]*\r?\n?/);
    var meta = {};
    var body = text;
    if (m) {
      body = text.slice(m[0].length);
      m[1].split(/\r?\n/).forEach(function (line) {
        var i = line.indexOf(":");
        if (i === -1) return;
        var key = line.slice(0, i).trim();
        var val = line.slice(i + 1).trim();
        if (
          (val.charAt(0) === '"' && val.charAt(val.length - 1) === '"') ||
          (val.charAt(0) === "'" && val.charAt(val.length - 1) === "'")
        ) {
          val = val.slice(1, -1);
        }
        if (key) meta[key] = val;
      });
    }
    return { meta: meta, body: body };
  }

  function safeUrl(href) {
    var trimmed = String(href).trim();
    if (/^(https?:|mailto:|\/|#)/i.test(trimmed)) return trimmed;
    return "";
  }

  function emph(escaped) {
    return escaped
      .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
      .replace(/\*([^*]+)\*/g, "<em>$1</em>");
  }

  function inline(raw) {
    var re = /`([^`]+)`|!\[([^\]]*)\]\(([^)]+)\)|\[([^\]]+)\]\(([^)]+)\)/g;
    var out = "";
    var last = 0;
    var m;
    while ((m = re.exec(raw))) {
      out += emph(escapeHtml(raw.slice(last, m.index)));
      if (m[1] != null) {
        out += "<code>" + escapeHtml(m[1]) + "</code>";
      } else if (m[3] != null) {
        var img = safeUrl(m[3]);
        out += img
          ? '<img src="' + escapeHtml(img) + '" alt="' + escapeHtml(m[2]) + '">'
          : escapeHtml(m[2]);
      } else {
        var href = safeUrl(m[5]);
        var text = emph(escapeHtml(m[4]));
        if (!href) {
          out += text;
        } else {
          var ext = /^https?:/i.test(href)
            ? ' rel="noopener noreferrer"'
            : "";
          out += '<a href="' + escapeHtml(href) + '"' + ext + ">" + text + "</a>";
        }
      }
      last = m.index + m[0].length;
    }
    out += emph(escapeHtml(raw.slice(last)));
    return out;
  }

  function renderMarkdown(src) {
    var lines = String(src).replace(/\r\n/g, "\n").replace(/\n+$/, "").split("\n");
    var out = [];
    var i = 0;
    var para = [];

    function flushPara() {
      if (!para.length) return;
      out.push("<p>" + inline(para.join("\n")) + "</p>");
      para = [];
    }

    while (i < lines.length) {
      var line = lines[i];
      var fence = line.match(/^```(.*)$/);
      if (fence) {
        flushPara();
        var lang = fence[1].trim();
        var code = [];
        i += 1;
        while (i < lines.length && !/^```/.test(lines[i])) {
          code.push(lines[i]);
          i += 1;
        }
        if (i < lines.length) i += 1;
        var cls = lang ? ' class="language-' + escapeHtml(lang) + '"' : "";
        out.push("<pre><code" + cls + ">" + escapeHtml(code.join("\n")) + "</code></pre>");
        continue;
      }

      if (/^([*_-])\1\1+$/.test(line.trim())) {
        flushPara();
        out.push("<hr>");
        i += 1;
        continue;
      }

      var heading = line.match(/^(#{1,6})[ \t]+(.+?)[ \t]*#*[ \t]*$/);
      if (heading) {
        flushPara();
        var n = heading[1].length;
        out.push("<h" + n + ">" + inline(heading[2]) + "</h" + n + ">");
        i += 1;
        continue;
      }

      if (/^>[ \t]?/.test(line)) {
        flushPara();
        var quoted = [];
        while (i < lines.length && /^>[ \t]?/.test(lines[i])) {
          quoted.push(lines[i].replace(/^>[ \t]?/, ""));
          i += 1;
        }
        out.push("<blockquote>" + renderMarkdown(quoted.join("\n")) + "</blockquote>");
        continue;
      }

      if (/^[ \t]*[-*+][ \t]+/.test(line)) {
        flushPara();
        out.push("<ul>");
        while (i < lines.length && /^[ \t]*[-*+][ \t]+/.test(lines[i])) {
          out.push("<li>" + inline(lines[i].replace(/^[ \t]*[-*+][ \t]+/, "")) + "</li>");
          i += 1;
        }
        out.push("</ul>");
        continue;
      }

      if (/^[ \t]*\d+\.[ \t]+/.test(line)) {
        flushPara();
        out.push("<ol>");
        while (i < lines.length && /^[ \t]*\d+\.[ \t]+/.test(lines[i])) {
          out.push("<li>" + inline(lines[i].replace(/^[ \t]*\d+\.[ \t]+/, "")) + "</li>");
          i += 1;
        }
        out.push("</ol>");
        continue;
      }

      if (line.trim() === "") {
        flushPara();
        i += 1;
        continue;
      }

      para.push(line);
      i += 1;
    }
    flushPara();
    return out.join("\n");
  }

  root.parseFrontMatter = parseFrontMatter;
  root.renderMarkdown = renderMarkdown;
})(typeof window !== "undefined" ? window : globalThis);
