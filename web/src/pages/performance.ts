// ⚠ SPOILER: este ficheiro implementa em TypeScript a matemática de um ou mais exercícios C++.
//   Ler o código antes de fazeres o exercício equivale a ver a solução; usa a página no browser.
import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { button, callout, controls, el, legend, p, panel, playButton, readout, select, slider, sources, toggle } from "../lib/ui";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const FLT_MIN = 1.1754943508222875e-38; // menor float normal
const DENORM_MIN = 1.401298464324817e-45; // menor float desnormalizado

/** Versões JS do exercício 36: a "ingénua" recalcula tudo por amostra; a outra só por bloco. */
function naiveBlock(data: Float32Array[], state: number[], driveDb: number, cutoff: number, gainDb: number, fs: number) {
  const n = data[0].length;
  for (let i = 0; i < n; i++) {
    for (let ch = 0; ch < data.length; ch++) {
      const drive = Math.pow(10, driveDb / 20);
      const a = Math.exp((-2 * Math.PI * cutoff) / fs);
      const gain = Math.pow(10, gainDb / 20);
      const x = data[ch][i] * drive;
      state[ch] = (1 - a) * x + a * state[ch];
      const c = Math.max(-1.5, Math.min(1.5, state[ch]));
      data[ch][i] = (c - (4 / 27) * c * c * c) * gain;
    }
  }
}

function fastBlock(data: Float32Array[], state: number[], driveDb: number, cutoff: number, gainDb: number, fs: number) {
  const drive = Math.pow(10, driveDb / 20);
  const a = Math.exp((-2 * Math.PI * cutoff) / fs);
  const b = 1 - a;
  const gain = Math.pow(10, gainDb / 20);
  for (let ch = 0; ch < data.length; ch++) {
    const x = data[ch];
    let s = state[ch];
    for (let i = 0; i < x.length; i++) {
      s = b * (x[i] * drive) + a * s;
      const c = Math.max(-1.5, Math.min(1.5, s));
      x[i] = (c - (4 / 27) * c * c * c) * gain;
    }
    state[ch] = s;
  }
}

export const performancePage: PageDef = {
  id: "performance",
  title: "Performance",
  module: 9,
  summary: "Cada bloco tem um orçamento de tempo. Medir, não fazer trabalho repetido, usar SIMD e fugir dos denormais.",
  render(root) {
    // ---- 1. Orçamento e medição -------------------------------------------------------------------
    let fs = 48000;
    let block = 512;
    const budgetInfo = readout();
    const measured = { naive: 0, fast: 0 };

    const bars = new Plot(el("div"), {
      height: 150,
      x: { min: 0, max: 100, ticks: [0, 25, 50, 75, 100], format: (v) => `${v}%`, label: "carga (% do orçamento de um bloco)" },
      y: { min: 0, max: 3, ticks: [] },
      draw: (pl) => {
        pl.grid();
        const budgetMs = (block / fs) * 1000;
        const rows: Array<[string, number, string]> = [
          ["ingénua", measured.naive, "signal-2"],
          ["otimizada", measured.fast, "signal"],
        ];
        rows.forEach(([label, ms, color], i) => {
          const pct = budgetMs > 0 ? (100 * ms) / budgetMs : 0;
          const y0 = 2 - i - 0.2;
          pl.rect(0, y0 - 0.6, Math.min(100, Math.max(pct, 0.3)), y0, cssColor(color));
          pl.text(Math.min(70, Math.max(pct, 0.3)) + 1, y0 - 0.25, `${label}: ${ms > 0 ? (pct < 0.1 ? pct.toFixed(3) : pct.toFixed(2)) + "%" : "—"}`, cssColor("text"));
        });
      },
    });

    const updateBudget = () => {
      const budgetMs = (block / fs) * 1000;
      budgetInfo.set(
        `${block} amostras a ${fs / 1000} kHz → orçamento de <strong>${budgetMs.toFixed(2)} ms</strong> por bloco ` +
          `(<strong>${(1000 / budgetMs).toFixed(0)}</strong> blocos por segundo)` +
          (measured.naive > 0 ? ` · ingénua <strong>${(measured.naive * 1000).toFixed(1)} µs</strong>, otimizada <strong>${(measured.fast * 1000).toFixed(1)} µs</strong> por bloco (${(measured.naive / Math.max(measured.fast, 1e-6)).toFixed(1)}×)` : ""),
      );
      bars.redraw();
    };

    const measure = () => {
      const blocks = Math.max(50, Math.round((2 * fs) / block)); // ~2 s de áudio estéreo
      const data = [new Float32Array(block), new Float32Array(block)];
      for (const ch of data) for (let i = 0; i < ch.length; i++) ch[i] = 0.5 * Math.sin(i * 0.05);
      const run = (fn: typeof naiveBlock) => {
        const state = [0, 0];
        let best = Infinity;
        for (let r = 0; r < 3; r++) {
          const t0 = performance.now();
          for (let b = 0; b < blocks; b++) fn(data, state, 12, 3000, -6, fs);
          best = Math.min(best, (performance.now() - t0) / blocks);
        }
        return best;
      };
      measured.naive = run(naiveBlock);
      measured.fast = run(fastBlock);
      updateBudget();
    };

    // ---- 2. Ouvir um underrun -------------------------------------------------------------------
    let load = 0.8;
    let audioBlock = 512;
    let source: SourceKind = "loop";
    const player = new LoopPlayer((rate) => {
      const input = renderSource(source, rate, 0.5);
      if (load <= 1) return input;
      // Com carga > 100%, a placa de som fica sem amostras no fim de cada bloco: silêncio (buracos).
      const missing = Math.min(audioBlock, Math.round(audioBlock * (1 - 1 / load)));
      return input.map((ch) => {
        const out = ch.slice();
        for (let start = 0; start < out.length; start += audioBlock)
          for (let i = start + audioBlock - missing; i < Math.min(start + audioBlock, out.length); i++) out[i] = 0;
        return out;
      });
    });

    // ---- 3. Denormais ---------------------------------------------------------------------------
    let pole = 0.999;
    let ftz = false;
    const denormInfo = readout();
    const denorm = new Plot(el("div"), {
      height: 260,
      x: { min: 0, max: 140000, label: "amostras depois de a entrada parar" },
      y: { min: -46, max: 0, ticks: [-45, -38, -30, -20, -10, 0], format: (v) => `1e${v}` },
      draw: (pl) => {
        pl.grid();
        const maxN = pl.x.max;
        pl.rect(0, Math.log10(DENORM_MIN), maxN, Math.log10(FLT_MIN), cssColor("accent-soft"));
        pl.text(maxN * 0.01, -40.5, "zona desnormalizada (lenta)", cssColor("text-muted"));
        const step = Math.max(1, Math.round(maxN / 1200));
        const xs: number[] = [];
        const ys: number[] = [];
        let s = Math.fround(1);
        let stuckAt = -1;
        for (let n = 0; n <= maxN; n++) {
          if (n % step === 0) {
            xs.push(n);
            ys.push(s === 0 ? -46 : Math.log10(Math.abs(s)));
          }
          const next = Math.fround(Math.fround(pole) * s);
          const flushed = ftz && Math.abs(next) < FLT_MIN ? 0 : next;
          if (flushed === s && s !== 0 && stuckAt < 0) stuckAt = n;
          s = flushed;
        }
        pl.line(xs, ys, cssColor("signal"), 2);
        const normalEnd = Math.log(FLT_MIN) / Math.log(pole);
        denormInfo.set(
          `pólo ${pole}: chega à zona desnormalizada ao fim de <strong>${Math.round(normalEnd)}</strong> amostras · ` +
            (ftz
              ? "com FTZ passa a <strong>0</strong> e fica lá (e o exercício 38 pode saltar os blocos)"
              : stuckAt >= 0
                ? `sem FTZ fica <strong>preso</strong> num desnormal a partir da amostra ${stuckAt} (${pole}·s arredonda para s): todas as contas seguintes são lentas`
                : "sem FTZ atravessa a zona desnormalizada até zero"),
        );
      },
    });

    // ---- 4. SIMD: cabeça, corpo, cauda ----------------------------------------------------------
    let offset = 1;
    let count = 21;
    const simdInfo = readout();
    const simd = new Plot(el("div"), {
      height: 120,
      x: { min: 0, max: 32, ticks: [0, 4, 8, 12, 16, 20, 24, 28, 32], label: "índice do float na memória (grupos de 16 bytes)" },
      y: { min: 0, max: 1, ticks: [] },
      draw: (pl) => {
        for (let g = 0; g <= 32; g += 4) pl.vline(g, cssColor("text-muted"), [], 1);
        const head = Math.min(count, (4 - (offset % 4)) % 4);
        const body = Math.floor((count - head) / 4) * 4;
        for (let i = 0; i < 32; i++) {
          const k = i - offset;
          let color = "";
          if (k >= 0 && k < count) color = k < head ? "signal-2" : k < head + body ? "signal" : "signal-3";
          if (color) pl.rect(i + 0.08, 0.2, i + 0.92, 0.8, cssColor(color));
        }
        const tail = count - head - body;
        simdInfo.set(
          `${count} amostras a começar no índice ${offset}: cabeça <strong>${head}</strong> (1 a 1) · corpo <strong>${body / 4}</strong> instruções de 4 · cauda <strong>${tail}</strong> (1 a 1) → ` +
            `<strong>${head + body / 4 + tail}</strong> passos em vez de ${count}`,
        );
      },
    });

    root.append(
      el("h2", {}, "1. O orçamento de um bloco"),
      p("Um bloco de N amostras tem N/fs segundos para ser calculado — para todos os plugins de todas as pistas. Carrega em **Medir** para correr, aqui no browser, as duas versões do exercício 36 (em JavaScript: os números não são os do C++, e o compilador JIT do browser também otimiza por conta própria, mas a ordem de grandeza da diferença mantém-se)."),
      panel(
        bars.canvas,
        controls(
          select("Sample rate", ["44100", "48000", "96000"].map((v) => ({ value: v, label: `${+v / 1000} kHz` })), String(fs), (v) => { fs = Number(v); updateBudget(); }),
          select("Bloco", ["32", "64", "128", "256", "512", "1024"].map((v) => ({ value: v, label: `${v} amostras` })), String(block), (v) => { block = Number(v); updateBudget(); }),
          button("Medir", measure),
        ),
        budgetInfo,
      ),
      el("h2", {}, "2. Ouvir o que acontece acima de 100%"),
      p("Se um bloco demora mais do que o orçamento, a placa de som chega ao fim do que tem para tocar e toca silêncio até o bloco seguinte chegar. Simulação: com carga acima de 100%, falta o fim de cada bloco."),
      panel(
        controls(
          sourceSelect(source, (v) => { source = v; player.refresh(); }, ["loop", "sine", "noise"]),
          slider({ label: "Carga", min: 0.5, max: 2, step: 0.01, value: load, format: (v) => `${Math.round(v * 100)}%`, onInput: (v) => { load = v; player.refresh(); } }),
          select("Bloco", ["128", "512", "2048"].map((v) => ({ value: v, label: `${v} amostras` })), String(audioBlock), (v) => { audioBlock = Number(v); player.refresh(); }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Por isso a média não chega: basta **um** bloco lento (um `malloc`, um lock, um ficheiro) para um clique. O exercício 36 mede também o pico e conta os blocos acima de 100%."),
      el("h2", {}, "3. Denormais"),
      p("O estado de um filtro com feedback depois de a entrada parar: multiplica por um pólo < 1 a cada amostra, em `float` (escala logarítmica). Abaixo de ~1.2·10⁻³⁸ entra na zona desnormalizada. (O JavaScript calcula em `double`, por isso a lentidão não se nota aqui: o teste `[demo]` do exercício 38 mede-a em C++.)"),
      panel(
        denorm.canvas,
        controls(
          select("Pólo", ["0.99", "0.999", "0.9999"].map((v) => ({ value: v, label: v })), String(pole), (v) => { pole = Number(v); denorm.setAxes({ ...denorm.x, max: Math.ceil((1.3 * Math.log(DENORM_MIN)) / Math.log(pole)) }); }),
          toggle("FTZ/DAZ (juce::ScopedNoDenormals)", ftz, (v) => { ftz = v; denorm.redraw(); }),
        ),
        denormInfo,
      ),
      el("h2", {}, "4. SIMD: cabeça, corpo e cauda"),
      p("`SIMDRegister::fromRawArray` só lê de endereços alinhados a 16 bytes (4 floats). Um bloco pode começar em qualquer índice: processa-se a **cabeça** um a um até alinhar, o **corpo** 4 de cada vez, e a **cauda** um a um."),
      panel(
        simd.canvas,
        legend([{ label: "cabeça", color: "signal-2" }, { label: "corpo (SIMD)", color: "signal" }, { label: "cauda", color: "signal-3" }]),
        controls(
          slider({ label: "Início", min: 0, max: 7, step: 1, value: offset, format: (v) => String(v), onInput: (v) => { offset = v; simd.redraw(); } }),
          slider({ label: "Amostras", min: 1, max: 24, step: 1, value: count, format: (v) => String(v), onInput: (v) => { count = v; simd.redraw(); } }),
        ),
        simdInfo,
      ),
      sources([
        { title: "Ross Bencina — Real-time audio programming 101", url: "http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing", why: "o orçamento e o pior caso" },
        { title: "juce::AudioProcessLoadMeasurer — documentação", url: "https://docs.juce.com/master/classjuce_1_1AudioProcessLoadMeasurer.html", why: "o medidor de carga do JUCE" },
        { title: "juce::dsp::SIMDRegister — documentação", url: "https://docs.juce.com/master/structjuce_1_1dsp_1_1SIMDRegister.html", why: "SIMD no JUCE" },
        { title: "L. de Soras — Denormal numbers in floating point signal processing applications (PDF)", url: "http://ldesoras.free.fr/doc/articles/denormal-en.pdf", why: "denormais em áudio" },
        { title: "Wikipedia — Subnormal number", url: "https://en.wikipedia.org/wiki/Subnormal_number", why: "o formato" },
      ]),
    );

    updateBudget();
    simd.redraw();
  },
};
