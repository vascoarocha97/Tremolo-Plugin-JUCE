import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { callout, controls, el, legend, p, panel, playButton, readout, slider, sources } from "../lib/ui";
import { SchroederReverb, energyDecayDb, envelope, estimateRt60, processLoopTwice } from "../lib/reverb";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;

export const reverbPage: PageDef = {
  id: "reverb",
  title: "Reverb de Schroeder",
  module: 7,
  summary: "8 combs em paralelo fazem a cauda, 4 allpasses em série multiplicam os ecos. O RT60 define o feedback de cada comb; a curva de Schroeder mede-o.",
  render(root) {
    let rt60 = 2;
    let damping = 0.3;
    let seconds = 3;
    let left = new Float32Array(0);
    let right = new Float32Array(0);
    let edc: Float32Array = new Float32Array(0);
    let combGains: number[] = [];
    const info = readout();

    const compute = () => {
      // mostra 1.3·RT60, mas calcula 2·RT60: a curva de Schroeder truncada cedo subestima o RT60
      seconds = Math.min(12, Math.max(1.5, rt60 * 1.3));
      const n = Math.floor(Math.min(16, Math.max(2, rt60 * 2)) * fs);
      const rev = new SchroederReverb(fs, rt60, damping);
      left = new Float32Array(n);
      right = new Float32Array(n);
      for (let i = 0; i < n; i++) [left[i], right[i]] = rev.process(i === 0 ? 1 : 0);
      edc = energyDecayDb(left);
      combGains = rev.combs[0].map((c) => c.feedback);
    };

    const decay = new Plot(el("div"), {
      height: 260,
      x: { min: 0, max: 3, label: "tempo (s)" },
      y: { min: -90, max: 0, ticks: [-80, -60, -40, -20, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const env = envelope(left, 800);
        let peak = 0;
        for (const v of env.v) peak = Math.max(peak, v);
        pl.line(Array.from(env.t, (t) => t / fs), Array.from(env.v, (v) => 20 * Math.log10(v / peak + 1e-12)), cssColor("signal"), 1);
        const step = Math.max(1, Math.floor(edc.length / 800));
        const xs: number[] = [];
        const ys: number[] = [];
        for (let i = 0; i < edc.length; i += step) { xs.push(i / fs); ys.push(edc[i]); }
        pl.line(xs, ys, cssColor("signal-2"), 2.2);
        pl.hline(-60, cssColor("text-muted"));
        pl.vline(rt60, cssColor("signal-3"), [6, 4], 1.5);
      },
    });

    const early = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: 120, label: "tempo (ms)" },
      y: { min: -0.1, max: 0.1, ticks: [-0.1, -0.05, 0, 0.05, 0.1] },
      draw: (pl) => {
        pl.grid();
        const n = Math.floor(0.12 * fs);
        const xs = Array.from({ length: n }, (_, i) => (i / fs) * 1000);
        pl.line(xs, left.subarray(0, n), cssColor("signal"), 1);
        pl.line(xs, right.subarray(0, n), cssColor("signal-2"), 1);
      },
    });

    const update = () => {
      compute();
      decay.setAxes({ min: 0, max: seconds, label: "tempo (s)" });
      early.redraw();
      const measured = estimateRt60(edc, fs);
      info.set(
        `RT60 pedido <strong>${rt60.toFixed(2)} s</strong> · medido pela curva de Schroeder (T20 × 3): <strong>${measured.toFixed(2)} s</strong>` +
          (damping > 0.05 ? " (com damping os agudos morrem mais cedo: o RT60 de banda larga fica abaixo do pedido; com damping 0 coincidem)" : "") +
          "<br>" +
          `g de cada comb (L): ${combGains.map((g) => `<code>${g.toFixed(3)}</code>`).join(" ")} — os mais compridos têm g menor`,
      );
    };

    // Áudio
    let source: SourceKind = "loop";
    let audioRt60 = 2.5;
    let audioDamping = 0.35;
    let mix = 0.3;
    const player = new LoopPlayer((rate) => {
      const rev = new SchroederReverb(rate, audioRt60, audioDamping);
      return processLoopTwice(renderSource(source, rate, 0.45), mix, (l, r) => rev.process(0.5 * (l + r)));
    });

    root.append(
      el("h2", {}, "1. A cauda e a curva de decaimento"),
      p("A resposta a um impulso (em dB, linha fina) é ruidosa. A **curva de Schroeder** (energia que ainda falta chegar, integrada de trás para a frente) é quase uma reta: o tempo que ela leva a cair 60 dB é o RT60. A linha a tracejado marca o RT60 pedido."),
      panel(
        decay.canvas,
        legend([{ label: "|h(t)| (envelope)", color: "signal" }, { label: "curva de Schroeder (EDC)", color: "signal-2" }, { label: "RT60 pedido", color: "signal-3" }]),
        controls(
          slider({ label: "RT60", min: 0.3, max: 8, step: 0.1, value: rt60, log: true, format: (v) => `${v.toFixed(1)} s`, onInput: (v) => { rt60 = v; update(); } }),
          slider({ label: "Damping", min: 0, max: 0.9, step: 0.01, value: damping, format: (v) => v.toFixed(2), onInput: (v) => { damping = v; update(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. Os primeiros 120 ms, L e R"),
      p("Os primeiros ecos só chegam ao fim de ~25 ms (o comb mais curto) e ainda se distinguem uns dos outros. L e R usam atrasos +23 amostras diferentes: os ecos não coincidem — **descorrelação**, que é o que dá largura estéreo."),
      panel(early.canvas, legend([{ label: "L", color: "signal" }, { label: "R", color: "signal-2" }])),
      el("h2", {}, "3. Ouvir"),
      panel(
        controls(
          sourceSelect(source, (v) => { source = v; player.refresh(); }),
          slider({ label: "RT60", min: 0.3, max: 8, step: 0.1, value: audioRt60, log: true, format: (v) => `${v.toFixed(1)} s`, onInput: (v) => { audioRt60 = v; player.refresh(); } }),
          slider({ label: "Damping", min: 0, max: 0.9, step: 0.01, value: audioDamping, format: (v) => v.toFixed(2), onInput: (v) => { audioDamping = v; player.refresh(); } }),
          slider({ label: "Mistura", min: 0, max: 1, step: 0.01, value: mix, format: (v) => v.toFixed(2), onInput: (v) => { mix = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Ouve a tarola do loop com RT60 alto e damping 0: nota-se um \"flutter\" metálico na cauda — os 8 pentes ainda se ouvem. É o que a FDN (exercício 29) melhora."),
      sources([
        { title: "J. O. Smith — PASP: Schroeder Reverberators", url: "https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html", why: "a estrutura combs + allpasses" },
        { title: "J. O. Smith — PASP: Freeverb", url: "https://ccrma.stanford.edu/~jos/pasp/Freeverb.html", why: "afinações e stereo spread usados aqui" },
        { title: "J. O. Smith — PASP: Energy Decay Curve", url: "https://ccrma.stanford.edu/~jos/pasp/Energy_Decay_Curve.html", why: "a integração de Schroeder" },
        { title: "juce::Reverb — documentação", url: "https://docs.juce.com/master/classjuce_1_1Reverb.html", why: "a versão JUCE do Freeverb" },
      ]),
    );

    update();
  },
};
