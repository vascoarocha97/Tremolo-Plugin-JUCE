import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, slider, sources, callout } from "../lib/ui";
import { TWO_PI, magnitudeSpectrumDb, rampedGain } from "../lib/dsp";

export const smoothingPage: PageDef = {
  id: "smoothing",
  title: "Smoothing e clicks",
  module: 2,
  summary: "Um ganho que muda de repente é um degrau na forma de onda — ouve-se como um click. Uma rampa de alguns milissegundos resolve.",
  render(root) {
    let rampMs = 0;
    const displayRate = 48000;
    const info = readout();

    // Um seno de 200 Hz com o ganho a mudar de 1 para 0.2 a meio
    const makeSignals = (rate: number, length: number, switchAt: number) => {
      const targets = new Float32Array(length).map((_, i) => (i < switchAt ? 1 : 0.2));
      const gain = rampedGain(targets, Math.round((rampMs / 1000) * rate));
      const out = new Float32Array(length).map((_, i) => gain[i] * Math.sin((TWO_PI * 200 * i) / rate));
      return { targets, gain, out };
    };

    const wavePlot = new Plot(el("div"), {
      height: 240,
      x: { min: 0, max: 30, label: "tempo (ms)" },
      y: { min: -1.15, max: 1.15, ticks: [-1, 0, 0.2, 1] },
      draw: (pl) => {
        pl.grid();
        const length = Math.round(0.03 * displayRate);
        const { targets, gain, out } = makeSignals(displayRate, length, Math.round(0.0118 * displayRate));
        const ms = Array.from({ length }, (_, i) => (1000 * i) / displayRate);
        pl.line(ms, out, cssColor("signal"), 1.5);
        pl.line(ms, targets, cssColor("text-muted"), 1, [3, 3]);
        pl.line(ms, gain, cssColor("signal-2"), 2);
      },
    });

    const spectrumPlot = new Plot(el("div"), {
      height: 220,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -120, max: 0, ticks: [-100, -80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const n = 8192;
        const { out } = makeSignals(displayRate, n, n / 2);
        const db = magnitudeSpectrumDb(out, n);
        const freqs = Array.from({ length: n / 2 }, (_, k) => Math.max(1, (k * displayRate) / n));
        pl.line(freqs, db, cssColor("signal"), 1);
      },
    });

    const update = () => {
      const samples = Math.round((rampMs / 1000) * 48000);
      info.set(
        rampMs === 0
          ? "Rampa de <strong>0 ms</strong>: o ganho salta numa amostra. Repara no degrau na forma de onda e na energia espalhada por todo o espectro."
          : `Rampa de <strong>${rampMs.toFixed(1)} ms</strong> = <strong>${samples}</strong> amostras a 48 kHz (o passo por amostra é 0.8/${samples}).`,
      );
      wavePlot.redraw();
      spectrumPlot.redraw();
    };

    // Áudio: o ganho alterna entre 1 e 0.2 a cada 0.25 s
    const player = new LoopPlayer((rate) => {
      const length = rate * 2;
      const period = Math.round(rate * 0.25);
      const targets = new Float32Array(length).map((_, i) => (Math.floor(i / period) % 2 === 0 ? 1 : 0.2));
      const gain = rampedGain(targets, Math.round((rampMs / 1000) * rate));
      return [new Float32Array(length).map((_, i) => 0.5 * gain[i] * Math.sin((TWO_PI * 200 * i) / rate))];
    });

    root.append(
      el("h2", {}, "1. O degrau e a rampa"),
      p("Um seno de 200 Hz cujo ganho passa de 1 para 0.2 (laranja = ganho aplicado, tracejado = o valor pedido pelo parâmetro). Com 0 ms, a forma de onda tem um degrau."),
      panel(
        wavePlot.canvas,
        legend([
          { label: "sinal", color: "signal" },
          { label: "ganho com smoothing", color: "signal-2" },
          { label: "ganho pedido", color: "text-muted" },
        ]),
        controls(
          slider({ label: "Duração da rampa", min: 0, max: 20, step: 0.5, value: rampMs, format: (v) => `${v.toFixed(1)} ms`, onInput: (v) => { rampMs = v; update(); player.refresh(); } }),
          playButton(player, "Ouvir (o ganho alterna a cada 0.25 s)"),
        ),
        info,
      ),
      el("h2", {}, "2. No espectro"),
      p("Um seno puro seria uma única risca a 200 Hz. A mudança brusca de ganho espalha energia por todas as frequências (é isso que ouves como click). Com uma rampa de alguns ms, essa energia cai muitas dezenas de dB."),
      panel(spectrumPlot.canvas),
      callout("No código (exercícios 08 e 09): o smoother avança **uma vez por amostra** e todos os canais usam o mesmo valor. Os parâmetros chegam de outra thread: lê-os de um `std::atomic` uma vez por bloco e passa-os ao `SmoothedValue` dentro do `process`."),
      sources([
        { title: "juce::SmoothedValue — documentação", url: "https://docs.juce.com/master/classjuce_1_1SmoothedValue.html", why: "smoothing linear e multiplicativo" },
        { title: "Tutorial JUCE: Control audio levels", url: "https://juce.com/tutorials/tutorial_synth_level_control/", why: "o problema do ganho aplicado bloco a bloco" },
      ]),
    );

    update();
  },
};
