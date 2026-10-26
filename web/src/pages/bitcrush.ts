// ⚠ SPOILER: este ficheiro implementa em TypeScript a matemática de um ou mais exercícios C++.
//   Ler o código antes de fazeres o exercício equivale a ver a solução; usa a página no browser.
import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, slider, sources, callout } from "../lib/ui";
import { TWO_PI, aliasedFrequency, magnitudeSpectrumDb, quantise } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;

/** Bitcrusher: sample & hold com acumulador de fase + quantização. */
function crush(input: Float32Array, bits: number, factor: number): Float32Array {
  const out = new Float32Array(input.length);
  let phase = 1;
  let held = 0;
  for (let i = 0; i < input.length; i++) {
    if (phase >= 1) {
      phase -= 1;
      held = quantise(input[i], bits);
    }
    phase += 1 / factor;
    out[i] = held;
  }
  return out;
}

export const bitcrushPage: PageDef = {
  id: "bitcrush",
  title: "Bitcrusher",
  module: 6,
  summary: "Poucos bits (quantização) e sample & hold sem filtro (redução de sample rate). O aliasing aqui é o efeito.",
  render(root) {
    let bits = 6;
    let factor = 6;
    let f = 2500;
    const info = readout();

    const wave = new Plot(el("div"), {
      height: 230,
      x: { min: 0, max: 2, label: "ms" },
      y: { min: -1.1, max: 1.1, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        const len = Math.round(0.002 * fs);
        const x = new Float32Array(len).map((_, i) => 0.9 * Math.sin((TWO_PI * f * i) / fs));
        const y = crush(x, bits, factor);
        const ms = Array.from({ length: len }, (_, i) => (1000 * i) / fs);
        pl.line(ms, x, cssColor("text-muted"), 1);
        pl.line(ms, y, cssColor("signal"), 2);
      },
    });

    const spectrum = new Plot(el("div"), {
      height: 240,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -100, max: 0, ticks: [-80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const n = 16384;
        const x = new Float32Array(n).map((_, i) => 0.9 * Math.sin((TWO_PI * f * i) / fs));
        const db = magnitudeSpectrumDb(crush(x, bits, factor), n);
        const freqs = Array.from({ length: n / 2 }, (_, k) => Math.max(1, (k * fs) / n));
        pl.line(freqs, db, cssColor("signal"), 1);
        pl.vline(fs / factor / 2, cssColor("warn"));
        pl.vline(f, cssColor("signal-3"));
      },
    });

    const update = () => {
      const effective = fs / factor;
      info.set(
        `sample rate efetivo <strong>${effective.toFixed(0)} Hz</strong> (novo Nyquist ${(effective / 2).toFixed(0)} Hz, linha vermelha) · ` +
          `${bits} bits = <strong>${Math.pow(2, bits)}</strong> níveis` +
          (f > effective / 2 ? ` · <span style="color:var(--warn)">${f} Hz está acima do novo Nyquist: dobra para ${aliasedFrequency(f, effective).toFixed(0)} Hz</span>` : ""),
      );
      wave.redraw();
      spectrum.redraw();
    };

    let source: SourceKind = "loop";
    const player = new LoopPlayer((rate) => renderSource(source, rate, 0.8).map((ch) => crush(ch, bits, factor * (rate / fs)).map((v) => v * 0.4)));

    root.append(
      el("h2", {}, "1. Degraus no tempo e na amplitude"),
      p("Cada patamar dura `fator` amostras (redução de sample rate); a altura de cada patamar só pode ter 2^bits valores (redução de bits)."),
      panel(
        wave.canvas,
        legend([ { label: "original", color: "text-muted" }, { label: "bitcrushed", color: "signal" } ]),
        controls(
          slider({ label: "Bits", min: 1, max: 16, step: 1, value: bits, format: (v) => `${v}`, onInput: (v) => { bits = v; update(); player.refresh(); } }),
          slider({ label: "Fator", min: 1, max: 32, value: factor, log: true, format: (v) => `${v.toFixed(2)}×`, onInput: (v) => { factor = v; update(); player.refresh(); } }),
          slider({ label: "Seno de teste", min: 100, max: 12000, value: f, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { f = v; update(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. O espectro"),
      p("O sample & hold sem filtro cria réplicas do espectro em torno de múltiplos do sample rate efetivo, e tudo o que passa do novo Nyquist dobra. Linha verde: o seno original."),
      panel(spectrum.canvas),
      el("h2", {}, "3. Ouvir"),
      panel(controls(sourceSelect(source, (k) => { source = k; player.refresh(); }), playButton(player, "Ouvir"))),
      callout("Um conversor real filtraria antes de baixar o sample rate (exercício 26 faz isso para o oversampling). O bitcrusher não filtra de propósito: o \"sujo\" é o efeito."),
      sources([
        { title: "J. O. Smith — Digital Audio Resampling", url: "https://ccrma.stanford.edu/~jos/resample/", why: "como se muda o sample rate bem (o contrário disto)" },
        { title: "J. O. Smith — Round-Off Error Variance", url: "https://ccrma.stanford.edu/~jos/mdft/Round_Off_Error_Variance.html", why: "o erro de quantização (exercício 04)" },
      ]),
    );

    update();
  },
};
