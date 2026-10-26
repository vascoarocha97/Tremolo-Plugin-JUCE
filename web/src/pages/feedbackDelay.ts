// ⚠ SPOILER: este ficheiro implementa em TypeScript a matemática de um ou mais exercícios C++.
//   Ler o código antes de fazeres o exercício equivale a ver a solução; usa a página no browser.
import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, demoLoop, gainToDb } from "../lib/dsp";

type Mode = "echo" | "pingpong";

/** Eco com feedback ou ping-pong, aplicado a um loop.
    Processa o loop duas vezes e devolve a segunda passagem: assim a cauda dos ecos continua no
    início do loop e a repetição não tem cortes. */
function processLoop(input: Float32Array[], rate: number, delayMs: number, feedback: number, mix: number, mode: Mode): Float32Array[] {
  const d = Math.max(1, Math.round((delayMs / 1000) * rate));
  const size = d; // ler a posição prestes a ser reescrita dá a amostra de há exatamente d passos
  const lines = [new Float32Array(size), new Float32Array(size)];
  let w = 0;
  const length = input[0].length;
  const out = [new Float32Array(length), new Float32Array(length)];

  for (let pass = 0; pass < 2; pass++) {
    for (let i = 0; i < length; i++) {
      const r = (w + 1) % size; // a posição mais antiga = atraso de d amostras
      const fromL = lines[0][r];
      const fromR = lines[1][r];
      const inL = input[0][i];
      const inR = input[1 % input.length][i];

      if (mode === "echo") {
        lines[0][r] = inL + feedback * fromL;
        lines[1][r] = inR + feedback * fromR;
      } else {
        lines[0][r] = 0.5 * (inL + inR) + feedback * fromR;
        lines[1][r] = feedback * fromL;
      }
      w = r;

      if (pass === 1) {
        out[0][i] = (1 - mix) * inL + mix * fromL;
        out[1][i] = (1 - mix) * inR + mix * fromR;
      }
    }
  }
  return out;
}

export const feedbackDelayPage: PageDef = {
  id: "feedback-delay",
  title: "Delay com feedback",
  module: 3,
  summary: "Parte da saída volta a entrar: ecos em D, 2D, 3D... com amplitudes fb, fb², fb³. Também é um filtro (pente).",
  render(root) {
    let delayMs = 5;
    let feedback = 0.6;
    const info = readout();

    const impulse = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: 50, label: "tempo (ms)" },
      y: { min: 0, max: 1.05, ticks: [0, 0.25, 0.5, 0.75, 1] },
      draw: (pl) => {
        pl.grid();
        const xs: number[] = [];
        const ys: number[] = [];
        for (let k = 1; k * delayMs <= 50; k++) {
          xs.push(k * delayMs);
          ys.push(Math.pow(feedback, k - 1));
        }
        pl.stems(xs, ys, cssColor("signal"), 3);
      },
    });

    const comb = new Plot(el("div"), {
      height: 220,
      x: { min: 0, max: 2000, label: "frequência (Hz)" },
      y: { min: -12, max: 24, ticks: [-12, -6, 0, 6, 12, 18, 24], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const D = delayMs / 1000;
        // H(f) = e^{-jωD} / (1 - fb·e^{-jωD})  → |H| = 1 / |1 - fb·e^{-jωD}|
        pl.fn((f) => {
          const w = TWO_PI * f * D;
          return gainToDb(1 / Math.hypot(1 - feedback * Math.cos(w), feedback * Math.sin(w)));
        }, cssColor("signal-2"), 1.6, 2000);
      },
    });

    const update = () => {
      const peakDb = gainToDb(1 / (1 - feedback));
      const notchDb = gainToDb(1 / (1 + feedback));
      info.set(
        `ecos a cada <strong>${delayMs.toFixed(1)} ms</strong> · picos do pente a cada <strong>${(1000 / delayMs).toFixed(0)} Hz</strong> · ` +
          `picos de <strong>${peakDb.toFixed(1)} dB</strong> e vales de <strong>${notchDb.toFixed(1)} dB</strong> · ` +
          `a cauda cai 60 dB ao fim de <strong>${((delayMs * Math.log(0.001)) / Math.log(Math.max(feedback, 1e-6))).toFixed(0)} ms</strong>`,
      );
      impulse.redraw();
      comb.redraw();
    };

    // Áudio
    let audioDelayMs = 350;
    let audioFeedback = 0.45;
    let mix = 0.4;
    let mode: Mode = "echo";
    let loopCache: { rate: number; data: Float32Array[] } | null = null;
    const player = new LoopPlayer((rate) => {
      if (!loopCache || loopCache.rate !== rate) loopCache = { rate, data: demoLoop(rate).map((ch) => ch.map((v) => v * 0.55)) };
      return processLoop(loopCache.data, rate, audioDelayMs, audioFeedback, mix, mode);
    });

    root.append(
      el("h2", {}, "1. Resposta a um impulso"),
      p("Um impulso à entrada (com mix = 1) dá ecos em D, 2D, 3D, ... cada um multiplicado por fb. Se fb ≥ 1 os ecos nunca acabam ou crescem: por isso o exercício 12 limita o feedback a 0.95."),
      panel(
        impulse.canvas,
        controls(
          slider({ label: "Atraso", min: 1, max: 20, step: 0.5, value: delayMs, format: (v) => `${v.toFixed(1)} ms`, onInput: (v) => { delayMs = v; update(); } }),
          slider({ label: "Feedback", min: 0, max: 0.95, step: 0.01, value: feedback, format: (v) => v.toFixed(2), onInput: (v) => { feedback = v; update(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. O mesmo sistema visto em frequência: um filtro pente"),
      p("Com atrasos curtos (alguns ms), os ecos já não se ouvem como ecos: ouve-se um timbre metálico. A resposta em frequência tem picos a cada 1/D Hz — o **filtro pente com feedback** que vais usar no flanger (21) e nas reverbs (27–28)."),
      panel(comb.canvas, legend([{ label: "|H(f)| = 1 / |1 − fb·e^(−jωD)|", color: "signal-2" }])),
      el("h2", {}, "3. Ouvir"),
      panel(
        controls(
          select<Mode>("Modo", [ { value: "echo", label: "eco (exercício 12)" }, { value: "pingpong", label: "ping-pong (exercício 13)" } ], mode, (v) => { mode = v; player.refresh(); }),
          slider({ label: "Atraso", min: 20, max: 800, step: 1, value: audioDelayMs, format: (v) => `${v} ms`, onInput: (v) => { audioDelayMs = v; player.refresh(); } }),
          slider({ label: "Feedback", min: 0, max: 0.95, step: 0.01, value: audioFeedback, format: (v) => v.toFixed(2), onInput: (v) => { audioFeedback = v; player.refresh(); } }),
          slider({ label: "Mistura", min: 0, max: 1, step: 0.01, value: mix, format: (v) => v.toFixed(2), onInput: (v) => { mix = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Ordem dentro do ciclo: **ler** o eco, **escrever** entrada + fb·eco, **misturar**. Escrever antes de ler encurta o atraso uma amostra."),
      sources([
        { title: "J. O. Smith — PASP: Feedback Comb Filters", url: "https://ccrma.stanford.edu/~jos/pasp/Feedback_Comb_Filters.html", why: "o eco com feedback como filtro e a sua estabilidade" },
        { title: "juce::dsp::DelayLine — documentação", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html", why: "pushSample/popSample, usados nos exercícios 12 e 13" },
        { title: "juce::dsp::DryWetMixer — documentação", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1DryWetMixer.html", why: "misturas seco/molhado com várias leis" },
      ]),
    );

    update();
  },
};
