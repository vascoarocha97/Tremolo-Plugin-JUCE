import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, toggle, callout } from "../lib/ui";
import { TWO_PI, dbToGain, demoLoop, magnitudeSpectrumDb, quantise, seededRandom } from "../lib/dsp";

/** Quantiza um sinal para N bits, com dither TPDF opcional (como o exercício 04). */
function quantiseSignal(input: Float32Array, bits: number, dither: boolean, seed = 7): Float32Array {
  const rnd = seededRandom(seed);
  const lsb = 1 / Math.pow(2, bits - 1);
  const out = new Float32Array(input.length);
  for (let i = 0; i < input.length; i++) {
    const d = dither ? (rnd() - rnd()) * lsb : 0;
    out[i] = quantise(input[i] + d, bits);
  }
  return out;
}

export const bitDepthPage: PageDef = {
  id: "bit-depth",
  title: "Quantização e bit depth",
  module: 1,
  summary: "Com N bits há 2ᴺ níveis. O erro de arredondar é ruído — ou distorção, quando o sinal é pequeno. O dither troca a distorção por ruído.",
  render(root) {
    let bits = 4;
    let dither = false;
    let amplitude = 0.9;

    const info = readout();
    const waveform = new Plot(el("div"), {
      height: 260,
      x: { min: 0, max: 200, label: "amostras" },
      y: { min: -1.1, max: 1.1, ticks: [-1, -0.5, 0, 0.5, 1] },
      draw: (pl) => {
        pl.grid();
        const input = new Float32Array(200).map((_, n) => amplitude * Math.sin((TWO_PI * n) / 97));
        const q = quantiseSignal(input, bits, dither);
        const error = q.map((v, i) => v - input[i]);
        // níveis possíveis (só se forem poucos)
        if (bits <= 5) {
          const lsb = 1 / Math.pow(2, bits - 1);
          for (let level = -1; level < 1; level += lsb) pl.hline(level, cssColor("grid"), [1, 2]);
        }
        pl.line(null, input, cssColor("text-muted"), 1.2);
        pl.line(null, q, cssColor("signal"), 1.8);
        pl.line(null, error, cssColor("signal-2"), 1.2);
      },
    });

    // Espectro: seno de 1 kHz a 48 kHz quantizado
    const spectrum = new Plot(el("div"), {
      height: 230,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -140, max: 0, ticks: [-120, -96, -72, -48, -24, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const n = 16384;
        const input = new Float32Array(n).map((_, i) => amplitude * Math.sin((TWO_PI * 1000 * i) / 48000));
        const db = magnitudeSpectrumDb(quantiseSignal(input, bits, dither), n);
        const freqs = Array.from({ length: n / 2 }, (_, k) => Math.max(1, (k * 48000) / n));
        pl.line(freqs, db, cssColor("signal"), 1);
      },
    });

    const update = () => {
      const lsb = 1 / Math.pow(2, bits - 1);
      const n = 48000;
      const input = new Float32Array(n).map((_, i) => amplitude * Math.sin((TWO_PI * 997 * i) / 48000));
      const q = quantiseSignal(input, bits, dither);
      let s = 0;
      let e = 0;
      for (let i = 0; i < n; i++) {
        s += input[i] * input[i];
        e += (q[i] - input[i]) ** 2;
      }
      const snr = 10 * Math.log10(s / Math.max(e, 1e-30));
      info.set(
        `${bits} bits → <strong>${Math.pow(2, bits)}</strong> níveis · LSB = <strong>${lsb.toPrecision(3)}</strong> · ` +
          `SNR medido = <strong>${snr.toFixed(1)} dB</strong> · teoria para seno em escala completa: 6.02·${bits} + 1.76 = <strong>${(6.02 * bits + 1.76).toFixed(1)} dB</strong>`,
      );
      waveform.redraw();
      spectrum.redraw();
    };

    // Áudio: loop de demonstração quantizado, com nível ajustável (para ouvir sinais pequenos)
    let levelDb = 0;
    let source: "loop" | "sine" = "loop";
    let loopCache: { rate: number; data: Float32Array[] } | null = null;
    const player = new LoopPlayer((rate) => {
      if (source === "sine") {
        const s = new Float32Array(rate * 2).map((_, i) => dbToGain(levelDb) * 0.9 * Math.sin((TWO_PI * 440 * i) / rate));
        const q = quantiseSignal(s, bits, dither);
        return [q.map((v) => v / Math.max(dbToGain(levelDb), 1e-4) * 0.5)];
      }
      if (!loopCache || loopCache.rate !== rate) loopCache = { rate, data: demoLoop(rate) };
      const g = dbToGain(levelDb);
      // Quantiza ao nível escolhido e volta a subir o volume depois, para se ouvir o efeito
      return loopCache.data.map((ch, i) => quantiseSignal(ch.map((v) => v * g), bits, dither, 7 + i).map((v) => (v / Math.max(g, 1e-4)) * 0.6));
    });

    root.append(
      el("h2", {}, "1. Arredondar para o nível mais próximo"),
      p("A cinzento o sinal original, a azul o quantizado, a laranja o erro (diferença). Com poucos bits vês os degraus; com o dither ligado o erro deixa de seguir a forma do sinal."),
      panel(
        waveform.canvas,
        legend([
          { label: "original", color: "text-muted" },
          { label: "quantizado", color: "signal" },
          { label: "erro", color: "signal-2" },
        ]),
        controls(
          slider({ label: "Bits", min: 1, max: 16, step: 1, value: bits, format: (v) => `${v}`, onInput: (v) => { bits = v; update(); player.refresh(); } }),
          slider({ label: "Amplitude", min: 0.001, max: 1, value: amplitude, log: true, format: (v) => v.toFixed(3), onInput: (v) => { amplitude = v; update(); } }),
          toggle("Dither TPDF", dither, (v) => { dither = v; update(); player.refresh(); }),
        ),
        info,
      ),
      el("h2", {}, "2. No espectro: distorção ou ruído?"),
      p("Um seno de 1 kHz quantizado. Sem dither, com amplitudes pequenas, aparecem **harmónicos** (distorção). Com dither, o erro espalha-se num chão de ruído plano — e um sinal abaixo de meio LSB, que sem dither desaparecia, continua visível."),
      panel(spectrum.canvas),
      el("h2", {}, "3. Ouvir"),
      p("Escolhe poucos bits e baixa o nível: o áudio é quantizado a esse nível e depois amplificado, para ouvires o que acontece aos sinais fracos (como o fim de uma nota a decair)."),
      panel(
        controls(
          select("Fonte", [ { value: "loop", label: "loop de demonstração" }, { value: "sine", label: "seno 440 Hz" } ], source, (v) => { source = v; player.refresh(); }),
          slider({ label: "Nível antes de quantizar", min: -60, max: 0, value: levelDb, step: 1, format: (v) => `${v} dB`, onInput: (v) => { levelDb = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Na prática: dentro do plugin trabalhas em `float` (24 bits de mantissa + expoente). O dither aplica-se uma vez, ao exportar para 16 bits. O bitcrusher (exercício 24) usa a quantização de propósito."),
      sources([
        { title: "J. O. Smith — Round-Off Error Variance", url: "https://ccrma.stanford.edu/~jos/mdft/Round_Off_Error_Variance.html", why: "de onde vem LSB²/12" },
        { title: "Lipshitz, Wannamaker & Vanderkooy — Quantization and Dither: A Theoretical Survey", url: "https://hajim.rochester.edu/ece/sites/zduan/teaching/ece472/reading/Lipshitz_1992.pdf", why: "porque é que o dither TPDF funciona" },
      ]),
    );

    update();
  },
};
