import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { animate } from "../lib/lifecycle";
import { controls, el, legend, p, panel, readout, slider, sources, callout } from "../lib/ui";
import { TWO_PI } from "../lib/dsp";

export const buffersPage: PageDef = {
  id: "buffers",
  title: "Buffers e blocos",
  module: 1,
  summary: "Como o áudio chega ao teu código: um `AudioBuffer` de canais × amostras, um bloco de cada vez.",
  render(root) {
    // ---- 1. Canais × amostras -----------------------------------------------------------------
    let blockSize = 32;

    const bufferPlot = new Plot(el("div"), {
      height: 240,
      x: { min: -0.5, max: 31.5, label: "índice da amostra n" },
      y: { min: -2.4, max: 2.4, ticks: [-1.2, 0, 1.2], format: (v) => (v > 0 ? "canal 0" : v < 0 ? "canal 1" : "") },
      draw: (pl) => {
        pl.grid();
        const left: number[] = [];
        const right: number[] = [];
        const xs: number[] = [];
        for (let n = 0; n < blockSize; n++) {
          xs.push(n);
          left.push(1.2 + 0.9 * Math.sin((TWO_PI * n) / 16));
          right.push(-1.2 + 0.9 * Math.sin((TWO_PI * n) / 11 + 1));
        }
        pl.hline(1.2, cssColor("grid"), []);
        pl.hline(-1.2, cssColor("grid"), []);
        pl.stems(xs, left, cssColor("signal"), 2.5);
        pl.ctx.save();
        pl.stems(xs, right, cssColor("signal-2"), 2.5);
        pl.ctx.restore();
        pl.text(0, 2.2, "getWritePointer (0)[n]", cssColor("signal"));
        pl.text(0, -0.1, "getWritePointer (1)[n]", cssColor("signal-2"));
      },
    });

    const blockSlider = slider({
      label: "Amostras no bloco (getNumSamples)",
      min: 8,
      max: 64,
      step: 1,
      value: blockSize,
      format: (v) => `${v}`,
      onInput: (v) => {
        blockSize = v;
        bufferPlot.setAxes({ ...bufferPlot.x, max: v - 0.5 });
      },
    });

    root.append(
      el("h2", {}, "1. Um buffer é uma tabela canais × amostras"),
      p("Cada canal é um array contíguo de `float` (o JUCE não intercala L R L R). No exercício 01 percorres esta tabela com dois `for`: canais por fora, amostras por dentro."),
      panel(bufferPlot.canvas, controls(blockSlider)),
    );

    // ---- 2. O callback: um bloco de cada vez --------------------------------------------------
    let callbackBlock = 480;
    let sampleRate = 48000;

    const info = readout();
    const updateInfo = () => {
      const ms = (1000 * callbackBlock) / sampleRate;
      info.set(
        `Cada bloco dura <strong>${ms.toFixed(2)} ms</strong> · o callback é chamado <strong>${(sampleRate / callbackBlock).toFixed(0)}×</strong> por segundo · ` +
          `com dois buffers (um a tocar, um a preparar) a latência de saída é pelo menos <strong>${(2 * ms).toFixed(1)} ms</strong>`,
      );
    };

    let time = 0;
    let last = 0;
    const timeline = new Plot(el("div"), {
      height: 200,
      x: { min: 0, max: 40, label: "tempo (ms)" },
      y: { min: -1.3, max: 1.3, ticks: [-1, 0, 1] },
      draw: (pl) => {
        pl.grid();
        const blockMs = (1000 * callbackBlock) / sampleRate;
        // posição "agora": o bloco que está a tocar e o próximo que o processBlock está a preparar
        const now = time % 40;
        const playingStart = Math.floor(now / blockMs) * blockMs;
        pl.rect(playingStart, -1.3, playingStart + blockMs, 1.3, "rgba(56,189,248,0.18)");
        pl.rect(playingStart + blockMs, -1.3, playingStart + 2 * blockMs, 1.3, "rgba(255,138,76,0.18)");
        for (let t = 0; t <= 40; t += blockMs) pl.vline(t, cssColor("text-muted"), [2, 3]);
        pl.fn((t) => Math.sin(TWO_PI * 0.15 * t) * 0.9, cssColor("signal"), 1.6, 800);
        pl.vline(now, cssColor("warn"), [], 2);
      },
    });

    animate((t) => {
      const dt = last ? t - last : 0;
      last = t;
      time += dt * 4; // 4 ms de áudio por segundo real: câmara lenta
      timeline.redraw();
    });

    updateInfo();
    root.append(
      el("h2", {}, "2. A placa de som pede blocos"),
      p("O sistema chama o teu `processBlock` na thread de áudio. Enquanto um bloco toca (azul), preparas o seguinte (laranja). A linha vermelha é o \"agora\" (em câmara lenta: 4 ms por segundo)."),
      panel(
        timeline.canvas,
        legend([
          { label: "bloco a tocar", color: "signal" },
          { label: "bloco a preparar (processBlock)", color: "signal-2" },
        ]),
        controls(
          slider({ label: "Tamanho do bloco", min: 32, max: 2048, value: callbackBlock, log: true, step: 1, format: (v) => `${v} amostras`, onInput: (v) => { callbackBlock = v; updateInfo(); } }),
          slider({ label: "Sample rate", min: 44100, max: 96000, value: sampleRate, step: 100, format: (v) => `${v} Hz`, onInput: (v) => { sampleRate = v; updateInfo(); } }),
        ),
        info,
      ),
    );

    // ---- 3. Blocos de tamanho variável --------------------------------------------------------
    const sizes = [512, 512, 37, 475, 512, 0, 512, 256, 256, 512, 1, 511];
    const sizesPlot = new Plot(el("div"), {
      height: 170,
      x: { min: -0.5, max: sizes.length - 0.5, ticks: sizes.map((_, i) => i), format: (v) => `${v + 1}` },
      y: { min: 0, max: 600, ticks: [0, 256, 512], label: "amostras" },
      draw: (pl) => {
        pl.grid();
        pl.hline(512, cssColor("warn"), [5, 4]);
        sizes.forEach((s, i) => pl.rect(i - 0.35, 0, i + 0.35, s, s === 512 ? cssColor("signal") : cssColor("signal-2")));
        pl.text(sizes.length - 1.5, 540, "máximo anunciado no prepareToPlay", cssColor("warn"), "right");
      },
    });

    root.append(
      el("h2", {}, "3. O tamanho do bloco não é fixo"),
      p("A documentação do `AudioProcessor::processBlock` avisa: o número de amostras pode mudar em cada chamada, ser menor (ou até maior, num host com bugs) do que o máximo do `prepareToPlay`, e alguns hosts mandam blocos de **0 amostras**. Um host pode, por exemplo, partir o bloco para aplicar automação a meio:"),
      panel(sizesPlot.canvas),
      callout("Regra: usa sempre `buffer.getNumSamples()` e `buffer.getNumChannels()`. Os testes do curso processam blocos de 1, 7, 13, 333... amostras de propósito."),
      sources([
        { title: "juce::AudioBuffer — documentação", url: "https://docs.juce.com/master/classjuce_1_1AudioBuffer.html", why: "layout canais × amostras, getReadPointer/getWritePointer" },
        { title: "juce::AudioProcessor — documentação", url: "https://docs.juce.com/master/classjuce_1_1AudioProcessor.html", why: "prepareToPlay e processBlock, blocos de tamanho variável" },
        { title: "Ross Bencina — Real-time audio programming 101", url: "http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing", why: "porque é que o callback tem um prazo" },
      ]),
    );
  },
};
