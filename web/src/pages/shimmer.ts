import type { PageDef } from "./types";
import { Plot, audioFrequencyTicks, cssColor, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { callout, controls, el, legend, p, panel, playButton, readout, slider, sources } from "../lib/ui";
import { magnitudeSpectrumDb, sine } from "../lib/dsp";
import { PitchShifter, Shimmer, crossfadeGain, processLoopTwice } from "../lib/reverb";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;
const fftSize = 8192;
const windowMs = 50;

export const shimmerPage: PageDef = {
  id: "shimmer",
  title: "Shimmer",
  module: 7,
  summary: "Um pitch shifter de duas leituras dentro do ciclo de feedback de uma reverb: a cada volta, a cauda sobe uma oitava.",
  render(root) {
    // ---- 1. Mecânica do pitch shifter
    let semitones = 12;
    const shifterInfo = readout();
    const span = 160; // ms mostrados

    const delays = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: span, label: "tempo (ms)" },
      y: { min: 0, max: windowMs + 2, ticks: [0, 25, 50], format: (v) => `${v} ms`, label: "atraso" },
      draw: (pl) => draw(pl, "delay"),
    });
    const gains = new Plot(el("div"), {
      height: 160,
      x: { min: 0, max: span, label: "tempo (ms)" },
      y: { min: 0, max: 1.05, ticks: [0, 0.5, 1], label: "ganho" },
      draw: (pl) => draw(pl, "gain"),
    });

    function draw(pl: Plot, what: "delay" | "gain") {
      pl.grid();
      const shifter = new PitchShifter(fs, windowMs);
      shifter.setSemitones(semitones);
      const n = Math.floor((span / 1000) * fs);
      const step = 16;
      const xs: number[] = [];
      const a: number[] = [];
      const b: number[] = [];
      const sum: number[] = [];
      for (let i = 0; i < n; i++) {
        if (i % step === 0) {
          const pA = shifter.phase;
          const pB = pA + 0.5 >= 1 ? pA - 0.5 : pA + 0.5;
          xs.push((i / fs) * 1000);
          if (what === "delay") {
            a.push(pA * windowMs);
            b.push(pB * windowMs);
          } else {
            a.push(crossfadeGain(pA));
            b.push(crossfadeGain(pB));
            sum.push(crossfadeGain(pA) + crossfadeGain(pB));
          }
        }
        shifter.process(0);
      }
      // quebrar a linha nos saltos: desenha por segmentos
      const segmented = (ys: number[], color: string) => {
        let start = 0;
        for (let i = 1; i <= ys.length; i++) {
          if (i === ys.length || (what === "delay" && Math.abs(ys[i] - ys[i - 1]) > windowMs / 2)) {
            pl.line(xs.slice(start, i), ys.slice(start, i), color, 2);
            start = i;
          }
        }
      };
      segmented(a, cssColor("signal"));
      segmented(b, cssColor("signal-2"));
      if (what === "gain") pl.line(xs, sum, cssColor("text-muted"), 1.2, [5, 4]);
    }

    const updateShifter = () => {
      const ratio = Math.pow(2, semitones / 12);
      const period = ratio === 1 ? Infinity : windowMs / Math.abs(1 - ratio);
      shifterInfo.set(
        `razão <strong>${ratio.toFixed(3)}</strong> · o atraso muda <strong>${(1 - ratio).toFixed(3)}</strong> amostras por amostra · ` +
          (Number.isFinite(period) ? `cada leitura dá a volta a cada <strong>${period.toFixed(0)} ms</strong>` : "com 0 semitons o atraso fica parado"),
      );
      delays.redraw();
      gains.redraw();
    };

    // ---- 2. O espectro da cauda
    let amount = 0.7;
    let tailSemis = 12;
    const spectra: Float32Array[] = [];
    const times = [0.4, 1.5, 3.0];
    const colors = ["signal", "signal-2", "signal-3"];
    const spectrum = new Plot(el("div"), {
      height: 280,
      x: { min: 100, max: 12000, log: true, ticks: audioFrequencyTicks.filter((f) => f >= 100 && f <= 10000), format: formatHz },
      y: { min: -110, max: -10, ticks: [-100, -80, -60, -40, -20], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const freqs = Array.from({ length: fftSize / 2 }, (_, k) => Math.max(1, (k * fs) / fftSize));
        spectra.forEach((s, i) => pl.line(freqs, s, cssColor(colors[i]), 1.3));
        for (let k = 0; k <= 5; k++) pl.vline(220 * 2 ** k, cssColor("text-muted"), [2, 4]);
      },
    });
    const updateSpectrum = () => {
      const n = Math.floor(3.5 * fs);
      const shimmer = new Shimmer(fs, 6, amount, tailSemis);
      const burst = sine(220, fs, Math.floor(0.3 * fs), 0.5);
      const out = new Float32Array(n);
      for (let i = 0; i < n; i++) out[i] = shimmer.process(i < burst.length ? burst[i] : 0)[0];
      spectra.length = 0;
      for (const t of times) spectra.push(magnitudeSpectrumDb(out, fftSize, Math.floor(t * fs)));
      spectrum.redraw();
    };

    // ---- 3. Áudio
    let source: SourceKind = "loop";
    let rt60 = 6;
    let audioAmount = 0.6;
    let audioSemis = 12;
    let mix = 0.4;
    const player = new LoopPlayer((rate) => {
      const shimmer = new Shimmer(rate, rt60, audioAmount, audioSemis);
      return processLoopTwice(renderSource(source, rate, 0.4), mix, (l, r) => shimmer.process(0.5 * (l + r)));
    });

    root.append(
      el("h2", {}, "1. Pitch shift com duas leituras"),
      p("A leitura anda à velocidade `ratio`; por isso o atraso muda (1 − ratio) amostras por amostra até chegar ao fim da janela (50 ms) e saltar. Há **duas** leituras desfasadas de meia janela; cada uma é pesada por sin²(π·fase), que é 0 exatamente no salto. Os dois ganhos somam sempre 1 (tracejado)."),
      panel(
        delays.canvas,
        gains.canvas,
        legend([{ label: "leitura A", color: "signal" }, { label: "leitura B", color: "signal-2" }, { label: "soma dos ganhos", color: "text-muted" }]),
        controls(slider({ label: "Semitons", min: -12, max: 12, step: 1, value: semitones, format: (v) => `${v > 0 ? "+" : ""}${v}`, onInput: (v) => { semitones = v; updateShifter(); } })),
        shifterInfo,
      ),
      el("h2", {}, "2. A cauda sobe"),
      p("Um seno de 220 Hz durante 0.3 s entra na reverb com shimmer (RT60 = 6 s). Espectro da saída em três instantes: a energia passa para 440, 880, 1760 Hz... — uma oitava por cada volta ao ciclo. As linhas verticais marcam as oitavas de 220 Hz. O passa-baixo no ciclo apaga as oitavas mais agudas."),
      panel(
        spectrum.canvas,
        legend(times.map((t, i) => ({ label: `aos ${t} s`, color: colors[i] }))),
        controls(
          slider({ label: "Shimmer", min: 0, max: 1, step: 0.01, value: amount, format: (v) => v.toFixed(2), onInput: (v) => { amount = v; updateSpectrum(); } }),
          slider({ label: "Semitons", min: -12, max: 12, step: 1, value: tailSemis, format: (v) => `${v > 0 ? "+" : ""}${v}`, onInput: (v) => { tailSemis = v; updateSpectrum(); } }),
        ),
      ),
      el("h2", {}, "3. Ouvir"),
      panel(
        controls(
          sourceSelect(source, (v) => { source = v; player.refresh(); }),
          slider({ label: "RT60", min: 1, max: 15, step: 0.5, value: rt60, format: (v) => `${v.toFixed(1)} s`, onInput: (v) => { rt60 = v; player.refresh(); } }),
          slider({ label: "Shimmer", min: 0, max: 1, step: 0.01, value: audioAmount, format: (v) => v.toFixed(2), onInput: (v) => { audioAmount = v; player.refresh(); } }),
          slider({ label: "Semitons", min: -12, max: 12, step: 1, value: audioSemis, format: (v) => `${v > 0 ? "+" : ""}${v}`, onInput: (v) => { audioSemis = v; player.refresh(); } }),
          slider({ label: "Mistura", min: 0, max: 1, step: 0.01, value: mix, format: (v) => v.toFixed(2), onInput: (v) => { mix = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Sem o `tanh` no ciclo, shimmer perto de 1 com RT60 longo pode crescer sem limite. Num plugin, uma saída sem limite é um bug — e um risco para os ouvidos de quem está a usar auscultadores.", "warn"),
      sources([
        { title: "Valhalla DSP — ValhallaShimmer", url: "https://valhalladsp.com/shop/reverb/valhalla-shimmer/", why: "pitch shift no ciclo de feedback; o som \"Eno/Lanois\" com +12 semitons" },
        { title: "Signalsmith Audio — código de \"Four Ways To Write A Pitch-Shifter\" (ADC22)", url: "https://github.com/Signalsmith-Audio/pitch-time-example-code", why: "pitch shifters melhores do que o de duas leituras" },
        { title: "Signalsmith Audio — Let's Write A Reverb", url: "https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/", why: "a FDN usada no ciclo" },
      ]),
    );

    updateShifter();
    updateSpectrum();
  },
};
