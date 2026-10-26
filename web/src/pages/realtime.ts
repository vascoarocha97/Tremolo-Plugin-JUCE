import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { button, controls, el, legend, p, panel, playButton, readout, slider, sources, toggle, callout } from "../lib/ui";
import { demoLoop, seededRandom } from "../lib/dsp";

interface Simulation {
  times: number[]; // tempo de processamento de cada bloco (ms)
  misses: boolean[];
}

export const realtimePage: PageDef = {
  id: "realtime",
  title: "Tempo-real e deadlines",
  module: 1,
  summary: "Cada bloco tem um prazo. Não interessa o tempo médio: interessa o pior caso. Uma alocação ou um lock no sítio errado chega para um click.",
  render(root) {
    let blockSize = 128;
    const sampleRate = 48000;
    let load = 40; // % do orçamento, em média
    let spikes = false;
    let seed = 1;
    const numBlocks = 60;

    const simulate = (): Simulation => {
      const rnd = seededRandom(seed);
      const budget = (1000 * blockSize) / sampleRate;
      const times: number[] = [];
      const misses: boolean[] = [];
      for (let i = 0; i < numBlocks; i++) {
        let t = budget * (load / 100) * (0.85 + 0.3 * rnd());
        // Um "pico": a thread de áudio alocou memória ou esperou por um lock da GUI
        if (spikes && rnd() < 0.08) t += 0.5 + 6 * rnd();
        times.push(t);
        misses.push(t > budget);
      }
      return { times, misses };
    };

    let sim = simulate();
    const info = readout();

    const plot = new Plot(el("div"), {
      height: 240,
      x: { min: -0.5, max: numBlocks - 0.5, label: "bloco", ticks: [0, 10, 20, 30, 40, 50] },
      y: { min: 0, max: 8, label: "ms", ticks: [0, 2, 4, 6, 8] },
      draw: (pl) => {
        const budget = (1000 * blockSize) / sampleRate;
        pl.y = { min: 0, max: Math.max(budget * 1.6, 1), label: "tempo de processamento (ms)" };
        pl.grid();
        sim.times.forEach((t, i) => pl.rect(i - 0.38, 0, i + 0.38, t, sim.misses[i] ? cssColor("warn") : cssColor("signal")));
        pl.hline(budget, cssColor("signal-2"), [6, 4], 2);
        pl.text(numBlocks - 1, budget * 1.04, `prazo: ${budget.toFixed(2)} ms`, cssColor("signal-2"), "right");
      },
    });

    const update = () => {
      sim = simulate();
      const misses = sim.misses.filter(Boolean).length;
      const average = sim.times.reduce((a, b) => a + b, 0) / sim.times.length;
      const worst = Math.max(...sim.times);
      info.set(
        `prazo por bloco = ${blockSize}/${sampleRate} = <strong>${((1000 * blockSize) / sampleRate).toFixed(2)} ms</strong> · ` +
          `médio <strong>${average.toFixed(2)} ms</strong> · pior <strong>${worst.toFixed(2)} ms</strong> · ` +
          `<strong style="color:var(--warn)">${misses}</strong> blocos atrasados de ${numBlocks}`,
      );
      plot.redraw();
    };

    // Áudio: o loop com os blocos atrasados substituídos por silêncio (é o que a placa de som toca)
    let loopCache: { rate: number; data: Float32Array[] } | null = null;
    const player = new LoopPlayer((rate) => {
      if (!loopCache || loopCache.rate !== rate) loopCache = { rate, data: demoLoop(rate) };
      const rnd = seededRandom(seed + 1000);
      const budget = blockSize / rate;
      const out = loopCache.data.map((ch) => ch.slice());
      for (let start = 0; start < out[0].length; start += blockSize) {
        let t = budget * (load / 100) * (0.85 + 0.3 * rnd());
        if (spikes && rnd() < 0.08) t += (0.5 + 6 * rnd()) / 1000;
        if (t > budget) for (const ch of out) ch.fill(0, start, Math.min(ch.length, start + blockSize));
      }
      return out.map((ch) => ch.map((v) => v * 0.6));
    });

    root.append(
      el("h2", {}, "1. O orçamento de cada bloco"),
      p("Cada barra é o tempo que o `processBlock` demorou num bloco. Se passar a linha laranja, a placa de som fica sem dados e toca silêncio (ou repete lixo): ouve-se um click. Liga os **picos** para simular uma alocação ou um lock de vez em quando."),
      panel(
        plot.canvas,
        legend([
          { label: "a tempo", color: "signal" },
          { label: "atrasado (dropout)", color: "warn" },
          { label: "prazo", color: "signal-2" },
        ]),
        controls(
          slider({ label: "Tamanho do bloco", min: 32, max: 1024, value: blockSize, log: true, step: 1, format: (v) => `${v} amostras`, onInput: (v) => { blockSize = v; update(); player.refresh(); } }),
          slider({ label: "Carga média", min: 5, max: 120, value: load, step: 1, format: (v) => `${v} %`, onInput: (v) => { load = v; update(); player.refresh(); } }),
          toggle("Picos (alocação, lock, I/O)", spikes, (v) => { spikes = v; update(); player.refresh(); }),
          button("Nova simulação", () => { seed++; update(); player.refresh(); }),
          playButton(player, "Ouvir o resultado"),
        ),
        info,
      ),
      callout("Blocos pequenos = menos latência, mas prazos mais curtos: o mesmo pico de 2 ms que passa despercebido com 1024 amostras (21 ms) é um dropout com 64 amostras (1.3 ms)."),
      el("h2", {}, "2. As regras"),
      el(
        "ul",
        {},
        el("li", {}, "Não alocar nem libertar memória (new/delete, push_back, juce::String...)."),
        el("li", {}, "Não bloquear em locks partilhados com outras threads (std::mutex, CriticalSection)."),
        el("li", {}, "Não fazer I/O (ficheiros, consola, rede, DBG)."),
        el("li", {}, "Não chamar código de tempo desconhecido nem esperar por outras threads."),
        el("li", {}, "Para comunicar: std::atomic para valores, FIFO lock-free (juce::AbstractFifo) para sequências."),
      ),
      sources([
        { title: "Ross Bencina — Real-time audio programming 101: time waits for nothing", url: "http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing", why: "as regras e porquê" },
        { title: "Rowland & Renn-Giles — Real time 101 (Meeting C++ 2019)", url: "https://www.youtube.com/watch?v=ndeN983j_GQ", why: "custos de alocações, locks e chamadas ao sistema" },
        { title: "JUCE — Real-time programming in audio development", url: "https://juce.com/posts/real-time-programming-in-audio-development/", why: "o mesmo tema no blog do JUCE" },
        { title: "juce::AbstractFifo — documentação", url: "https://docs.juce.com/master/classjuce_1_1AbstractFifo.html", why: "a FIFO lock-free do exercício 05" },
      ]),
    );

    update();
  },
};
