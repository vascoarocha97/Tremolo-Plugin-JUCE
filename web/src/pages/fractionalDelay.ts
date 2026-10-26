import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, demoLoop, gainToDb } from "../lib/dsp";

/** Delay modulado com interpolação linear (vibrato): o mecanismo do exercício 11 e do chorus. */
function vibrato(input: Float32Array, rate: number, depthMs: number, lfoHz: number): Float32Array {
  const maxDelay = Math.ceil(((depthMs * 2 + 2) / 1000) * rate) + 4;
  const size = maxDelay + 2;
  const line = new Float32Array(size);
  let w = 0;
  const out = new Float32Array(input.length);
  const centre = ((depthMs + 1) / 1000) * rate;
  const depth = (depthMs / 1000) * rate;
  for (let i = 0; i < input.length; i++) {
    w = (w + 1) % size;
    line[w] = input[i];
    const d = centre + depth * Math.sin((TWO_PI * lfoHz * i) / rate);
    const whole = Math.floor(d);
    const frac = d - whole;
    const a = line[(w - whole + size) % size];
    const b = line[(w - whole - 1 + size) % size];
    out[i] = a + frac * (b - a);
  }
  return out;
}

export const fractionalDelayPage: PageDef = {
  id: "fractional-delay",
  title: "Interpolação e delay fracionário",
  module: 3,
  summary: "Ler \"entre amostras\" com uma reta entre as duas vizinhas — e o preço que isso tem nas altas frequências.",
  render(root) {
    let fraction = 0.25;
    const info = readout();

    const samples = [0.1, 0.55, 0.9, 0.35, -0.4, -0.85, -0.2];
    const interp = new Plot(el("div"), {
      height: 230,
      x: { min: -0.5, max: 6.5, label: "atraso em amostras (0 = a mais recente)" },
      y: { min: -1.1, max: 1.1, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        pl.line(null, samples, cssColor("grid"), 1);
        pl.stems(null, samples, cssColor("signal"), 3.5);
        const d = 2 + fraction;
        const whole = Math.floor(d);
        const value = samples[whole] + (d - whole) * (samples[whole + 1] - samples[whole]);
        pl.line([whole, whole + 1], [samples[whole], samples[whole + 1]], cssColor("signal-2"), 2);
        pl.stems([d], [value], cssColor("signal-2"), 6);
        pl.text(d + 0.1, value + 0.12, `readLinear(${d.toFixed(2)}) = ${value.toFixed(3)}`, cssColor("signal-2"));
      },
    });

    const response = new Plot(el("div"), {
      height: 220,
      x: { min: 0, max: 24000, label: "frequência (Hz) a 48 kHz", ticks: [0, 5000, 10000, 15000, 20000] },
      y: { min: -24, max: 1, ticks: [-24, -18, -12, -6, -3, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const mag = (f: number, fr: number) => {
          const w = (TWO_PI * f) / 48000;
          // H = (1 - fr) + fr·e^{-jw}
          const re = 1 - fr + fr * Math.cos(w);
          const im = -fr * Math.sin(w);
          return gainToDb(Math.hypot(re, im));
        };
        pl.fn((f) => mag(f, 0.5), cssColor("text-muted"), 1.5, 400, [5, 4]);
        pl.fn((f) => mag(f, fraction), cssColor("signal-2"), 2);
      },
    });

    const update = () => {
      info.set(
        `fração = <strong>${fraction.toFixed(2)}</strong>: peso <strong>${(1 - fraction).toFixed(2)}</strong> na amostra mais recente e <strong>${fraction.toFixed(2)}</strong> na anterior · ` +
          `a 20 kHz o ganho é <strong>${gainToDb(Math.hypot(1 - fraction + fraction * Math.cos((TWO_PI * 20000) / 48000), fraction * Math.sin((TWO_PI * 20000) / 48000))).toFixed(1)} dB</strong>`,
      );
      interp.redraw();
      response.redraw();
    };

    // Áudio: vibrato (delay modulado)
    let depthMs = 2;
    let lfoHz = 5;
    let source: "loop" | "sine" = "sine";
    let loopCache: { rate: number; data: Float32Array[] } | null = null;
    const player = new LoopPlayer((rate) => {
      if (source === "sine") {
        const len = rate * 2;
        const s = new Float32Array(len).map((_, i) => 0.5 * Math.sin((TWO_PI * 440 * i) / rate));
        return [vibrato(s, rate, depthMs, lfoHz)];
      }
      if (!loopCache || loopCache.rate !== rate) loopCache = { rate, data: demoLoop(rate) };
      return loopCache.data.map((ch) => vibrato(ch.map((v) => v * 0.6), rate, depthMs, lfoHz));
    });

    root.append(
      el("h2", {}, "1. Uma reta entre duas amostras"),
      p("Para um atraso de 2 + fração, a interpolação linear usa as amostras de há 2 e de há 3 passos, pesadas por (1 − fração) e fração."),
      panel(
        interp.canvas,
        controls(slider({ label: "Fração", min: 0, max: 0.99, step: 0.01, value: fraction, format: (v) => v.toFixed(2), onInput: (v) => { fraction = v; update(); } })),
        info,
      ),
      el("h2", {}, "2. O preço: um passa-baixo"),
      p("A interpolação linear é um filtro FIR de 2 coeficientes. Com fração 0 não faz nada; com 0.5 (tracejado) é a média de duas amostras e a 20 kHz perde quase 12 dB. Num delay modulado, o brilho do som oscila com a modulação."),
      panel(
        response.canvas,
        legend([
          { label: "fração atual", color: "signal-2" },
          { label: "fração 0.5 (pior caso)", color: "text-muted" },
        ]),
      ),
      el("h2", {}, "3. Atraso que muda = altura que muda (Doppler)"),
      p("Um delay cujo tempo oscila com um LFO: quando o atraso cresce, o ponto de leitura anda mais devagar do que a escrita e o som desce; quando diminui, sobe. É um vibrato — e é a base do chorus (exercício 20)."),
      panel(
        controls(
          select("Fonte", [ { value: "sine", label: "seno 440 Hz" }, { value: "loop", label: "loop de demonstração" } ], source, (v) => { source = v; player.refresh(); }),
          slider({ label: "Profundidade", min: 0, max: 8, step: 0.1, value: depthMs, format: (v) => `${v.toFixed(1)} ms`, onInput: (v) => { depthMs = v; player.refresh(); } }),
          slider({ label: "Velocidade do LFO", min: 0.1, max: 12, value: lfoHz, log: true, format: (v) => `${v.toFixed(2)} Hz`, onInput: (v) => { lfoHz = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Variação de altura máxima ≈ 2π·profundidade·f_LFO (em fração da frequência): 2 ms a 5 Hz dá ±6.3%, cerca de um meio-tom."),
      sources([
        { title: "J. O. Smith — PASP: Delay-Line Interpolation", url: "https://ccrma.stanford.edu/~jos/pasp/Delay_Line_Interpolation.html", why: "interpolação em delay lines" },
        { title: "J. O. Smith — PASP: Linear Interpolation", url: "https://ccrma.stanford.edu/~jos/pasp/Linear_Interpolation.html", why: "a fórmula e a resposta em frequência" },
        { title: "J. O. Smith — PASP: Doppler Effect", url: "https://ccrma.stanford.edu/~jos/pasp/Doppler_Effect.html", why: "porque é que um atraso variável muda a altura" },
      ]),
    );

    update();
  },
};
