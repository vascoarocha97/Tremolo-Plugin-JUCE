import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { animate } from "../lib/lifecycle";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, gainToDb, lfoValue, modulatedDelay } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

type Effect = "chorus" | "flanger";

const presets: Record<Effect, { minMs: number; maxMs: number; rate: number; feedback: number; mix: number }> = {
  chorus: { minMs: 10, maxMs: 20, rate: 0.8, feedback: 0, mix: 0.5 },
  flanger: { minMs: 0.3, maxMs: 5, rate: 0.25, feedback: 0.6, mix: 0.5 },
};

export const chorusFlangerPage: PageDef = {
  id: "chorus-flanger",
  title: "Chorus e flanger",
  module: 5,
  summary: "Um delay curto e modulado somado ao original. Com 0.1–10 ms e feedback é um flanger; com 5–30 ms é um chorus.",
  render(root) {
    let effect: Effect = "flanger";
    let { minMs, maxMs, rate, feedback, mix } = presets[effect];
    let time = 0;
    let last = 0;
    const info = readout();

    const currentDelayMs = () => minMs + (maxMs - minMs) * 0.5 * (lfoValue("triangle", rate * time) + 1);

    const response = new Plot(el("div"), {
      height: 260,
      x: { min: 20, max: 20000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -30, max: 18, ticks: [-24, -12, -6, 0, 6, 12], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const D = currentDelayMs() / 1000;
        // H = (1 − m) + m·e^{−jωD} / (1 − fb·e^{−jωD})
        pl.fn((f) => {
          const w = TWO_PI * f * D;
          const cr = Math.cos(w), ci = -Math.sin(w);
          const dr = 1 - feedback * cr, di = -feedback * ci;
          const den = dr * dr + di * di;
          const qr = (cr * dr + ci * di) / den, qi = (ci * dr - cr * di) / den;
          return gainToDb(Math.hypot(1 - mix + mix * qr, mix * qi));
        }, cssColor("signal"), 1.8, 1500);
        pl.text(25, 14, `atraso agora: ${currentDelayMs().toFixed(2)} ms`, cssColor("text-muted"));
      },
    });

    const update = () => {
      const firstNotch = 1000 / (2 * currentDelayMs());
      info.set(
        `atraso entre <strong>${minMs.toFixed(1)}</strong> e <strong>${maxMs.toFixed(1)} ms</strong> · LFO <strong>${rate.toFixed(2)} Hz</strong> · ` +
          `feedback <strong>${feedback.toFixed(2)}</strong> · com o atraso atual o 1.º vale está em ~${firstNotch.toFixed(0)} Hz`,
      );
    };

    animate((t) => {
      if (last) time += t - last;
      last = t;
      response.redraw();
      update();
    });

    let kind: SourceKind = "noise";
    const player = new LoopPlayer((sampleRate) => {
      const input = renderSource(kind, sampleRate, 0.45);
      const msToSamples = sampleRate / 1000;
      return input.map((ch, c) =>
        modulatedDelay(
          ch,
          (n) => (minMs + (maxMs - minMs) * 0.5 * (lfoValue("triangle", (rate * n) / sampleRate + (c === 1 ? 0.25 : 0)) + 1)) * msToSamples,
          feedback,
          mix,
          (maxMs + 1) * msToSamples,
        ),
      );
    });

    const sliders = {
      min: slider({ label: "Atraso mínimo", min: 0.1, max: 25, value: minMs, log: true, format: (v) => `${v.toFixed(2)} ms`, onInput: (v) => { minMs = Math.min(v, maxMs); player.refresh(); } }),
      max: slider({ label: "Atraso máximo", min: 0.2, max: 30, value: maxMs, log: true, format: (v) => `${v.toFixed(2)} ms`, onInput: (v) => { maxMs = Math.max(v, minMs); player.refresh(); } }),
      rate: slider({ label: "LFO", min: 0.02, max: 5, value: rate, log: true, format: (v) => `${v.toFixed(2)} Hz`, onInput: (v) => { rate = v; player.refresh(); } }),
      feedback: slider({ label: "Feedback", min: -0.95, max: 0.95, step: 0.01, value: feedback, format: (v) => v.toFixed(2), onInput: (v) => { feedback = v; player.refresh(); } }),
      mix: slider({ label: "Mistura", min: 0, max: 1, step: 0.01, value: mix, format: (v) => v.toFixed(2), onInput: (v) => { mix = v; player.refresh(); } }),
    };

    const applyPreset = (e: Effect) => {
      effect = e;
      ({ minMs, maxMs, rate, feedback, mix } = presets[e]);
      sliders.min.setValue(minMs);
      sliders.max.setValue(maxMs);
      sliders.rate.setValue(rate);
      sliders.feedback.setValue(feedback);
      sliders.mix.setValue(mix);
      player.refresh();
    };

    root.append(
      el("h2", {}, "1. O pente a mexer"),
      p("A resposta em frequência de `original + cópia atrasada` (com feedback), com o atraso a ser varrido pelo LFO em tempo real. Com atrasos de poucos ms os vales estão em todo o espectro audível (flanger); com 10–20 ms ficam tão juntos que deixam de se ouvir como vales — ouve-se a desafinação (chorus)."),
      panel(
        response.canvas,
        legend([{ label: "|H(f)|", color: "signal" }]),
        controls(
          select<Effect>("Predefinição", [ { value: "flanger", label: "flanger" }, { value: "chorus", label: "chorus" } ], effect, applyPreset),
          sliders.min, sliders.max, sliders.rate, sliders.feedback, sliders.mix,
        ),
        info,
      ),
      el("h2", {}, "2. Ouvir"),
      panel(controls(sourceSelect(kind, (k) => { kind = k; player.refresh(); }), playButton(player, "Ouvir"))),
      callout("Feedback negativo troca os picos com os vales; mistura 0.5 dá os vales mais fundos. O canal direito usa o LFO desfasado 90° (como nos exercícios 20 e 21)."),
      sources([
        { title: "J. O. Smith — PASP: Flanging", url: "https://ccrma.stanford.edu/~jos/pasp/Flanging.html", why: "o flanger como pente variável" },
        { title: "J. O. Smith — PASP: Chorus Effect", url: "https://ccrma.stanford.edu/~jos/pasp/Chorus_Effect.html", why: "o chorus como delay modulado" },
        { title: "juce::dsp::Chorus", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1Chorus.html", why: "a versão do JUCE" },
      ]),
    );

    update();
  },
};
