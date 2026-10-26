// Componentes de interface pequenos (DOM direto, sem framework).

import { LoopPlayer, track } from "./audio";
import { exerciseStatus, onProgress, type ExerciseStatus } from "../progress";
import { findExercise } from "../curriculum";
import { onDispose } from "./lifecycle";

type Child = Node | string | null | undefined | false;

export function el<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  props: Partial<Record<string, string>> = {},
  ...children: Child[]
): HTMLElementTagNameMap[K] {
  const element = document.createElement(tag);
  for (const [key, value] of Object.entries(props)) {
    if (value === undefined) continue;
    if (key === "class") element.className = value;
    else if (key === "html") element.innerHTML = value;
    else element.setAttribute(key, value);
  }
  for (const child of children) {
    if (child === null || child === undefined || child === false) continue;
    element.append(typeof child === "string" ? document.createTextNode(child) : child);
  }
  return element;
}

/** Texto com marcação mínima: `código` e **negrito**. */
export function rich(text: string): HTMLElement {
  const span = el("span");
  const escaped = text.replace(/&/g, "&amp;").replace(/</g, "&lt;");
  span.innerHTML = escaped.replace(/`([^`]+)`/g, "<code>$1</code>").replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
  return span;
}

export function p(text: string): HTMLParagraphElement {
  return el("p", {}, rich(text));
}

export function callout(text: string, kind: "info" | "warn" = "info"): HTMLElement {
  return el("div", { class: kind === "warn" ? "callout warn" : "callout" }, rich(text));
}

export function panel(...children: Child[]): HTMLElement {
  return el("div", { class: "panel" }, ...children);
}

export function controls(...children: Child[]): HTMLElement {
  return el("div", { class: "controls" }, ...children);
}

export interface SliderOptions {
  label: string;
  min: number;
  max: number;
  step?: number;
  value: number;
  log?: boolean;
  format?: (value: number) => string;
  onInput: (value: number) => void;
}

/** Slider com etiqueta e valor. Com log: true, a posição é logarítmica (frequências). */
export function slider(options: SliderOptions): HTMLElement & { setValue: (v: number) => void } {
  const { label, min, max, log = false } = options;
  const format = options.format ?? ((v: number) => `${+v.toFixed(2)}`);
  const input = el("input", { type: "range", min: "0", max: "1000", step: "1" });
  const value = el("span", { class: "value" });

  const toPosition = (v: number) =>
    log ? (Math.log(v / min) / Math.log(max / min)) * 1000 : ((v - min) / (max - min)) * 1000;
  const fromPosition = (pos: number) => {
    const t = pos / 1000;
    let v = log ? min * Math.pow(max / min, t) : min + t * (max - min);
    if (options.step) v = Math.round(v / options.step) * options.step;
    return Math.min(max, Math.max(min, v));
  };

  const set = (v: number) => {
    input.value = String(toPosition(v));
    value.textContent = format(v);
  };

  input.addEventListener("input", () => {
    const v = fromPosition(Number(input.value));
    value.textContent = format(v);
    options.onInput(v);
  });

  set(options.value);
  return Object.assign(el("div", { class: "control" }, el("label", {}, label), input, value), { setValue: set });
}

export function select<T extends string>(
  label: string,
  options: Array<{ value: T; label: string }>,
  value: T,
  onChange: (value: T) => void,
): HTMLElement {
  const s = el("select");
  for (const o of options) {
    const option = el("option", { value: o.value }, o.label);
    if (o.value === value) option.selected = true;
    s.append(option);
  }
  s.addEventListener("change", () => onChange(s.value as T));
  return el("div", { class: "control" }, el("label", {}, label), s);
}

export function toggle(label: string, value: boolean, onChange: (value: boolean) => void): HTMLElement {
  const input = el("input", { type: "checkbox" });
  input.checked = value;
  input.addEventListener("change", () => onChange(input.checked));
  return el("label", { class: "control" }, input, label);
}

export function button(label: string, onClick: () => void): HTMLButtonElement {
  const b = el("button", {}, label);
  b.addEventListener("click", onClick);
  return b;
}

/** Botão ▶/■ ligado a um LoopPlayer. O player é parado automaticamente ao sair da página. */
export function playButton(player: LoopPlayer, label = "Ouvir"): HTMLButtonElement {
  track(player);
  const b = el("button", {}, `▶ ${label}`);
  const update = (playing: boolean) => {
    b.textContent = playing ? `■ Parar` : `▶ ${label}`;
    b.classList.toggle("playing", playing);
  };
  player.onChange(update);
  b.addEventListener("click", () => player.toggle());
  return b;
}

export function readout(): HTMLElement & { set: (html: string) => void } {
  const r = el("div", { class: "readout" });
  return Object.assign(r, {
    set: (html: string) => {
      r.innerHTML = html;
    },
  });
}

export function legend(items: Array<{ label: string; color: string }>): HTMLElement {
  return el(
    "div",
    { class: "legend" },
    ...items.map((i) => el("span", { style: `--c: var(--${i.color})` }, i.label)),
  );
}

export interface Source {
  title: string;
  url: string;
  why: string;
}

export function sources(list: Source[]): HTMLElement {
  return el(
    "div",
    { class: "sources" },
    el("h2", {}, "Fontes"),
    el(
      "ul",
      {},
      ...list.map((s) =>
        el("li", {}, el("a", { href: s.url, target: "_blank", rel: "noopener" }, s.title), " — ", el("span", { class: "why" }, s.why)),
      ),
    ),
  );
}

const statusLabel: Record<ExerciseStatus, string> = {
  done: "✔ feito",
  in_progress: "… em curso",
  not_started: "por começar",
};

/** Chips "Exercício NN · título · estado", atualizados quando o progress.json muda. */
export function exerciseChips(ids: string[]): HTMLElement {
  const container = el("div", { class: "chips" });

  const render = () => {
    container.replaceChildren(
      ...ids.map((id) => {
        const ex = findExercise(id);
        const status = exerciseStatus(id);
        return el(
          "span",
          { class: `chip ${status}`, title: ex ? `exercises/${ex.folder}/README.md` : "" },
          `Exercício ${id}${ex ? " · " + ex.title : ""} · ${statusLabel[status]}`,
        );
      }),
    );
  };

  render();
  onDispose(onProgress(render));
  return container;
}
