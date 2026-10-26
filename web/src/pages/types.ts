import { el, exerciseChips, rich } from "../lib/ui";
import { exercisesForPage } from "../curriculum";

export interface PageDef {
  /** Usado no URL: #/<id>. É o valor "web" de cada exercício no curriculum.json. */
  id: string;
  title: string;
  module: number;
  summary: string;
  render: (root: HTMLElement) => void;
}

/** Cabeçalho comum: título, resumo e os exercícios que usam esta página. */
export function pageHeader(page: PageDef): HTMLElement {
  const exercises = exercisesForPage(page.id).map((e) => e.id);
  return el(
    "header",
    {},
    el("h1", {}, page.title),
    el("div", { class: "subtitle" }, rich(page.summary)),
    exercises.length > 0 ? exerciseChips(exercises) : null,
  );
}
