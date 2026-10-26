// ⚠ SPOILER: este ficheiro implementa em TypeScript a matemática de um ou mais exercícios C++.
//   Ler o código antes de fazeres o exercício equivale a ver a solução; usa a página no browser.
import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, toggle, callout } from "../lib/ui";
import { demoLoop, gainToDb, noise } from "../lib/dsp";

type Law = "linear" | "constantPower" | "compromise";

function gains(pan: number, law: Law): [number, number] {
  const quarterPi = Math.PI / 4;
  const l = Math.sin((1 - pan) * quarterPi);
  const r = Math.sin((1 + pan) * quarterPi);
  if (law === "linear") return [(1 - pan) / 2, (1 + pan) / 2];
  if (law === "constantPower") return [l, r];
  return [Math.pow(l, 1.5), Math.pow(r, 1.5)];
}

export const panLawsPage: PageDef = {
  id: "pan-laws",
  title: "Leis de pan",
  module: 2,
  summary: "Linear (−6 dB no centro), potência constante (−3 dB) e compromisso (−4.5 dB): o que cada uma conserva.",
  render(root) {
    let law: Law = "constantPower";
    let pan = 0;
    let sweep = false;
    const info = readout();

    const gainPlot = new Plot(el("div"), {
      height: 240,
      x: { min: -1, max: 1, label: "pan", ticks: [-1, -0.5, 0, 0.5, 1], format: (v) => (v === -1 ? "L" : v === 1 ? "R" : v.toString()) },
      y: { min: 0, max: 1.05, label: "ganho", ticks: [0, 0.5, 0.707, 1] },
      draw: (pl) => {
        pl.grid();
        pl.fn((x) => gains(x, law)[0], cssColor("signal"), 2);
        pl.fn((x) => gains(x, law)[1], cssColor("signal-2"), 2);
        pl.vline(pan, cssColor("text-muted"), [4, 3]);
      },
    });

    const levelPlot = new Plot(el("div"), {
      height: 200,
      x: { min: -1, max: 1, label: "pan", ticks: [-1, -0.5, 0, 0.5, 1] },
      y: { min: -9, max: 3, label: "dB", ticks: [-6, -3, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        pl.fn((x) => { const [l, r] = gains(x, law); return gainToDb(Math.sqrt(l * l + r * r)); }, cssColor("signal-3"), 2);
        pl.fn((x) => { const [l, r] = gains(x, law); return gainToDb(l + r); }, cssColor("signal-4"), 2, 600, [6, 4]);
      },
    });

    const update = () => {
      const [l, r] = gains(pan, law);
      info.set(
        `L = <strong>${l.toFixed(3)}</strong> (${gainToDb(l).toFixed(1)} dB) · R = <strong>${r.toFixed(3)}</strong> (${gainToDb(r).toFixed(1)} dB) · ` +
          `potência L²+R² = <strong>${(l * l + r * r).toFixed(3)}</strong> · soma mono L+R = <strong>${(l + r).toFixed(3)}</strong>`,
      );
      gainPlot.redraw();
      levelPlot.redraw();
    };

    let source: "loop" | "noise" = "loop";
    let loopCache: { rate: number; data: Float32Array[] } | null = null;
    const player = new LoopPlayer((rate) => {
      let mono: Float32Array;
      if (source === "noise") {
        mono = noise(rate * 4, 0.3, 5);
      } else {
        if (!loopCache || loopCache.rate !== rate) loopCache = { rate, data: demoLoop(rate) };
        mono = loopCache.data[0].map((v, i) => 0.5 * (v + loopCache!.data[1][i]) * 0.6);
      }
      const left = new Float32Array(mono.length);
      const right = new Float32Array(mono.length);
      for (let i = 0; i < mono.length; i++) {
        // Em modo varrimento, o pan vai de L a R e volta durante o loop
        const position = sweep ? Math.sin((2 * Math.PI * i) / mono.length) : pan;
        const [l, r] = gains(position, law);
        left[i] = l * mono[i];
        right[i] = r * mono[i];
      }
      return [left, right];
    });

    root.append(
      el("h2", {}, "1. Ganhos de cada lado"),
      panel(
        gainPlot.canvas,
        legend([
          { label: "esquerda (L)", color: "signal" },
          { label: "direita (R)", color: "signal-2" },
        ]),
        controls(
          select<Law>("Lei", [
            { value: "linear", label: "linear (−6 dB)" },
            { value: "constantPower", label: "potência constante (−3 dB)" },
            { value: "compromise", label: "compromisso (−4.5 dB)" },
          ], law, (v) => { law = v; update(); player.refresh(); }),
          slider({ label: "Pan", min: -1, max: 1, step: 0.01, value: pan, format: (v) => v.toFixed(2), onInput: (v) => { pan = v; update(); player.refresh(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. O que se conserva"),
      p("Verde: o nível em **potência** (o que ouvimos em estéreo). Roxo tracejado: o nível se L e R forem **somados a mono**. Nenhuma lei mantém as duas linhas planas ao mesmo tempo — daí haver um compromisso."),
      panel(
        levelPlot.canvas,
        legend([
          { label: "potência √(L²+R²)", color: "signal-3" },
          { label: "soma mono L+R", color: "signal-4" },
        ]),
      ),
      el("h2", {}, "3. Ouvir (com auscultadores)"),
      panel(
        controls(
          select("Fonte", [ { value: "loop", label: "loop (somado a mono)" }, { value: "noise", label: "ruído" } ], source, (v) => { source = v; player.refresh(); }),
          toggle("Varrimento L → R → L", sweep, (v) => { sweep = v; player.refresh(); }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Com a lei linear e o varrimento ligado, ouve o som \"afundar\" quando passa pelo centro. Com potência constante, o nível percebido mantém-se."),
      callout("Armadilha (exercício 07): em `float`, `cos(π/2)` dá −4.4·10⁻⁸ e `pow` de um número negativo com expoente 1.5 dá NaN. Esta página usa dois senos, que dão 0 exato nos extremos.", "warn"),
      sources([
        { title: "CMU — Loudness Concepts & Panning Laws", url: "https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/", why: "as três leis e quando usar cada uma" },
        { title: "juce::dsp::Panner — documentação", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1Panner.html", why: "as regras do JUCE (que normalizam o centro para 0 dB)" },
      ]),
    );

    update();
  },
};
