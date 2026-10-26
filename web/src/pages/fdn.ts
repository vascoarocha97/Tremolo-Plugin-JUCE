import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { callout, controls, el, p, panel, playButton, readout, select, slider, sources } from "../lib/ui";
import { DiffuserFdn, Fdn, SchroederReverb, fdnSize, hadamard, householder, processLoopTwice } from "../lib/reverb";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;
type Structure = "schroeder" | "fdn" | "diffuser";
type Matrix = "householder" | "hadamard";

const structureLabels: Array<{ value: Structure; label: string }> = [
  { value: "schroeder", label: "Schroeder (exercício 28)" },
  { value: "fdn", label: "FDN (exercício 29)" },
  { value: "diffuser", label: "Difusor + FDN (exercício 30)" },
];

function makeReverb(structure: Structure, rate: number, rt60: number): (l: number, r: number) => [number, number] {
  if (structure === "schroeder") {
    const rev = new SchroederReverb(rate, rt60, 0.2);
    return (l, r) => rev.process(0.5 * (l + r));
  }
  if (structure === "fdn") {
    const rev = new Fdn(rate, rt60);
    return (l, r) => rev.process(0.5 * (l + r));
  }
  const rev = new DiffuserFdn(rate, rt60);
  return (l, r) => rev.process(l, r);
}

/** Matriz N×N aplicando a operação a cada vetor da base. */
function matrixOf(op: (v: Float64Array) => void): number[][] {
  const rows: number[][] = Array.from({ length: fdnSize }, () => new Array<number>(fdnSize).fill(0));
  for (let j = 0; j < fdnSize; j++) {
    const v = new Float64Array(fdnSize);
    v[j] = 1;
    op(v);
    for (let i = 0; i < fdnSize; i++) rows[i][j] = v[i];
  }
  return rows;
}

export const fdnPage: PageDef = {
  id: "fdn",
  title: "FDN e difusão",
  module: 7,
  summary: "Linhas de atraso cujas saídas são misturadas por uma matriz ortogonal antes de voltarem às entradas. Com um difusor à frente, a cauda já é densa desde a primeira volta.",
  render(root) {
    // ---- Matrizes
    let matrix: Matrix = "householder";
    const matrixInfo = readout();
    const matrixPlot = new Plot(el("div"), {
      height: 300,
      x: { min: 0, max: fdnSize, ticks: [], label: "entrada j" },
      y: { min: 0, max: fdnSize, ticks: [], label: "saída i" },
      draw: (pl) => {
        pl.grid();
        const m = matrixOf(matrix === "householder" ? householder : hadamard);
        const max = Math.max(...m.flat().map(Math.abs));
        for (let i = 0; i < fdnSize; i++) {
          for (let j = 0; j < fdnSize; j++) {
            const v = m[i][j];
            const alpha = Math.abs(v) / max;
            const color = v >= 0 ? cssColor("signal") : cssColor("signal-2");
            pl.ctx.save();
            pl.ctx.globalAlpha = 0.15 + 0.85 * alpha;
            pl.rect(j + 0.04, fdnSize - i - 1 + 0.04, j + 0.96, fdnSize - i - 0.04, color);
            pl.ctx.restore();
            pl.text(j + 0.5, fdnSize - i - 0.6, v.toFixed(2), cssColor("text"), "center");
          }
        }
      },
    });
    const updateMatrix = () => {
      matrixPlot.redraw();
      matrixInfo.set(
        matrix === "householder"
          ? "Householder: <code>A = I − (2/N)·11ᵀ</code>. Diagonal 1 − 2/8 = <strong>0.75</strong>, resto <strong>−0.25</strong>. Mistura pouco: cada saída é sobretudo a própria entrada. Custo O(N)."
          : "Hadamard normalizada: todos os valores <strong>±1/√8 ≈ ±0.354</strong>. Mistura tudo com tudo, por igual. Custo O(N·log N) com borboletas.",
      );
    };

    // ---- Densidade de ecos
    let structure: Structure = "fdn";
    let h = new Float32Array(0);
    const densityInfo = readout();
    const density = new Plot(el("div"), {
      height: 240,
      x: { min: 0, max: 500, label: "tempo (ms)" },
      y: { min: -0.15, max: 0.15, ticks: [-0.1, 0, 0.1] },
      draw: (pl) => {
        pl.grid();
        const xs = Array.from(h, (_, i) => (i / fs) * 1000);
        pl.line(xs, h, cssColor("signal"), 1);
      },
    });
    const updateDensity = () => {
      const n = Math.floor(0.5 * fs);
      const rev = makeReverb(structure, fs, 2);
      h = new Float32Array(n);
      for (let i = 0; i < n; i++) h[i] = rev(i === 0 ? 1 : 0, i === 0 ? 1 : 0)[0];
      const fraction = (from: number, to: number) => {
        let count = 0;
        for (let i = Math.floor(from * fs); i < Math.floor(to * fs); i++) if (Math.abs(h[i]) > 1e-7) count++;
        return count / ((to - from) * fs);
      };
      densityInfo.set(
        `amostras com som: 0–100 ms <strong>${(100 * fraction(0, 0.1)).toFixed(1)}%</strong> · ` +
          `100–300 ms <strong>${(100 * fraction(0.1, 0.3)).toFixed(1)}%</strong> · 300–500 ms <strong>${(100 * fraction(0.3, 0.5)).toFixed(1)}%</strong>`,
      );
      density.redraw();
    };

    // ---- Áudio
    let audioStructure: Structure = "diffuser";
    let source: SourceKind = "loop";
    let rt60 = 3;
    let mix = 0.35;
    const player = new LoopPlayer((rate) => {
      const rev = makeReverb(audioStructure, rate, rt60);
      return processLoopTwice(renderSource(source, rate, 0.45), mix, rev);
    });

    root.append(
      el("h2", {}, "1. A matriz de feedback"),
      p("Numa FDN, as N saídas são misturadas por uma matriz **ortogonal** antes de voltarem às entradas. Ortogonal = preserva a energia (‖A·v‖ = ‖v‖): sem os ganhos g_i a rede nem decai nem explode. As duas cores distinguem valores positivos e negativos; a intensidade é o módulo."),
      panel(
        matrixPlot.canvas,
        controls(select<Matrix>("Matriz", [{ value: "householder", label: "Householder (exercício 29)" }, { value: "hadamard", label: "Hadamard (exercício 30)" }], matrix, (v) => { matrix = v; updateMatrix(); })),
        matrixInfo,
      ),
      callout("Dentro do ciclo de feedback usa-se a Householder (mistura pouco); no difusor, fora do ciclo, a Hadamard (mistura o máximo). O artigo da Signalsmith explica porquê: demasiada mistura no ciclo \"prende\" as linhas umas às outras e colore caudas longas."),
      el("h2", {}, "2. Densidade de ecos: os primeiros 500 ms"),
      p("Resposta a um impulso (canal L, RT60 = 2 s). **Schroeder**: os allpasses enchem o início depressa, mas a cauda continua a ser 8 pentes periódicos. **FDN** sozinha (atrasos de 100–200 ms): nada nos primeiros 100 ms e depois ecos isolados que se multiplicam a cada volta. **Difusor + FDN** (4 passos de 20, 40, 80, 160 ms): cada impulso já entra na FDN como uma nuvem — a cauda é densa desde a primeira volta. Os primeiros ~100 ms continuam vazios (o atraso mais curto da FDN); o artigo da Signalsmith sugere preenchê-los com \"early reflections\" tiradas do difusor."),
      panel(
        density.canvas,
        controls(select<Structure>("Estrutura", structureLabels, structure, (v) => { structure = v; updateDensity(); })),
        densityInfo,
      ),
      el("h2", {}, "3. Ouvir e comparar"),
      p("Usa a tarola e o hi-hat do loop: com Schroeder ouve-se \"flutter\"; com a FDN a cauda é mais lisa mas começa granulada (ecos isolados); com o difusor, a cauda é lisa desde o início."),
      panel(
        controls(
          select<Structure>("Estrutura", structureLabels, audioStructure, (v) => { audioStructure = v; player.refresh(); }),
          sourceSelect(source, (v) => { source = v; player.refresh(); }),
          slider({ label: "RT60", min: 0.3, max: 10, step: 0.1, value: rt60, log: true, format: (v) => `${v.toFixed(1)} s`, onInput: (v) => { rt60 = v; player.refresh(); } }),
          slider({ label: "Mistura", min: 0, max: 1, step: 0.01, value: mix, format: (v) => v.toFixed(2), onInput: (v) => { mix = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      sources([
        { title: "Signalsmith Audio — Let's Write A Reverb", url: "https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/", why: "FDN multicanal com Householder + difusor com Hadamard (base desta página)" },
        { title: "J. O. Smith — PASP: Feedback Delay Networks", url: "https://ccrma.stanford.edu/~jos/pasp/Feedback_Delay_Networks_FDN.html", why: "a FDN em forma matricial" },
        { title: "J. O. Smith — PASP: Householder Feedback Matrix", url: "https://ccrma.stanford.edu/~jos/pasp/Householder_Feedback_Matrix.html", why: "a matriz usada no ciclo" },
        { title: "J. O. Smith — PASP: Hadamard Matrix", url: "https://ccrma.stanford.edu/~jos/pasp/Hadamard_Matrix.html", why: "a matriz usada no difusor" },
        { title: "J. O. Smith — PASP: History of FDNs", url: "https://ccrma.stanford.edu/~jos/pasp/History_FDNs_Artificial_Reverberation.html", why: "Gerzon, Stautner & Puckette, Jot" },
      ]),
    );

    updateMatrix();
    updateDensity();
  },
};
