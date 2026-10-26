// ⚠ SPOILER: este ficheiro implementa em TypeScript a matemática de um ou mais exercícios C++.
//   Ler o código antes de fazeres o exercício equivale a ver a solução; usa a página no browser.
import type { PageDef } from "./types";
import { Plot, audioFrequencyTicks, cssColor, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { onDispose } from "../lib/lifecycle";
import { callout, controls, el, legend, p, panel, playButton, readout, slider, sources, toggle } from "../lib/ui";
import { magnitudeSpectrumDb, oversampled, onePole } from "../lib/dsp";
import { DiffuserFdn } from "../lib/reverb";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

interface PedalSettings {
  fuzzOn: boolean;
  drive: number;
  tone: number;
  crushOn: boolean;
  bits: number;
  downsample: number;
  glitchOn: boolean;
  sliceMs: number;
  reverbMix: number;
  decay: number;
  oversampling: boolean;
}

/** Glitch, ligado entre `from` e `to` (amostras), com pré-rolo e crossfades. */
function glitch(x: Float32Array, from: number, to: number, sliceLength: number, fade: number): Float32Array {
  const out = x.slice();
  if (from - sliceLength - fade < 0) return out;
  const slice = x.slice(from - sliceLength - fade, from); // [pré-rolo][fatia]
  for (let n = from; n < Math.min(to + fade, x.length); n++) {
    const pos = (n - from) % sliceLength;
    let loop = slice[fade + pos];
    if (pos >= sliceLength - fade) {
      const k = pos - (sliceLength - fade);
      const w = (k + 1) / (fade + 1);
      loop = (1 - w) * loop + w * slice[k];
    }
    const g = n < from + fade ? (n - from) / fade : n >= to ? 1 - (n - to) / fade : 1;
    out[n] = (1 - g) * x[n] + g * loop;
  }
  return out;
}

function renderPedal(input: Float32Array[], rate: number, s: PedalSettings): Float32Array[] {
  const drive = Math.pow(10, s.drive / 20);
  let channels = input.map((ch) => {
    let y = ch;
    if (s.fuzzOn) {
      y = oversampled(y, s.oversampling ? 4 : 1, (v) => 0.5 * Math.tanh(drive * v));
      y = onePole(y, s.tone, rate);
    }
    if (s.crushOn) {
      const levels = Math.pow(2, s.bits - 1);
      const out = new Float32Array(y.length);
      let held = 0;
      for (let n = 0; n < y.length; n++) {
        if (n % s.downsample === 0) held = Math.round(Math.max(-1, Math.min(1, y[n])) * levels) / levels;
        out[n] = held;
      }
      y = out;
    }
    if (s.glitchOn) {
      // No loop de demonstração: o glitch liga a meio e desliga a 3/4
      y = glitch(y, Math.floor(y.length * 0.5), Math.floor(y.length * 0.75), Math.round((s.sliceMs / 1000) * rate), Math.round(0.004 * rate));
    }
    return y;
  });

  if (s.reverbMix > 0) {
    const reverb = new DiffuserFdn(rate, s.decay);
    const length = channels[0].length;
    const out = [new Float32Array(length), new Float32Array(length)];
    for (let pass = 0; pass < 2; pass++) {
      for (let n = 0; n < length; n++) {
        const l = channels[0][n];
        const r = channels[1 % channels.length][n];
        const [wl, wr] = reverb.process(l, r);
        if (pass === 1) {
          out[0][n] = (1 - s.reverbMix) * l + s.reverbMix * wl;
          out[1][n] = (1 - s.reverbMix) * r + s.reverbMix * wr;
        }
      }
    }
    channels = out;
  }
  return channels;
}

export const pedalPage: PageDef = {
  id: "pedal",
  title: "O pedal",
  module: 10,
  summary: "Fuzz com oversampling, bitcrusher, glitch e reverb numa só cadeia — o projeto final. Liga e desliga cada secção e ouve o que cada uma acrescenta.",
  render(root) {
    const s: PedalSettings = {
      fuzzOn: true, drive: 20, tone: 4000,
      crushOn: false, bits: 8, downsample: 1,
      glitchOn: false, sliceMs: 125,
      reverbMix: 0.25, decay: 2.5,
      oversampling: true,
    };
    let source: SourceKind = "loop";
    const fs = 48000;
    let spectrum: Float32Array = new Float32Array(0);
    let inputSpectrum: Float32Array = new Float32Array(0);
    const info = readout();

    const plot = new Plot(el("div"), {
      height: 280,
      x: { min: 30, max: 22000, log: true, ticks: audioFrequencyTicks.filter((f) => f >= 50), format: formatHz },
      y: { min: -110, max: -10, ticks: [-100, -80, -60, -40, -20], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const n = spectrum.length * 2;
        const freqs = Array.from({ length: n / 2 }, (_, k) => Math.max(1, (k * fs) / n));
        if (inputSpectrum.length) pl.line(freqs, inputSpectrum, cssColor("text-muted"), 1);
        if (spectrum.length) pl.line(freqs, spectrum, cssColor("signal"), 1.3);
      },
    });

    let timer = 0;
    onDispose(() => window.clearTimeout(timer));
    const analyse = () => {
      window.clearTimeout(timer);
      timer = window.setTimeout(() => {
        // Análise sobre 0.5 s de uma fonte estável (seno 440 Hz ou ruído): o espectro diz mais do que o loop
        const kind: SourceKind = source === "loop" ? "sine" : source;
        const input = renderSource(kind, fs, 0.5).map((ch) => ch.subarray(0, fs / 2));
        const t0 = performance.now();
        const out = renderPedal(input, fs, { ...s, glitchOn: false });
        const ms = performance.now() - t0;
        spectrum = magnitudeSpectrumDb(out[0], 8192, 8192);
        inputSpectrum = magnitudeSpectrumDb(input[0], 8192, 8192);
        info.set(
          `espectro da saída (${kind === "sine" ? "seno 440 Hz" : kind}) a cinzento a entrada · ` +
            `fuzz ${s.fuzzOn ? (s.oversampling ? "com oversampling 4×" : "<strong>sem</strong> oversampling (procura as riscas de aliasing)") : "desligado"} · ` +
            `calculado em ${ms.toFixed(0)} ms (JavaScript, sem SIMD — o teu C++ faz isto em tempo real)`,
        );
        plot.redraw();
      }, 120);
    };

    const player = new LoopPlayer((rate) => renderPedal(renderSource(source, rate, 0.45), rate, s));
    const changed = () => {
      analyse();
      player.refresh();
    };

    const section = (title: string, ...children: HTMLElement[]) => el("div", { class: "pedal-section" }, el("h3", {}, title), ...children);

    root.append(
      p("A ordem importa: o **fuzz** gera harmónicos (e aliasing, se não houver oversampling); o **crush** reduz resolução e sample rate; o **glitch** repete fatias; a **reverb** no fim espalha tudo no espaço (e disfarça as junções do glitch)."),
      el(
        "div",
        { class: "pedal" },
        section("1 · Fuzz",
          toggle("ligado", s.fuzzOn, (v) => { s.fuzzOn = v; changed(); }),
          slider({ label: "Drive", min: 0, max: 40, step: 0.5, value: s.drive, format: (v) => `${v} dB`, onInput: (v) => { s.drive = v; changed(); } }),
          slider({ label: "Tom", min: 500, max: 12000, step: 10, value: s.tone, log: true, format: (v) => `${Math.round(v)} Hz`, onInput: (v) => { s.tone = v; changed(); } }),
          toggle("oversampling 4×", s.oversampling, (v) => { s.oversampling = v; changed(); })),
        section("2 · Crush",
          toggle("ligado", s.crushOn, (v) => { s.crushOn = v; changed(); }),
          slider({ label: "Bits", min: 2, max: 16, step: 1, value: s.bits, format: (v) => String(v), onInput: (v) => { s.bits = v; changed(); } }),
          slider({ label: "Downsample", min: 1, max: 32, step: 1, value: s.downsample, format: (v) => `${v}×`, onInput: (v) => { s.downsample = v; changed(); } })),
        section("3 · Glitch",
          toggle("ligado (2.º quarto do loop)", s.glitchOn, (v) => { s.glitchOn = v; changed(); }),
          slider({ label: "Fatia", min: 20, max: 1000, step: 1, value: s.sliceMs, log: true, format: (v) => `${Math.round(v)} ms`, onInput: (v) => { s.sliceMs = v; changed(); } })),
        section("4 · Reverb",
          slider({ label: "Mistura", min: 0, max: 1, step: 0.01, value: s.reverbMix, format: (v) => v.toFixed(2), onInput: (v) => { s.reverbMix = v; changed(); } }),
          slider({ label: "Decay", min: 0.2, max: 20, step: 0.1, value: s.decay, log: true, format: (v) => `${v.toFixed(1)} s`, onInput: (v) => { s.decay = v; changed(); } })),
      ),
      panel(controls(sourceSelect(source, (v) => { source = v; changed(); }), playButton(player, "Ouvir o pedal"))),
      el("h2", {}, "O que cada secção faz ao espectro"),
      panel(plot.canvas, legend([{ label: "entrada", color: "text-muted" }, { label: "saída do pedal", color: "signal" }]), info),
      callout("No plugin (exercício 40) há mais duas coisas que aqui não se veem: a **latência** do oversampling (reportada ao host e igual no bypass) e o **bypass sem cliques**. Os testes do exercício verificam ambas."),
      sources([
        { title: "juce::dsp::Oversampling — documentação", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html", why: "o fuzz sem aliasing" },
        { title: "Signalsmith Audio — Let's Write A Reverb", url: "https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/", why: "a reverb do pedal" },
        { title: "pluginval (Tracktion)", url: "https://github.com/Tracktion/pluginval", why: "validar o VST3 final" },
      ]),
    );

    analyse();
  },
};
