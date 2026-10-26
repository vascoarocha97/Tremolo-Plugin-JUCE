import "./styles.css";

import { loadCurriculum, getModules } from "./curriculum";
import { startProgressPolling } from "./progress";
import { stopAll } from "./lib/audio";
import { disposeAll } from "./lib/lifecycle";
import { el } from "./lib/ui";
import { pages } from "./pages";
import { renderHome } from "./pages/home";
import { pageHeader } from "./pages/types";

const app = document.getElementById("app")!;
const sidebar = el("nav", { class: "sidebar" });
const content = el("main");
app.append(el("div", { class: "layout" }, sidebar, content));

function buildSidebar(): void {
  sidebar.replaceChildren(
    el("div", { class: "brand" }, "Learn JUCE"),
    el("div", { class: "brand-sub" }, "Teoria interativa · JUCE 9.0.4"),
    el("a", { href: "#/" }, "Início e progresso"),
  );

  for (const module of getModules()) {
    const modulePages = pages.filter((p) => p.module === module.id);
    sidebar.append(el("h3", {}, `${module.id} · ${module.title}`));

    if (modulePages.length === 0) {
      sidebar.append(el("a", { class: "disabled", href: "#/" }, "(em breve)"));
      continue;
    }

    for (const page of modulePages) sidebar.append(el("a", { href: `#/${page.id}` }, page.title));
  }
}

function route(): void {
  stopAll();
  disposeAll();
  content.replaceChildren();
  window.scrollTo(0, 0);

  const id = location.hash.replace(/^#\/?/, "");
  const page = pages.find((p) => p.id === id);

  for (const a of sidebar.querySelectorAll("a")) {
    a.classList.toggle("active", a.getAttribute("href") === `#/${id}`);
  }

  if (!page) {
    renderHome(content);
    document.title = "Learn JUCE — teoria interativa";
    return;
  }

  document.title = `${page.title} — Learn JUCE`;
  content.append(pageHeader(page));
  page.render(content);
}

async function start(): Promise<void> {
  await loadCurriculum();
  buildSidebar();
  await startProgressPolling();
  window.addEventListener("hashchange", route);
  route();
}

void start();
