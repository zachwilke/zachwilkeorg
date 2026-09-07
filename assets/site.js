// Progressive enhancement only. Every page and post is readable without JS.
(() => {
  const legacy = new URLSearchParams(location.search).get("p");
  if (
    legacy &&
    /^\/blog\/(?:index\.html|post(?:\.html)?)?$/.test(location.pathname)
  ) {
    if (/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(legacy)) {
      // Only redirect to a published entry. Unknown links keep a useful index.
      const target = `/blog/${legacy}/`;
      if (
        Array.from(document.querySelectorAll(".post-row-link")).some(
          (a) => a.getAttribute("href") === target,
        )
      ) {
        location.replace(target + location.hash);
        return;
      }
    }
    const message = document.getElementById("legacy-message");
    if (message)
      message.textContent =
        "That field note could not be found. You can explore the notebook below.";
  }
  const themeButton = document.getElementById("theme-toggle");
  let theme = document.documentElement.dataset.theme || "system";
  function syncTheme() {
    if (theme === "system") delete document.documentElement.dataset.theme;
    else document.documentElement.dataset.theme = theme;
    themeButton.textContent = `Theme: ${theme}`;
    themeButton.setAttribute(
      "aria-label",
      `Color theme: ${theme}. Switch to ${theme === "system" ? "light" : theme === "light" ? "dark" : "system"}.`,
    );
  }
  if (themeButton) {
    themeButton.hidden = false;
    syncTheme();
    themeButton.addEventListener("click", () => {
      theme =
        theme === "system" ? "light" : theme === "light" ? "dark" : "system";
      try {
        localStorage.setItem("theme", theme);
      } catch {}
      syncTheme();
    });
  }
  const searchForm = document.getElementById("archive-search");
  if (searchForm) {
    const input = document.getElementById("search");
    const status = document.getElementById("search-status");
    const rows = [...document.querySelectorAll("#archive-posts .post-row")];
    let searchIndex;
    let pending;
    searchForm.hidden = false;
    input.value = new URLSearchParams(location.search).get("q") || "";
    const normalize = (text) =>
      text
        .normalize("NFKD")
        .replace(/[\u0300-\u036f]/g, "")
        .toLowerCase();
    async function filter() {
      const query = input.value.trim();
      const url = new URL(location.href);
      if (query) url.searchParams.set("q", query);
      else url.searchParams.delete("q");
      history.replaceState(null, "", url);
      if (!query) {
        rows.forEach((row) => (row.hidden = false));
        status.textContent = "";
        return;
      }
      status.textContent = "Searching…";
      try {
        if (!searchIndex) {
          pending ||= fetch("/assets/search.json")
            .then((response) => {
              if (!response.ok) throw new Error("Search unavailable");
              return response.json();
            })
            .catch((error) => {
              pending = null;
              throw error;
            });
          searchIndex = await pending;
        }
        if (query !== input.value.trim()) return;
        const terms = normalize(query).split(/\s+/);
        const matching = new Set(
          searchIndex
            .filter((post) =>
              terms.every((term) => normalize(post.text).includes(term)),
            )
            .map((post) => post.slug),
        );
        rows.forEach((row) => (row.hidden = !matching.has(row.dataset.slug)));
        status.textContent = matching.size
          ? `${matching.size} ${matching.size === 1 ? "entry" : "entries"} found.`
          : "No entries found. Try a different word or clear your search.";
      } catch {
        if (query !== input.value.trim()) return;
        rows.forEach((row) => (row.hidden = false));
        status.textContent =
          "Search is temporarily unavailable. All writing is listed below.";
      }
    }
    searchForm.addEventListener("submit", (event) => {
      event.preventDefault();
      filter();
    });
    input.addEventListener("input", filter);
    searchForm.addEventListener("reset", () => {
      input.value = "";
      filter();
      input.focus();
    });
    if (input.value) filter();
  }
  const copyLink = document.getElementById("copy-link");
  if (copyLink && navigator.clipboard) {
    copyLink.hidden = false;
    copyLink.addEventListener("click", async () => {
      const status = document.getElementById("copy-status");
      try {
        await navigator.clipboard.writeText(
          document.querySelector('link[rel="canonical"]').href,
        );
        status.textContent = "Article link copied.";
      } catch {
        status.textContent =
          "Could not copy. You can copy the address from your browser.";
      }
    });
  }
  if (navigator.clipboard)
    document.querySelectorAll(".prose pre").forEach((pre) => {
      const code = pre.querySelector("code");
      if (!code) return;
      const button = document.createElement("button");
      button.type = "button";
      button.className = "code-copy";
      button.textContent = "Copy code";
      button.setAttribute("aria-live", "polite");
      button.addEventListener("click", async () => {
        try {
          await navigator.clipboard.writeText(code.textContent);
          button.textContent = "Copied";
        } catch {
          button.textContent = "Could not copy — select the code";
        }
        setTimeout(() => (button.textContent = "Copy code"), 2500);
      });
      pre.prepend(button);
    });
  const time = document.getElementById("texas-time");
  function updateTime() {
    if (!time || document.hidden) return;
    const now = new Date();
    time.dateTime = now.toISOString();
    time.textContent = new Intl.DateTimeFormat("en-US", {
      timeZone: "America/Chicago",
      hour: "2-digit",
      minute: "2-digit",
      hour12: false,
    }).format(now);
  }
  updateTime();
  document.addEventListener("visibilitychange", updateTime);
  if (time) setInterval(updateTime, 60000);
  const apparatus = document.querySelector(".apparatus");
  if (!apparatus) return;
  const stage = apparatus.querySelector(".apparatus-stage");
  const spin = apparatus.querySelector(".apparatus-spin");
  const tilt = apparatus.querySelector(".apparatus-tilt");
  const motionButton = document.getElementById("motion-toggle");
  const explodeButton = document.getElementById("apparatus-explode");
  const story = document.getElementById("apparatus-story");
  const summary = apparatus.querySelector(".apparatus-summary");
  const lenses = Array.from(
    apparatus.querySelectorAll(".apparatus-lenses [data-lens]"),
  );
  const reduced = matchMedia("(prefers-reduced-motion: reduce)");
  const finePointer = matchMedia("(hover: hover) and (pointer: fine)");
  let visible = false;
  let paused = false;
  let animation;
  apparatus.querySelector(".apparatus-controls").hidden = false;
  if (typeof spin.animate === "function") {
    animation = spin.animate(
      [
        { transform: "rotateY(24deg) rotateZ(-23deg)" },
        { transform: "rotateY(384deg) rotateZ(-23deg)" },
      ],
      { duration: 48000, iterations: Infinity },
    );
    animation.pause();
  }
  function syncMotion() {
    const stopped = paused || reduced.matches;
    if (animation) {
      if (!stopped && visible && !document.hidden) animation.play();
      else animation.pause();
    }
    motionButton.hidden = reduced.matches || !animation;
    motionButton.textContent = paused ? "Play motion" : "Pause motion";
    motionButton.setAttribute("aria-pressed", String(paused));
    if (reduced.matches) {
      tilt.style.removeProperty("--tilt-x");
      tilt.style.removeProperty("--tilt-y");
    }
  }
  if ("IntersectionObserver" in window) {
    new IntersectionObserver(
      (entries) => {
        visible = entries[0].isIntersecting;
        syncMotion();
      },
      { threshold: 0 },
    ).observe(apparatus);
  } else visible = true;
  document.addEventListener("visibilitychange", () => {
    syncMotion();
    updateTime();
  });
  reduced.addEventListener("change", syncMotion);
  motionButton.addEventListener("click", () => {
    paused = !paused;
    syncMotion();
  });
  function syncPractice(expanded) {
    apparatus.classList.toggle("is-exploded", expanded);
    explodeButton.setAttribute("aria-pressed", String(expanded));
    explodeButton.textContent = expanded
      ? "Close the practice ↙"
      : "See the practice ↗";
  }
  explodeButton.addEventListener("click", () => {
    story.open = !story.open;
    syncPractice(story.open);
  });
  story.addEventListener("toggle", () => syncPractice(story.open));
  const lensCopy = {
    operations: "Operations: make the system legible, then keep it moving.",
    software: "Software: build small tools that earn their place through use.",
    life: "Life: leave room for curiosity, family, and the work between the work.",
  };
  lenses.forEach((lens) => {
    lens.addEventListener("click", () => {
      const active = lens.dataset.lens;
      apparatus.dataset.activeLens = active;
      summary.textContent = lensCopy[active];
      lenses.forEach((item) =>
        item.setAttribute("aria-current", String(item === lens)),
      );
    });
  });
  stage.addEventListener("pointermove", (event) => {
    if (!finePointer.matches || reduced.matches || paused) return;
    const bounds = stage.getBoundingClientRect();
    const x = (event.clientX - bounds.left) / bounds.width - 0.5;
    const y = (event.clientY - bounds.top) / bounds.height - 0.5;
    tilt.style.setProperty("--tilt-x", `${-16 - y * 18}deg`);
    tilt.style.setProperty("--tilt-y", `${-18 + x * 25}deg`);
  });
  stage.addEventListener("pointerleave", () => {
    tilt.style.removeProperty("--tilt-x");
    tilt.style.removeProperty("--tilt-y");
  });
  syncMotion();
})();
