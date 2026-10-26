import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, aliasedFrequency, bandlimitedSaw, magnitudeSpectrumDb, naiveSaw } from "../lib/dsp";

export const nyquistPage: PageDef = {
  id: "nyquist-aliasing",
  title: "Nyquist e aliasing",
  module: 1,
  summary: "Acima de fs/2 as frequências \"dobram\" para baixo. Um oscilador ingénuo gera harmónicos que fazem exatamente isso.",
  render(root) {
    // ---- 1. Mesmas amostras, frequências diferentes -------------------------------------------
    let fs = 8000;
    let f = 1000;
    const info = readout();

    const timePlot = new Plot(el("div"), {
      height: 240,
      x: { min: 0, max: 3, label: "tempo (ms)" },
      y: { min: -1.2, max: 1.2, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        const alias = aliasedFrequency(f, fs);
        // Sinal do alias: para f entre fs/2 e fs, as amostras de sin(2πf t) = -sin(2π(fs-f) t)
        const folded = Math.abs(f) % fs;
        const sign = folded > fs / 2 ? -1 : 1;
        pl.fn((t) => Math.sin((TWO_PI * f * t) / 1000), cssColor("text-muted"), 1, 1500);
        pl.fn((t) => sign * Math.sin((TWO_PI * alias * t) / 1000), cssColor("signal-2"), 2, 600, [6, 4]);
        const xs: number[] = [];
        const ys: number[] = [];
        for (let n = 0; (n * 1000) / fs <= 3; n++) {
          xs.push((n * 1000) / fs);
          ys.push(Math.sin((TWO_PI * f * n) / fs));
        }
        pl.stems(xs, ys, cssColor("signal"), 3.5);
      },
    });

    const foldPlot = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: 20000, label: "frequência do sinal (Hz)" },
      y: { min: 0, max: 5000, label: "frequência ouvida (Hz)" },
      draw: (pl) => {
        pl.x = { min: 0, max: 2.5 * fs, label: "frequência do sinal (Hz)", format: formatHz };
        pl.y = { min: 0, max: fs / 2, label: "frequência ouvida (Hz)", format: formatHz };
        pl.grid();
        for (let k = 1; k * fs / 2 <= 2.5 * fs; k++) pl.vline((k * fs) / 2, cssColor("text-muted"), [2, 4]);
        pl.fn((x) => aliasedFrequency(x, fs), cssColor("signal"), 2, 1000);
        pl.vline(f, cssColor("signal-2"), [], 1.5);
        pl.hline(aliasedFrequency(f, fs), cssColor("signal-2"), [4, 3]);
        pl.text(fs / 2, fs / 2 - fs * 0.04, " Nyquist = fs/2", cssColor("text-muted"));
      },
    });

    const update = () => {
      const alias = aliasedFrequency(f, fs);
      info.set(
        `f = <strong>${f.toFixed(0)} Hz</strong> amostrado a ${fs} Hz → ouve-se <strong>${alias.toFixed(0)} Hz</strong>` +
          (f > fs / 2 ? " · <span style='color:var(--warn)'>acima de Nyquist: aliasing</span>" : " · abaixo de Nyquist: sem ambiguidade"),
      );
      timePlot.redraw();
      foldPlot.redraw();
    };

    root.append(
      el("h2", {}, "1. As mesmas amostras servem a duas frequências"),
      p("Sobe a frequência acima de fs/2: os pontos (as amostras) passam a desenhar um seno mais lento — a tracejado. Depois de amostrado, não há forma de saber qual dos dois era o original."),
      panel(
        timePlot.canvas,
        legend([
          { label: "sinal original", color: "text-muted" },
          { label: "amostras", color: "signal" },
          { label: "o seno que as amostras \"mostram\" (alias)", color: "signal-2" },
        ]),
        controls(
          slider({ label: "Frequência do sinal", min: 100, max: 20000, value: f, step: 10, format: (v) => `${v} Hz`, onInput: (v) => { f = v; update(); } }),
          slider({ label: "Sample rate", min: 4000, max: 16000, value: fs, step: 100, format: (v) => `${v} Hz`, onInput: (v) => { fs = v; update(); } }),
        ),
        info,
      ),
      p("O \"acordeão\" da dobragem: a frequência ouvida sobe até fs/2, desce até 0 em fs, e repete."),
      panel(foldPlot.canvas),
    );

    // ---- 2. Dente de serra ingénuo vs. banda limitada ------------------------------------------
    let f0 = 2500;
    let kind: "naive" | "bandlimited" = "naive";
    const spectrumRate = 48000;

    // Os espectros só se recalculam quando a fundamental muda (não em cada redesenho).
    const fftSize = 16384;
    const freqs = Array.from({ length: fftSize / 2 }, (_, k) => Math.max(1, (k * spectrumRate) / fftSize));
    let naive: Float32Array = new Float32Array(0);
    let clean: Float32Array = new Float32Array(0);
    const recompute = () => {
      naive = magnitudeSpectrumDb(naiveSaw(f0, spectrumRate, fftSize), fftSize);
      clean = magnitudeSpectrumDb(bandlimitedSaw(f0, spectrumRate, fftSize), fftSize);
    };
    recompute();

    const spectrumPlot = new Plot(el("div"), {
      height: 260,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -100, max: 0, ticks: [-80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        pl.line(freqs, naive, cssColor("signal-2"), 1);
        pl.line(freqs, clean, cssColor("signal"), 1.4);
        for (let k = 1; k * f0 < spectrumRate / 2; k++) pl.vline(k * f0, cssColor("grid"), [1, 3]);
      },
    });

    const player = new LoopPlayer((rate) => {
      const len = rate * 2;
      return [kind === "naive" ? naiveSaw(f0, rate, len, 0.3) : bandlimitedSaw(f0, rate, len, 0.3)];
    });

    const sweepPlayer = new LoopPlayer((rate) => {
      // varrimento de 4 s de 200 Hz a 5 kHz, dente de serra ingénuo
      const len = rate * 4;
      const out = new Float32Array(len);
      let phase = 0;
      for (let i = 0; i < len; i++) {
        const freq = 200 * Math.pow(25, i / len);
        out[i] = 0.25 * (2 * phase - 1);
        phase += freq / rate;
        if (phase >= 1) phase -= 1;
      }
      return [out];
    });

    root.append(
      el("h2", {}, "2. O dente de serra ingénuo (exercício 03)"),
      p("Um dente de serra tem harmónicos em todos os múltiplos da fundamental. Os que passam de 24 kHz (a 48 kHz) dobram para baixo e caem **entre** os harmónicos verdadeiros (linhas verticais). A versão com banda limitada (soma só dos harmónicos abaixo de Nyquist) não os tem."),
      panel(
        spectrumPlot.canvas,
        legend([
          { label: "ingénuo (com aliasing)", color: "signal-2" },
          { label: "banda limitada", color: "signal" },
        ]),
        controls(
          slider({ label: "Fundamental", min: 100, max: 6000, value: f0, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { f0 = v; recompute(); spectrumPlot.redraw(); player.refresh(); } }),
          select("Ouvir", [ { value: "naive", label: "ingénuo" }, { value: "bandlimited", label: "banda limitada" } ], kind, (v) => { kind = v; player.refresh(); }),
          playButton(player, "Ouvir"),
          playButton(sweepPlayer, "Varrimento ingénuo 200 Hz → 5 kHz"),
        ),
      ),
      callout("No varrimento, ouve os tons que **descem** enquanto a nota sobe: são harmónicos acima de Nyquist a dobrar. O mesmo vai acontecer na distorção (módulo 6), e a solução lá é o oversampling."),
      sources([
        { title: "J. O. Smith — Sampling Theorem", url: "https://ccrma.stanford.edu/~jos/mdft/Sampling_Theorem.html", why: "o teorema e a sua prova" },
        { title: "Stilson & Smith — Alias-Free Digital Synthesis of Classic Analog Waveforms", url: "https://ccrma.stanford.edu/~stilti/papers/blit.pdf", why: "o problema do oscilador ingénuo e o BLIT" },
        { title: "Martin Finke — PolyBLEP Oscillator", url: "https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/", why: "uma solução prática muito usada" },
      ]),
    );

    update();
  },
};
