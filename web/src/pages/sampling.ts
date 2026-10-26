import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { animate } from "../lib/lifecycle";
import { controls, el, legend, p, panel, playButton, readout, slider, sources, callout } from "../lib/ui";
import { TWO_PI, sine } from "../lib/dsp";

export const samplingPage: PageDef = {
  id: "sampling",
  title: "Onda contínua vs. amostrada",
  module: 1,
  summary: "Amostrar é medir o sinal *fs* vezes por segundo. Um seno amostrado é uma fase que avança um passo fixo por amostra.",
  render(root) {
    let displayRate = 8000;
    let frequency = 440;

    const info = readout();
    const update = () => {
      const perPeriod = displayRate / frequency;
      const increment = (TWO_PI * frequency) / displayRate;
      info.set(
        `<strong>${perPeriod.toFixed(2)}</strong> amostras por período · incremento de fase = 2π·f/fs = <strong>${increment.toFixed(4)} rad</strong> (${((360 * frequency) / displayRate).toFixed(1)}°) por amostra`,
      );
      plot.redraw();
      circle.redraw();
    };

    // Janela de tempo: 3 períodos
    const plot = new Plot(el("div"), {
      height: 240,
      x: { min: 0, max: 1, label: "tempo (ms)" },
      y: { min: -1.2, max: 1.2, ticks: [-1, 0, 1] },
      draw: (pl) => {
        const windowMs = (3 * 1000) / frequency;
        pl.x = { min: 0, max: windowMs, label: "tempo (ms)" };
        pl.grid();
        pl.fn((tMs) => Math.sin((TWO_PI * frequency * tMs) / 1000), cssColor("text-muted"), 1.2, 900);
        const xs: number[] = [];
        const ys: number[] = [];
        for (let n = 0; n * 1000 / displayRate <= windowMs; n++) {
          xs.push((n * 1000) / displayRate);
          ys.push(Math.sin((TWO_PI * frequency * n) / displayRate));
        }
        pl.stems(xs, ys, cssColor("signal"), 3);
      },
    });

    let spin = 0;
    const circle = new Plot(el("div"), {
      height: 260,
      x: { min: -1.4, max: 1.4, ticks: [] },
      y: { min: -1.2, max: 1.2, ticks: [] },
      draw: (pl) => {
        const ctx = pl.ctx;
        const cx = pl.X(0), cy = pl.Y(0);
        const r = pl.Y(0) - pl.Y(1);
        ctx.strokeStyle = cssColor("grid");
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, TWO_PI);
        ctx.stroke();

        const increment = (TWO_PI * frequency) / displayRate;
        const shown = Math.min(24, Math.ceil(TWO_PI / increment) + 1);
        const current = Math.floor(spin) % 1000;
        for (let k = 0; k < shown; k++) {
          const phase = ((current - k) * increment) % TWO_PI;
          const alpha = 1 - k / shown;
          ctx.fillStyle = cssColor("signal");
          ctx.globalAlpha = Math.max(0.12, alpha);
          ctx.beginPath();
          ctx.arc(cx + r * Math.cos(phase), cy - r * Math.sin(phase), k === 0 ? 6 : 4, 0, TWO_PI);
          ctx.fill();
        }
        ctx.globalAlpha = 1;
        const phase = (current * increment) % TWO_PI;
        ctx.strokeStyle = cssColor("signal-2");
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(cx + r * Math.cos(phase), cy - r * Math.sin(phase));
        ctx.stroke();
        // projeção no eixo vertical = sin(fase) = a amostra
        ctx.setLineDash([3, 3]);
        ctx.beginPath();
        ctx.moveTo(cx + r * Math.cos(phase), cy - r * Math.sin(phase));
        ctx.lineTo(cx + r * 1.25, cy - r * Math.sin(phase));
        ctx.stroke();
        ctx.setLineDash([]);
        pl.text(1.02, Math.sin(phase) + 0.08, `sin(fase) = ${Math.sin(phase).toFixed(2)}`, cssColor("signal-2"));
      },
    });

    animate(() => {
      spin += 0.04; // ~2.4 amostras por segundo: muito devagar, para se ver cada passo
      circle.redraw();
    });

    const player = new LoopPlayer((rate) => [sine(frequency, rate, rate * 2, 0.4)]);

    root.append(
      el("h2", {}, "1. Medir o sinal fs vezes por segundo"),
      p("A linha cinzenta é o seno contínuo; os pontos são as amostras — o que o teu código realmente vê. Baixa o sample rate de demonstração e repara quantos pontos sobram por período."),
      panel(
        plot.canvas,
        legend([
          { label: "sinal contínuo", color: "text-muted" },
          { label: "amostras x[n]", color: "signal" },
        ]),
        controls(
          slider({ label: "Frequência", min: 50, max: 4000, value: frequency, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { frequency = v; update(); player.refresh(); } }),
          slider({ label: "Sample rate (demo)", min: 2000, max: 48000, value: displayRate, log: true, step: 100, format: (v) => `${v} Hz`, onInput: (v) => { displayRate = v; update(); } }),
          playButton(player, "Ouvir o seno"),
        ),
        info,
      ),
      el("h2", {}, "2. O acumulador de fase"),
      p("Um seno amostrado é um ponto a rodar num círculo: em cada amostra a fase avança `2π·f/fs`. A amostra é a projeção vertical, `sin(fase)`. O exercício 02 guarda esta fase entre blocos e mantém-na em [0, 2π)."),
      panel(circle.canvas),
      callout("Se a fase recomeçar a zero em cada bloco, o ponto salta — ouves um click a cada bloco. É o primeiro bug de quase toda a gente."),
      sources([
        { title: "J. O. Smith — Mathematics of the DFT: Sinusoids", url: "https://ccrma.stanford.edu/~jos/mdft/Sinusoids.html", why: "amplitude, frequência e fase" },
        { title: "J. O. Smith — Sampled Sinusoids", url: "https://ccrma.stanford.edu/~jos/mdft/Sampled_Sinusoids.html", why: "o que muda quando se amostra um seno" },
        { title: "Tutorial JUCE: Build a sine wave synthesiser", url: "https://juce.com/tutorials/tutorial_sine_synth/", why: "o acumulador de fase num sintetizador JUCE" },
      ]),
    );

    update();
  },
};
