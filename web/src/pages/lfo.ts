import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { animate } from "../lib/lifecycle";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { lfoValue, type LfoShape } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const shapes: Array<{ value: LfoShape; label: string }> = [
  { value: "sine", label: "seno" },
  { value: "triangle", label: "triângulo" },
  { value: "saw", label: "dente de serra" },
  { value: "square", label: "quadrada" },
];

export const lfoPage: PageDef = {
  id: "lfo",
  title: "LFO e modulação",
  module: 5,
  summary: "Um oscilador lento que mexe noutros parâmetros. Aqui: o volume (tremolo) e a posição estéreo.",
  render(root) {
    let shape: LfoShape = "sine";
    let depth = 0.8;
    let stereoDeg = 0;
    let rate = 2;
    let now = 0;
    let last = 0;
    const info = readout();

    const shapesPlot = new Plot(el("div"), {
      height: 220,
      x: { min: 0, max: 2, label: "ciclos", ticks: [0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2] },
      y: { min: -1.2, max: 1.2, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        const colours = ["signal", "signal-2", "signal-3", "signal-4"];
        shapes.forEach((s, i) => pl.fn((x) => lfoValue(s.value, x), cssColor(colours[i]), s.value === shape ? 2.6 : 1, 800));
      },
    });

    const gainPlot = new Plot(el("div"), {
      height: 220,
      x: { min: 0, max: 2, label: "tempo (s)" },
      y: { min: 0, max: 1.1, ticks: [0, 0.5, 1], label: "ganho" },
      draw: (pl) => {
        pl.grid();
        const offset = stereoDeg / 360;
        pl.fn((t) => 1 - depth * 0.5 * (lfoValue(shape, rate * t) + 1), cssColor("signal"), 2, 800);
        pl.fn((t) => 1 - depth * 0.5 * (lfoValue(shape, rate * t + offset) + 1), cssColor("signal-2"), 2, 800, [6, 4]);
        pl.vline(now % 2, cssColor("text-muted"), [], 1);
      },
    });

    const update = () => {
      info.set(
        `ganho entre <strong>${(1 - depth).toFixed(2)}</strong> e <strong>1</strong> · período <strong>${(1000 / rate).toFixed(0)} ms</strong>` +
          (stereoDeg > 0 ? ` · direita desfasada <strong>${stereoDeg.toFixed(0)}°</strong>` : ""),
      );
      shapesPlot.redraw();
      gainPlot.redraw();
    };

    animate((t) => {
      if (last) now += t - last;
      last = t;
      gainPlot.redraw();
    });

    let kind: SourceKind = "loop";
    const player = new LoopPlayer((sampleRate) => {
      const input = renderSource(kind, sampleRate, 0.6);
      const offset = stereoDeg / 360;
      return input.map((ch, c) => ch.map((v, n) => v * (1 - depth * 0.5 * (lfoValue(shape, (rate * n) / sampleRate + (c === 1 ? offset : 0)) + 1))));
    });

    root.append(
      el("h2", {}, "1. As quatro formas"),
      p("Todas alinhadas com o seno: começam em 0 e sobem (a quadrada começa em +1). Assim, trocar de forma não desloca o ciclo no tempo."),
      panel(
        shapesPlot.canvas,
        legend([
          { label: "seno", color: "signal" },
          { label: "triângulo", color: "signal-2" },
          { label: "dente de serra", color: "signal-3" },
          { label: "quadrada", color: "signal-4" },
        ]),
      ),
      el("h2", {}, "2. Tremolo: o LFO no ganho"),
      p("`ganho = 1 − profundidade · (LFO + 1)/2`. A tracejado, o canal direito com o desfasamento estéreo: a 180° quando um sobe o outro desce."),
      panel(
        gainPlot.canvas,
        legend([ { label: "esquerda", color: "signal" }, { label: "direita", color: "signal-2" } ]),
        controls(
          select<LfoShape>("Forma", shapes, shape, (v) => { shape = v; update(); player.refresh(); }),
          slider({ label: "Velocidade", min: 0.1, max: 20, value: rate, log: true, format: (v) => `${v.toFixed(2)} Hz`, onInput: (v) => { rate = v; update(); player.refresh(); } }),
          slider({ label: "Profundidade", min: 0, max: 1, step: 0.01, value: depth, format: (v) => v.toFixed(2), onInput: (v) => { depth = v; update(); player.refresh(); } }),
          slider({ label: "Fase estéreo", min: 0, max: 180, step: 1, value: stereoDeg, format: (v) => `${v}°`, onInput: (v) => { stereoDeg = v; update(); player.refresh(); } }),
          sourceSelect(kind, (k) => { kind = k; player.refresh(); }, ["loop", "saw", "noise"]),
          playButton(player, "Ouvir"),
        ),
        info,
      ),
      callout("Com a forma quadrada ouves clicks nas transições: um degrau de ganho é um degrau na forma de onda (exercício 08). Acima de ~20 Hz a modulação deixa de soar a pulsação e passa a soar a um timbre novo (bandas laterais)."),
      sources([
        { title: "J. O. Smith — Sinusoidal Amplitude Modulation", url: "https://ccrma.stanford.edu/~jos/mdft/Sinusoidal_Amplitude_Modulation_AM.html", why: "bandas laterais da modulação de amplitude" },
        { title: "juce::dsp::Oscillator", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1Oscillator.html", why: "o oscilador do JUCE, que também serve de LFO" },
      ]),
    );

    update();
  },
};
