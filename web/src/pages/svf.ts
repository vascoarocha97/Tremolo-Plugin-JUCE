// ⚠ SPOILER: este ficheiro implementa em TypeScript a matemática de um ou mais exercícios C++.
//   Ler o código antes de fazeres o exercício equivale a ver a solução; usa a página no browser.
import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, gainToDb, processSvf, type SvfMode } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;

/** Resposta do SVF TPT: protótipo analógico com ω pré-distorcido (o bilinear transform mapeia fc exatamente). */
function svfMagnitudes(f: number, fc: number, q: number): [number, number, number] {
  // ω analógico equivalente (normalizado por ωc) depois da pré-distorção
  const g = Math.tan((Math.PI * fc) / fs);
  const w = Math.tan((Math.PI * Math.min(f, fs * 0.4999)) / fs) / g;
  const R2 = 1 / q;
  // denominador: 1 − w² + j·R2·w
  const den = Math.hypot(1 - w * w, R2 * w);
  return [1 / den, w / den, (w * w) / den];
}

export const svfPage: PageDef = {
  id: "svf",
  title: "State Variable Filter",
  module: 4,
  summary: "Dois integradores com feedback: passa-baixo, passa-banda e passa-alto ao mesmo tempo, estável com modulação rápida.",
  render(root) {
    let fc = 1000;
    let q = 2;
    const info = readout();

    const response = new Plot(el("div"), {
      height: 260,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -48, max: 24, ticks: [-48, -36, -24, -12, -3, 0, 12, 24], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        pl.fn((f) => gainToDb(svfMagnitudes(f, fc, q)[0]), cssColor("signal"), 2, 800);
        pl.fn((f) => gainToDb(svfMagnitudes(f, fc, q)[1]), cssColor("signal-3"), 2, 800);
        pl.fn((f) => gainToDb(svfMagnitudes(f, fc, q)[2]), cssColor("signal-2"), 2, 800);
        pl.vline(fc, cssColor("text-muted"));
      },
    });

    const update = () => {
      const g = Math.tan((Math.PI * fc) / fs);
      info.set(
        `g = tan(π·fc/fs) = <strong>${g.toFixed(4)}</strong> · R2 = 1/Q = <strong>${(1 / q).toFixed(3)}</strong> · ` +
          `h = 1/(1 + R2·g + g²) = <strong>${(1 / (1 + g / q + g * g)).toFixed(4)}</strong> · em fc o BP vale Q = ${gainToDb(q).toFixed(1)} dB`,
      );
      response.redraw();
    };

    // Áudio: corte modulado por LFO, por amostra
    let kind: SourceKind = "saw";
    let mode: SvfMode = "lowPass";
    let audioCutoff = 600;
    let audioQ = 6;
    let depth = 2.5;
    let rate = 1.5;
    const player = new LoopPlayer((sampleRate) => {
      const input = renderSource(kind, sampleRate, 0.3);
      return input.map((ch) => processSvf(ch, sampleRate, audioQ, mode, (n) => audioCutoff * Math.pow(2, depth * Math.sin((TWO_PI * rate * n) / sampleRate))));
    });

    root.append(
      el("h2", {}, "1. Três saídas do mesmo filtro"),
      p("Com Q alto aparece um pico de ressonância em fc no LP e no HP; o BP (não normalizado, como no JUCE) vale exatamente Q em fc. Em todas as frequências, `x = HP + R2·BP + LP`."),
      panel(
        response.canvas,
        legend([
          { label: "passa-baixo", color: "signal" },
          { label: "passa-banda", color: "signal-3" },
          { label: "passa-alto", color: "signal-2" },
        ]),
        controls(
          slider({ label: "Corte", min: 20, max: 20000, value: fc, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { fc = v; update(); } }),
          slider({ label: "Q", min: 0.3, max: 20, value: q, log: true, format: (v) => v.toFixed(2), onInput: (v) => { q = v; update(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. Modulação por amostra"),
      p("Um LFO move o corte ± várias oitavas, recalculando `g` e `h` em **cada amostra**. Os estados do SVF TPT têm significado físico (são as saídas dos integradores), por isso não \"partem\" quando os coeficientes mudam."),
      panel(
        controls(
          sourceSelect(kind, (k) => { kind = k; player.refresh(); }),
          select<SvfMode>("Saída", [ { value: "lowPass", label: "passa-baixo" }, { value: "bandPass", label: "passa-banda" }, { value: "highPass", label: "passa-alto" } ], mode, (v) => { mode = v; player.refresh(); }),
          slider({ label: "Corte", min: 50, max: 8000, value: audioCutoff, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { audioCutoff = v; player.refresh(); } }),
          slider({ label: "Q", min: 0.5, max: 20, value: audioQ, log: true, format: (v) => v.toFixed(1), onInput: (v) => { audioQ = v; player.refresh(); } }),
          slider({ label: "Profundidade", min: 0, max: 4, step: 0.1, value: depth, format: (v) => `${v.toFixed(1)} oitavas`, onInput: (v) => { depth = v; player.refresh(); } }),
          slider({ label: "LFO", min: 0.1, max: 12, value: rate, log: true, format: (v) => `${v.toFixed(2)} Hz`, onInput: (v) => { rate = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("O cálculo (exercício 16): `HP = h·(x − s1·(g + R2) − s2)`, depois os dois integradores trapezoidais. É o mesmo que o `juce::dsp::StateVariableTPTFilter`."),
      sources([
        { title: "V. Zavalishin — The Art of VA Filter Design", url: "https://www.discodsp.net/VAFilterDesign_2.1.2.pdf", why: "TPT, ciclos sem atraso e o SVF" },
        { title: "A. Simper (Cytomic) — Linear Trapezoidal Integrated SVF", url: "https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf", why: "a versão otimizada" },
        { title: "juce::dsp::StateVariableTPTFilter", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1StateVariableTPTFilter.html", why: "a implementação do JUCE" },
      ]),
    );

    update();
  },
};
