import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { button, controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, aliasedFrequency, dbToGain, magnitudeSpectrumDb, oversampled, shape, type ShaperKind } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;
const n = 8192;

export const oversamplingPage: PageDef = {
  id: "oversampling",
  title: "Oversampling",
  module: 6,
  summary: "Distorcer a N·fs dá espaço aos harmónicos; filtrar antes de voltar a fs remove os que teriam dobrado.",
  render(root) {
    let kind: ShaperKind = "hardClip";
    let driveDb = 18;
    let f0 = 4987;
    let factor = 4;
    const info = readout();
    let plain: Float32Array = new Float32Array(0);
    let over: Float32Array = new Float32Array(0);

    const compute = () => {
      const g = dbToGain(driveDb);
      const x = new Float32Array(n + 2048).map((_, i) => Math.sin((TWO_PI * f0 * i) / fs));
      plain = magnitudeSpectrumDb(oversampled(x, 1, (v) => shape(kind, g * v)), n, 1024);
      over = magnitudeSpectrumDb(oversampled(x, factor, (v) => shape(kind, g * v)), n, 1024);
    };

    const plot = new Plot(el("div"), {
      height: 300,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -120, max: 0, ticks: [-100, -80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const freqs = Array.from({ length: n / 2 }, (_, k) => Math.max(1, (k * fs) / n));
        pl.line(freqs, plain, cssColor("warn"), 1);
        pl.line(freqs, over, cssColor("signal"), 1.2);
        for (let k = 1; k * f0 < fs / 2; k++) pl.vline(k * f0, cssColor("signal-3"), [1, 3]);
      },
    });

    const update = () => {
      compute();
      const aliases = [2, 3, 4, 5, 6, 7, 8, 9].map((k) => k * f0).filter((f) => f > fs / 2).slice(0, 3).map((f) => aliasedFrequency(f, fs));
      const level = (spec: Float32Array, f: number) => spec[Math.round((f * n) / fs)] ?? -200;
      info.set(
        `aliases de ${f0} Hz em ` +
          aliases.map((a) => `${a.toFixed(0)} Hz: <strong>${level(plain, a).toFixed(0)}</strong> → <strong>${level(over, a).toFixed(0)} dB</strong>`).join(" · "),
      );
      plot.redraw();
    };

    let source: SourceKind = "saw";
    let listenFactor = 1;
    const player = new LoopPlayer((rate) => {
      const g = dbToGain(driveDb);
      return renderSource(source, rate, 0.7).map((ch) => oversampled(ch, listenFactor, (v) => 0.3 * shape(kind, g * v)));
    });

    root.append(
      el("h2", {}, "1. Hard clip de um seno: 1× vs N×"),
      p("Vermelho: sem oversampling. Azul: com oversampling. As linhas verdes são os harmónicos verdadeiros (abaixo de Nyquist); tudo o que está entre elas é aliasing. Experimenta frequências altas, drive alto e o hard clip."),
      panel(
        plot.canvas,
        legend([
          { label: "1× (sem oversampling)", color: "warn" },
          { label: "com oversampling", color: "signal" },
          { label: "harmónicos", color: "signal-3" },
        ]),
        controls(
          select<ShaperKind>("Curva", [ { value: "hardClip", label: "hard clip" }, { value: "softClip", label: "soft clip" }, { value: "tanh", label: "tanh" }, { value: "asymmetric", label: "assimétrica" } ], kind, (v) => { kind = v; update(); }),
          select<string>("Fator", [ { value: "2", label: "2×" }, { value: "4", label: "4×" }, { value: "8", label: "8×" } ], String(factor), (v) => { factor = Number(v); update(); }),
          slider({ label: "Drive", min: 0, max: 36, step: 0.5, value: driveDb, format: (v) => `${v} dB`, onInput: (v) => { driveDb = v; } }),
          slider({ label: "Frequência", min: 200, max: 10000, value: f0, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { f0 = v; } }),
          button("Recalcular", update),
        ),
        info,
      ),
      el("h2", {}, "2. Ouvir"),
      p("O oversampling é calculado no browser com um FIR polifásico (o `juce::dsp::Oversampling` usa filtros de meia-banda, mais eficientes). Com o dente de serra e drive alto, compara 1× e 8×: o 1× tem um \"chiado\" inarmónico por cima das notas."),
      panel(
        controls(
          sourceSelect(source, (k) => { source = k; player.refresh(); }),
          select<string>("Oversampling", [ { value: "1", label: "1×" }, { value: "2", label: "2×" }, { value: "4", label: "4×" }, { value: "8", label: "8×" } ], "1", (v) => { listenFactor = Number(v); player.refresh(); }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("No JUCE: `Oversampling<float> (canais, log2(fator), filterHalfBandPolyphaseIIR, true)`, `initProcessing (maxBlock)` no prepare, e `processSamplesUp` → curva → `processSamplesDown` no process. E comunica a latência ao host com `setLatencySamples`."),
      sources([
        { title: "juce::dsp::Oversampling", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html", why: "a classe usada no exercício 26" },
        { title: "Parker, Zavalishin & Le Bivic — DAFx 2016", url: "https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf", why: "ADAA, a alternativa" },
        { title: "J. O. Smith — Digital Audio Resampling", url: "https://ccrma.stanford.edu/~jos/resample/", why: "a teoria de mudar o sample rate" },
      ]),
    );

    update();
  },
};
