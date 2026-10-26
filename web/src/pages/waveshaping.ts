import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, aliasedFrequency, dbToGain, magnitudeSpectrumDb, shape, type ShaperKind } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;
const kinds: Array<{ value: ShaperKind; label: string }> = [
  { value: "hardClip", label: "hard clip" },
  { value: "softClip", label: "soft clip (cúbica)" },
  { value: "tanh", label: "tanh" },
  { value: "asymmetric", label: "assimétrica" },
];

export const waveshapingPage: PageDef = {
  id: "waveshaping",
  title: "Waveshaping e harmónicos",
  module: 6,
  summary: "`y = f(drive · x)`: a forma da curva decide os harmónicos. Simétrica → só ímpares; assimétrica → também pares e DC.",
  render(root) {
    let kind: ShaperKind = "tanh";
    let driveDb = 12;
    let f0 = 1000;
    const info = readout();

    const curve = new Plot(el("div"), {
      height: 240,
      x: { min: -2, max: 2, label: "entrada x", ticks: [-2, -1, 0, 1, 2] },
      y: { min: -1.2, max: 1.2, label: "saída", ticks: [-1, -0.5, 0, 0.5, 1] },
      draw: (pl) => {
        pl.grid();
        pl.fn((x) => x, cssColor("grid"), 1);
        kinds.forEach((k) => pl.fn((x) => shape(k.value, x), cssColor(k.value === kind ? "signal" : "text-muted"), k.value === kind ? 2.5 : 1));
      },
    });

    const wave = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: 2, label: "ms" },
      y: { min: -1.2, max: 1.2, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        const g = dbToGain(driveDb);
        pl.fn((t) => Math.sin((TWO_PI * 1000 * t) / 1000), cssColor("text-muted"), 1);
        pl.fn((t) => shape(kind, g * Math.sin((TWO_PI * 1000 * t) / 1000)), cssColor("signal"), 2);
      },
    });

    let spectrum: Float32Array = new Float32Array(0);
    const n = 16384;
    const computeSpectrum = () => {
      const g = dbToGain(driveDb);
      const x = new Float32Array(n).map((_, i) => shape(kind, g * Math.sin((TWO_PI * f0 * i) / fs)));
      spectrum = magnitudeSpectrumDb(x, n);
    };

    const spectrumPlot = new Plot(el("div"), {
      height: 260,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -120, max: 0, ticks: [-100, -80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        // harmónicos verdadeiros (abaixo de Nyquist) e onde caem os que dobram
        for (let k = 1; k <= 40; k++) {
          const f = k * f0;
          if (f < fs / 2) pl.vline(f, cssColor("signal-3"), [1, 3]);
          else if (k <= 15) pl.vline(aliasedFrequency(f, fs), cssColor("warn"), [1, 3]);
        }
        const freqs = Array.from({ length: n / 2 }, (_, k) => Math.max(1, (k * fs) / n));
        pl.line(freqs, spectrum, cssColor("signal"), 1);
      },
    });

    const update = () => {
      computeSpectrum();
      const bin = (f: number) => spectrum[Math.round((f * n) / fs)] ?? -200;
      info.set(
        `harmónicos de ${f0} Hz: 2.º <strong>${bin(2 * f0).toFixed(0)} dB</strong>, 3.º <strong>${bin(3 * f0).toFixed(0)} dB</strong>, 5.º <strong>${bin(5 * f0).toFixed(0)} dB</strong> · ` +
          `linhas verdes = harmónicos abaixo de Nyquist; vermelhas = para onde dobram os que passam de 24 kHz`,
      );
      curve.redraw();
      wave.redraw();
      spectrumPlot.redraw();
    };

    let source: SourceKind = "saw";
    let level = 0.3;
    const player = new LoopPlayer((rate) => {
      const g = dbToGain(driveDb);
      return renderSource(source, rate, 0.8).map((ch) => ch.map((v) => level * shape(kind, g * v)));
    });

    root.append(
      el("h2", {}, "1. As curvas"),
      p("A cinzento as outras curvas, a azul a escolhida. Com sinais pequenos todas são quase retas (linear); com drive alto o sinal é empurrado para a zona curva."),
      panel(curve.canvas, controls(select<ShaperKind>("Curva", kinds, kind, (v) => { kind = v; update(); player.refresh(); }))),
      el("h2", {}, "2. Um seno de 1 kHz depois da curva"),
      panel(
        wave.canvas,
        controls(slider({ label: "Drive", min: 0, max: 40, step: 0.5, value: driveDb, format: (v) => `${v} dB`, onInput: (v) => { driveDb = v; update(); player.refresh(); } })),
      ),
      el("h2", {}, "3. O espectro"),
      p("Com tanh/hard clip/soft clip (curvas ímpares) só há harmónicos ímpares; com a assimétrica aparecem os pares. Sobe a frequência: os harmónicos acima de 24 kHz dobram para baixo (linhas vermelhas) — é aliasing (exercícios 25 e 26)."),
      panel(
        spectrumPlot.canvas,
        legend([
          { label: "espectro", color: "signal" },
          { label: "harmónicos", color: "signal-3" },
          { label: "aliases", color: "warn" },
        ]),
        controls(slider({ label: "Frequência", min: 100, max: 8000, value: f0, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { f0 = v; update(); } })),
        info,
      ),
      el("h2", {}, "4. Ouvir"),
      panel(
        controls(
          sourceSelect(source, (k) => { source = k; player.refresh(); }),
          slider({ label: "Volume", min: 0.05, max: 0.6, step: 0.01, value: level, format: (v) => v.toFixed(2), onInput: (v) => { level = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("A curva assimétrica cria um desvio DC (a média deixa de ser zero). No fuzz (exercício 23) um DC blocker trata disso."),
      sources([
        { title: "J. O. Smith — PASP: Cubic Soft Clipper", url: "https://ccrma.stanford.edu/~jos/pasp/Cubic_Soft_Clipper.html", why: "a curva cúbica" },
        { title: "Parker, Zavalishin & Le Bivic — DAFx 2016", url: "https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf", why: "aliasing em waveshaping e ADAA" },
        { title: "juce::dsp::WaveShaper", url: "https://docs.juce.com/master/structjuce_1_1dsp_1_1WaveShaper.html", why: "o waveshaper do JUCE" },
      ]),
    );

    update();
  },
};
