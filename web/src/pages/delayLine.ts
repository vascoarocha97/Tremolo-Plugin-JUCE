import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { animate } from "../lib/lifecycle";
import { button, controls, el, legend, p, panel, readout, slider, sources, callout } from "../lib/ui";
import { TWO_PI } from "../lib/dsp";

export const delayLinePage: PageDef = {
  id: "delay-line",
  title: "Delay line circular",
  module: 3,
  summary: "Um array de tamanho fixo, um índice de escrita que dá a volta, e uma leitura D posições atrás.",
  render(root) {
    let size = 16;
    let delay = 5;
    let running = true;
    let speed = 3; // amostras por segundo
    let n = 0; // número de pushes feitos
    let accumulator = 0;
    let last = 0;

    const buffer = new Float32Array(64);
    const input: number[] = [];
    const output: number[] = [];
    let writeIndex = 0;

    const signalAt = (k: number) => Math.sin((TWO_PI * k) / 13) * 0.8 + 0.15 * Math.sin((TWO_PI * k) / 3.7);

    const push = () => {
      const x = signalAt(n);
      writeIndex = (writeIndex + 1) % size;
      buffer[writeIndex] = x;
      let r = writeIndex - delay;
      if (r < 0) r += size;
      input.push(x);
      output.push(buffer[r]);
      if (input.length > 48) {
        input.shift();
        output.shift();
      }
      n++;
    };

    const reset = () => {
      buffer.fill(0);
      writeIndex = 0;
      n = 0;
      input.length = 0;
      output.length = 0;
    };

    const info = readout();

    const ring = new Plot(el("div"), {
      height: 300,
      x: { min: -1.6, max: 1.6, ticks: [] },
      y: { min: -1.25, max: 1.25, ticks: [] },
      draw: (pl) => {
        const ctx = pl.ctx;
        const cx = pl.X(0), cy = pl.Y(0);
        const radius = (pl.Y(0) - pl.Y(1)) * 0.85;
        const slot = TWO_PI / size;
        let readIndex = writeIndex - delay;
        if (readIndex < 0) readIndex += size;

        for (let i = 0; i < size; i++) {
          const a0 = -Math.PI / 2 + i * slot;
          const value = buffer[i];
          // fatia do anel: cor = valor
          ctx.beginPath();
          ctx.arc(cx, cy, radius, a0, a0 + slot * 0.92);
          ctx.arc(cx, cy, radius * 0.62, a0 + slot * 0.92, a0, true);
          ctx.closePath();
          ctx.fillStyle = value >= 0 ? `rgba(56,189,248,${0.15 + 0.85 * Math.min(1, Math.abs(value))})` : `rgba(255,138,76,${0.15 + 0.85 * Math.min(1, Math.abs(value))})`;
          ctx.fill();
          ctx.strokeStyle = i === writeIndex ? cssColor("signal-3") : i === readIndex ? cssColor("warn") : cssColor("grid");
          ctx.lineWidth = i === writeIndex || i === readIndex ? 3 : 1;
          ctx.stroke();

          const mid = a0 + slot * 0.46;
          ctx.fillStyle = cssColor("text-muted");
          ctx.font = "11px sans-serif";
          ctx.textAlign = "center";
          ctx.fillText(String(i), cx + Math.cos(mid) * radius * 1.12, cy + Math.sin(mid) * radius * 1.12 + 4);
        }

        ctx.fillStyle = cssColor("text");
        ctx.font = "13px sans-serif";
        ctx.textAlign = "center";
        ctx.fillText(`escrita: ${writeIndex}`, cx, cy - 8);
        ctx.fillText(`leitura: ${readIndex}`, cx, cy + 14);
      },
    });

    const signals = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: 47, label: "amostras recentes" },
      y: { min: -1.1, max: 1.1, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        pl.stems(null, input, cssColor("signal"), 2.5);
        pl.stems(null, output.map((v) => v), cssColor("signal-2"), 2.5);
      },
    });

    const updateInfo = () => {
      info.set(
        `tamanho do buffer = <strong>${size}</strong> → atraso máximo = <strong>${size - 1}</strong> · ` +
          `atraso D = <strong>${delay}</strong>: leitura = escrita − D (se der negativo, soma-se o tamanho)`,
      );
    };

    animate((t) => {
      const dt = last ? t - last : 0;
      last = t;
      if (running) {
        accumulator += dt * speed;
        while (accumulator >= 1) {
          push();
          accumulator -= 1;
        }
      }
      ring.redraw();
      signals.redraw();
    });

    const delaySlider = slider({ label: "Atraso D", min: 0, max: 15, step: 1, value: delay, format: (v) => `${v} amostras`, onInput: (v) => { delay = Math.min(v, size - 1); updateInfo(); } });

    root.append(
      el("h2", {}, "1. O buffer circular por dentro"),
      p("Cada fatia é uma posição do array (cor = valor da amostra guardada). O contorno **verde** é a posição de escrita (a amostra mais recente); o **vermelho** é a leitura, D posições atrás. Quando a escrita chega ao fim, volta à posição 0 e escreve por cima da amostra mais antiga."),
      panel(
        ring.canvas,
        controls(
          slider({ label: "Tamanho do buffer", min: 4, max: 32, step: 1, value: size, format: (v) => `${v}`, onInput: (v) => { size = v; delay = Math.min(delay, size - 1); delaySlider.setValue(delay); reset(); updateInfo(); } }),
          delaySlider,
          slider({ label: "Velocidade", min: 0.5, max: 30, value: speed, log: true, format: (v) => `${v.toFixed(1)} amostras/s`, onInput: (v) => { speed = v; } }),
          button("Pausa / continuar", () => { running = !running; }),
          button("Um passo", () => { push(); }),
          button("Limpar", reset),
        ),
        info,
      ),
      el("h2", {}, "2. Entrada e saída"),
      p("A saída (laranja) é a entrada (azul) deslocada exatamente D amostras. Nas primeiras D amostras a saída é 0: o buffer ainda não tinha nada."),
      panel(
        signals.canvas,
        legend([
          { label: "entrada x[n]", color: "signal" },
          { label: "saída y[n] = x[n − D]", color: "signal-2" },
        ]),
      ),
      callout("Em C++, `-3 % 16` dá `-3` (não 13). Por isso o *wrap* da leitura faz-se com `if (r < 0) r += tamanho` — ou com um tamanho potência de 2 e uma máscara `& (tamanho − 1)`."),
      sources([
        { title: "J. O. Smith — PASP: Delay Lines", url: "https://ccrma.stanford.edu/~jos/pasp/Delay_Lines.html", why: "a delay line como bloco fundamental" },
        { title: "juce::dsp::DelayLine — documentação", url: "https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html", why: "a versão do JUCE" },
      ]),
    );

    updateInfo();
  },
};
