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
  if (time) setInterval(updateTime, 60000);
  const apparatus = document.querySelector(".apparatus");
  if (!apparatus) return;
  const stage = apparatus.querySelector(".apparatus-stage");
  const spin = apparatus.querySelector(".apparatus-spin");
  const tilt = apparatus.querySelector(".apparatus-tilt");
  const motionButton = document.getElementById("motion-toggle");
  const explodeButton = document.getElementById("apparatus-explode");
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
  explodeButton.addEventListener("click", () => {
    const expanded = apparatus.classList.toggle("is-exploded");
    explodeButton.setAttribute("aria-pressed", String(expanded));
    explodeButton.textContent = expanded ? "Reassemble ↙" : "Disassemble ↗";
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
