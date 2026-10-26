import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { controls, el, legend, p, panel, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, fft, gainToDb } from "../lib/dsp";

type WindowKind = "rectangular" | "hann" | "blackman";

function windowValue(kind: WindowKind, i: number, n: number): number {
  switch (kind) {
    case "rectangular": return 1;
    case "hann": return 0.5 - 0.5 * Math.cos((TWO_PI * i) / n);
    case "blackman": return 0.42 - 0.5 * Math.cos((TWO_PI * i) / n) + 0.08 * Math.cos((2 * TWO_PI * i) / n);
  }
}

export const fftPage: PageDef = {
  id: "fft",
  title: "FFT e janelas",
  module: 6,
  summary: "A FFT mede quanto de cada frequência há num bloco de N amostras. Uma janela evita que a energia \"escorra\" para todo o espectro.",
  render(root) {
    const n = 256;
    let binPosition = 20.0; // frequência do seno em bins (fracionário = não cai num bin)
    let kind: WindowKind = "hann";
    const info = readout();

    const timePlot = new Plot(el("div"), {
      height: 180,
      x: { min: 0, max: n, label: "amostra" },
      y: { min: -1.1, max: 1.1, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        const xs = Array.from({ length: n }, (_, i) => i);
        pl.line(xs, xs.map((i) => Math.sin((TWO_PI * binPosition * i) / n)), cssColor("text-muted"), 1);
        pl.line(xs, xs.map((i) => windowValue(kind, i, n) * Math.sin((TWO_PI * binPosition * i) / n)), cssColor("signal"), 1.6);
        pl.line(xs, xs.map((i) => windowValue(kind, i, n)), cssColor("signal-2"), 1, [4, 3]);
      },
    });

    const spectrumPlot = new Plot(el("div"), {
      height: 260,
      x: { min: 0, max: 64, label: "bin k (frequência = k·fs/N)" },
      y: { min: -120, max: 5, ticks: [-120, -100, -80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const spectrum = (w: WindowKind) => {
          const re = new Float64Array(n);
          const im = new Float64Array(n);
          let sum = 0;
          for (let i = 0; i < n; i++) {
            const wv = windowValue(w, i, n);
            sum += wv;
            re[i] = wv * Math.sin((TWO_PI * binPosition * i) / n);
          }
          fft(re, im);
          // normalizado: um seno de amplitude 1 num bin exato dá 0 dB
          return Array.from({ length: 65 }, (_, k) => gainToDb((2 * Math.hypot(re[k], im[k])) / sum));
        };
        const ks = Array.from({ length: 65 }, (_, k) => k);
        pl.line(ks, spectrum("rectangular"), cssColor("text-muted"), 1);
        pl.stems(ks, spectrum(kind), cssColor("signal"), 2.5);
        pl.vline(binPosition, cssColor("signal-2"));
      },
    });

    const update = () => {
      const exact = Math.abs(binPosition - Math.round(binPosition)) < 1e-9;
      info.set(
        `o seno está em <strong>${binPosition.toFixed(2)}</strong> bins: ` +
          (exact ? "cai exatamente num bin — mesmo sem janela, toda a energia fica nesse bin." : "não cai num bin — sem janela (cinzento) a energia escorre para todo o espectro; com a janela fica concentrada perto."),
      );
      timePlot.redraw();
      spectrumPlot.redraw();
    };

    root.append(
      el("h2", {}, "1. O bloco e a janela"),
      p("A FFT vê só N = 256 amostras e assume que elas se repetem para sempre. Se o seno não tiver um número inteiro de ciclos no bloco, as pontas não colam. A janela (tracejado) leva as pontas a zero suavemente."),
      panel(
        timePlot.canvas,
        legend([ { label: "sinal", color: "text-muted" }, { label: "sinal × janela", color: "signal" }, { label: "janela", color: "signal-2" } ]),
        controls(
          slider({ label: "Frequência (em bins)", min: 10, max: 30, step: 0.05, value: binPosition, format: (v) => v.toFixed(2), onInput: (v) => { binPosition = v; update(); } }),
          select<WindowKind>("Janela", [ { value: "rectangular", label: "retangular (nenhuma)" }, { value: "hann", label: "Hann" }, { value: "blackman", label: "Blackman" } ], kind, (v) => { kind = v; update(); }),
        ),
        info,
      ),
      el("h2", {}, "2. O espectro"),
      p("Cinzento: sem janela. Azul: com a janela escolhida. Repara no compromisso: a janela alarga o pico principal (mais bins ocupados) mas faz cair os lados (as \"fugas\") dezenas de dB."),
      panel(spectrumPlot.canvas),
      callout("No exercício 25: `juce::dsp::WindowingFunction` (Hann, normalizada) + `juce::dsp::FFT::performFrequencyOnlyForwardTransform`, num array de 2·N floats. Com a janela normalizada, um seno de amplitude A dá |X| = A·N/2."),
      sources([
        { title: "J. O. Smith — Mathematics of the DFT: FFT", url: "https://ccrma.stanford.edu/~jos/mdft/Fast_Fourier_Transform_FFT.html", why: "a DFT e a FFT" },
        { title: "J. O. Smith — Spectrum Analysis Windows", url: "https://ccrma.stanford.edu/~jos/sasp/Spectrum_Analysis_Windows.html", why: "janelas e escorrimento espectral" },
        { title: "juce::dsp::FFT", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html", why: "a FFT do JUCE" },
      ]),
    );

    update();
  },
};
