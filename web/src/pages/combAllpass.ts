import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { callout, controls, el, legend, p, panel, playButton, readout, select, slider, sources } from "../lib/ui";
import { TWO_PI, gainToDb } from "../lib/dsp";
import { Allpass, Comb } from "../lib/reverb";
import { renderSource } from "../lib/sources";

const fs = 48000;
type Kind = "comb" | "allpass";
type Structure = "comb" | "allpass" | "series";
type Source = "clicks" | "loop" | "noise";

/** Um clique a cada meio segundo, 3 s. */
function clicks(rate: number): Float32Array[] {
  const x = new Float32Array(3 * rate);
  for (let k = 0; k < 6; k++) x[Math.floor(k * 0.5 * rate)] = 0.8;
  return [x, x.slice()];
}

export const combAllpassPage: PageDef = {
  id: "comb-allpass",
  title: "Comb e allpass",
  module: 7,
  summary: "Os dois blocos clássicos da reverb artificial: o pente faz ecos regulares (e picos no espectro); o allpass espalha o som no tempo sem mudar o espectro.",
  render(root) {
    let kind: Kind = "comb";
    let delayMs = 5;
    let gain = 0.7;
    let damping = 0;
    let h = new Float32Array(0);
    const info = readout();

    const compute = () => {
      const n = Math.floor(0.06 * fs);
      const d = Math.max(1, Math.round((delayMs / 1000) * fs));
      h = new Float32Array(n);
      const comb = new Comb(d, gain, damping);
      const ap = new Allpass(d, gain);
      for (let i = 0; i < n; i++) h[i] = kind === "comb" ? comb.process(i === 0 ? 1 : 0) : ap.process(i === 0 ? 1 : 0);
    };

    const impulse = new Plot(el("div"), {
      height: 220,
      x: { min: 0, max: 60, label: "tempo (ms)" },
      y: { min: -1, max: 1.05, ticks: [-1, -0.5, 0, 0.5, 1] },
      draw: (pl) => {
        pl.grid();
        const xs = Array.from(h, (_, i) => (i / fs) * 1000);
        pl.line(xs, h, cssColor("signal"), 1.4);
      },
    });

    const response = new Plot(el("div"), {
      height: 240,
      x: { min: 0, max: 2000, label: "frequência (Hz)" },
      y: { min: -18, max: 24, ticks: [-18, -12, -6, 0, 6, 12, 18, 24], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const D = Math.max(1, Math.round((delayMs / 1000) * fs));
        // Comb: H = z^-D / (1 − g·LP(z)·z^-D), LP(z) = (1 − d) / (1 − d·z^-1)
        pl.fn((f) => {
          const w = (TWO_PI * f) / fs;
          // LP(e^jw)
          const lpDenRe = 1 - damping * Math.cos(w);
          const lpDenIm = damping * Math.sin(w);
          const lpMag2 = lpDenRe * lpDenRe + lpDenIm * lpDenIm;
          const lpRe = ((1 - damping) * lpDenRe) / lpMag2;
          const lpIm = (-(1 - damping) * lpDenIm) / lpMag2;
          // g·LP·e^{-jwD}
          const c = Math.cos(w * D);
          const s = -Math.sin(w * D);
          const re = gain * (lpRe * c - lpIm * s);
          const im = gain * (lpRe * s + lpIm * c);
          return gainToDb(1 / Math.hypot(1 - re, -im));
        }, cssColor("signal"), 1.4, 2000);
        pl.hline(0, cssColor("signal-2"), [], 2.2);
      },
    });

    const update = () => {
      compute();
      const D = Math.max(1, Math.round((delayMs / 1000) * fs));
      info.set(
        `D = <strong>${D} amostras</strong> · picos do pente a cada <strong>${(fs / D).toFixed(0)} Hz</strong> · ` +
          `pico máximo do comb: <strong>${gainToDb(1 / (1 - gain)).toFixed(1)} dB</strong> · ` +
          `allpass: 0 dB a todas as frequências, mas o primeiro valor da resposta é <strong>−g = ${(-gain).toFixed(2)}</strong>`,
      );
      impulse.redraw();
      response.redraw();
    };

    // Áudio
    let structure: Structure = "series";
    let source: Source = "clicks";
    let audioDelayMs = 8;
    let audioGain = 0.6;
    const player = new LoopPlayer((rate) => {
      const input = source === "clicks" ? clicks(rate) : renderSource(source, rate, 0.4);
      return input.map((x, ch) => {
        const d = Math.round((audioDelayMs / 1000) * rate) + ch * 7;
        const out = new Float32Array(x.length);
        if (structure === "comb") {
          const c = new Comb(d, audioGain, 0.2);
          for (let pass = 0; pass < 2; pass++) for (let i = 0; i < x.length; i++) { const y = c.process(x[i]); if (pass) out[i] = 0.5 * y; }
        } else if (structure === "allpass") {
          const a = new Allpass(d, audioGain);
          for (let pass = 0; pass < 2; pass++) for (let i = 0; i < x.length; i++) { const y = a.process(x[i]); if (pass) out[i] = y; }
        } else {
          // 6 allpasses curtos em série (1–15 ms): a "mola" metálica de que fala a Valhalla
          const chain = [1.3, 2.9, 4.7, 7.1, 10.3, 14.9].map((ms) => new Allpass(Math.round((ms / 1000) * rate) + ch * 3, audioGain));
          for (let pass = 0; pass < 2; pass++)
            for (let i = 0; i < x.length; i++) {
              let y = x[i];
              for (const a of chain) y = a.process(y);
              if (pass) out[i] = y;
            }
        }
        return out;
      });
    });

    root.append(
      el("h2", {}, "1. Resposta a um impulso"),
      p("O **comb** dá ecos em D, 2D, 3D... com ganhos 1, g, g²... (com damping, cada eco fica mais arredondado: os agudos perdem-se a cada volta). O **allpass** começa com −g e depois dá ecos (1 − g²), (1 − g²)·g, ... — também é uma série de ecos!"),
      panel(
        impulse.canvas,
        controls(
          select<Kind>("Bloco", [{ value: "comb", label: "comb com feedback" }, { value: "allpass", label: "allpass de Schroeder" }], kind, (v) => { kind = v; update(); }),
          slider({ label: "Atraso D", min: 1, max: 15, step: 0.1, value: delayMs, format: (v) => `${v.toFixed(1)} ms`, onInput: (v) => { delayMs = v; update(); } }),
          slider({ label: "g", min: 0, max: 0.95, step: 0.01, value: gain, format: (v) => v.toFixed(2), onInput: (v) => { gain = v; update(); } }),
          slider({ label: "Damping (comb)", min: 0, max: 0.9, step: 0.01, value: damping, format: (v) => v.toFixed(2), onInput: (v) => { damping = v; update(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. Em frequência"),
      p("O comb tem **picos** a cada fs/D Hz (com damping, os picos agudos baixam). O allpass é **plano**: `|H| = 1` em todo o lado. A diferença entre os dois está só na fase — e é isso que faz o allpass espalhar o som sem o colorir (num som estacionário)."),
      panel(response.canvas, legend([{ label: "comb |H(f)|", color: "signal" }, { label: "allpass |H(f)| = 0 dB", color: "signal-2" }])),
      el("h2", {}, "3. Ouvir"),
      p("Com **cliques**: o comb dá uma nota metálica (frequência fs/D); um allpass sozinho mal se ouve; **seis allpasses curtos em série** dão uma \"mola\" — densa mas metálica. Por isso a reverb do exercício 28 usa os dois: combs para a cauda, allpasses para a densidade."),
      panel(
        controls(
          select<Structure>("Estrutura", [ { value: "comb", label: "1 comb (damping 0.2)" }, { value: "allpass", label: "1 allpass" }, { value: "series", label: "6 allpasses em série" } ], structure, (v) => { structure = v; player.refresh(); }),
          select<Source>("Fonte", [ { value: "clicks", label: "cliques" }, { value: "loop", label: "loop de demonstração" }, { value: "noise", label: "ruído branco" } ], source, (v) => { source = v; player.refresh(); }),
          slider({ label: "Atraso", min: 1, max: 40, step: 0.5, value: audioDelayMs, format: (v) => `${v.toFixed(1)} ms`, onInput: (v) => { audioDelayMs = v; player.refresh(); } }),
          slider({ label: "g", min: 0, max: 0.9, step: 0.01, value: audioGain, format: (v) => v.toFixed(2), onInput: (v) => { audioGain = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("O \"allpass\" do Freeverb (e do `juce::Reverb`) é `(−1 + (1 + g)·z⁻ᴰ) / (1 − g·z⁻ᴰ)`: só é allpass para g ≈ 0.618, e usa g = 0.5. No exercício 27 implementas o verdadeiro.", "warn"),
      sources([
        { title: "J. O. Smith — PASP: Feedback Comb Filters", url: "https://ccrma.stanford.edu/~jos/pasp/Feedback_Comb_Filters.html", why: "equação e resposta em frequência do pente" },
        { title: "J. O. Smith — PASP: Lowpass Feedback Comb Filter", url: "https://ccrma.stanford.edu/~jos/pasp/Lowpass_Feedback_Comb_Filter.html", why: "o comb com amortecimento do Freeverb" },
        { title: "J. O. Smith — PASP: Schroeder Allpass Sections", url: "https://ccrma.stanford.edu/~jos/pasp/Schroeder_Allpass_Sections.html", why: "o allpass e a sua resposta a um impulso" },
        { title: "J. O. Smith — PASP: Freeverb Allpass Approximation", url: "https://ccrma.stanford.edu/~jos/pasp/Freeverb_Allpass_Approximation.html", why: "porque é que o allpass do Freeverb não o é" },
        { title: "Valhalla DSP — Reverbs: Diffusion, allpass delays, and metallic artifacts", url: "https://valhalladsp.com/2011/01/21/reverbs-diffusion-allpass-delays-and-metallic-artifacts/", why: "porque é que allpasses em série soam metálicos" },
      ]),
    );

    update();
  },
};
