import { getModules, allExercises } from "../curriculum";
import { exerciseProgress, exerciseStatus, getProgress, onProgress } from "../progress";
import { onDispose } from "../lib/lifecycle";
import { el, p, rich } from "../lib/ui";
import { pages } from ".";

const statusText = { done: "✔ feito", in_progress: "… em curso", not_started: "· por começar" } as const;

export function renderHome(root: HTMLElement): void {
  const summary = el("div");
  const table = el("div");

  root.append(
    el("h1", {}, "Learn JUCE"),
    el("div", { class: "subtitle" }, "Teoria interativa para os 40 exercícios do curso. Uma página por conceito, com som."),
    el(
      "div",
      { class: "panel" },
      p("**Como usar:** lê a lição do exercício (`exercises/NN-nome/README.md`), abre a página de teoria indicada, completa os `TODO` e corre `./learn check NN`. Este ecrã atualiza sozinho com o resultado (lê o `progress.json` a cada 5 segundos)."),
      p("Os botões **▶ Ouvir** tocam áudio gerado no browser. O volume geral é baixo e há um limitador, mas começa com o volume do sistema baixo."),
      summary,
    ),
    table,
  );

  const render = () => {
    const exercises = allExercises();
    const done = exercises.filter((e) => exerciseStatus(e.id) === "done").length;
    const inProgress = exercises.filter((e) => exerciseStatus(e.id) === "in_progress").length;
    const percent = exercises.length ? (100 * done) / exercises.length : 0;
    const generatedAt = getProgress().generatedAt;

    summary.replaceChildren(
      el("div", { class: "progress-bar" }, el("div", { style: `width: ${percent}%` })),
      el(
        "div",
        { class: "readout" },
        rich(
          `**${done}** de ${exercises.length} exercícios feitos · **${inProgress}** em curso` +
            (generatedAt ? ` · último ./learn check: ${new Date(generatedAt).toLocaleString("pt-PT")}` : " · ainda não correste nenhum ./learn check"),
        ),
      ),
    );

    table.replaceChildren();

    for (const module of getModules()) {
      const rows = module.exercises.map((ex) => {
        const status = exerciseStatus(ex.id);
        const progress = exerciseProgress(ex.id);
        const page = pages.find((pg) => pg.id === ex.web);
        const score = progress && progress.total > 0 ? ` ${progress.passed}/${progress.total}` : "";

        return el(
          "tr",
          {},
          el("td", { class: "id" }, ex.id),
          el(
            "td",
            {},
            el("div", {}, el("strong", {}, ex.title), ` · ${ex.minutes} min`),
            el("div", { class: "summary" }, ex.summary),
            el("div", { class: "summary" }, rich(`\`./learn check ${ex.id}\` · \`exercises/${ex.folder}/README.md\``)),
          ),
          el("td", {}, page ? el("a", { href: `#/${page.id}` }, page.title) : el("span", { class: "summary" }, "—")),
          el("td", { class: `status status-${status}` }, statusText[status] + score),
        );
      });

      table.append(
        el("h2", {}, `Módulo ${module.id} — ${module.title}`),
        el("div", { class: "panel" }, el("table", { class: "exercises" }, el("tbody", {}, ...rows))),
      );
    }
  };

  render();
  onDispose(onProgress(render));
}
