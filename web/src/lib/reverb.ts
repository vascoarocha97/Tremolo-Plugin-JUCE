// Blocos de reverb (módulo 7) em TypeScript, com o mesmo comportamento dos exercícios 27–31.
// Servem para as visualizações e para ouvir no browser; não são as soluções em C++.

import { seededRandom } from "./dsp";

/** g = 0.001^(D / (T60·fs)): ganho por volta para cair 60 dB em T60 segundos. */
export function feedbackForRt60(delaySamples: number, sampleRate: number, rt60: number): number {
  return Math.pow(0.001, delaySamples / (Math.max(0.01, rt60) * sampleRate));
}

/** Pente com feedback e passa-baixo de um polo no ciclo (exercício 27). */
export class Comb {
  private readonly buffer: Float32Array;
  private index = 0;
  private filterState = 0;

  constructor(delaySamples: number, public feedback = 0.5, public damping = 0) {
    this.buffer = new Float32Array(Math.max(1, Math.round(delaySamples)));
  }

  get delay(): number {
    return this.buffer.length;
  }

  process(x: number): number {
    const out = this.buffer[this.index];
    this.filterState = (1 - this.damping) * out + this.damping * this.filterState;
    this.buffer[this.index] = x + this.feedback * this.filterState;
    if (++this.index >= this.buffer.length) this.index = 0;
    return out;
  }
}

/** Allpass de Schroeder verdadeiro (exercício 27). */
export class Allpass {
  private readonly buffer: Float32Array;
  private index = 0;

  constructor(delaySamples: number, public gain = 0.5) {
    this.buffer = new Float32Array(Math.max(1, Math.round(delaySamples)));
  }

  process(x: number): number {
    const delayed = this.buffer[this.index];
    const w = x + this.gain * delayed;
    this.buffer[this.index] = w;
    if (++this.index >= this.buffer.length) this.index = 0;
    return delayed - this.gain * w;
  }
}

// ---- Schroeder / Freeverb (exercício 28) ------------------------------------------------------

export const combTunings = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617];
export const allpassTunings = [556, 441, 341, 225];
export const stereoSpread = 23;

export class SchroederReverb {
  readonly combs: Comb[][];
  readonly allpasses: Allpass[][];

  constructor(sampleRate: number, rt60: number, damping: number) {
    const scale = sampleRate / 44100;
    this.combs = [0, stereoSpread].map((spread) =>
      combTunings.map((t) => {
        const d = Math.floor((t + spread) * scale);
        return new Comb(d, feedbackForRt60(d, sampleRate, rt60), damping);
      }),
    );
    this.allpasses = [0, stereoSpread].map((spread) => allpassTunings.map((t) => new Allpass(Math.floor((t + spread) * scale), 0.5)));
  }

  /** Entrada mono → [wetL, wetR]. */
  process(input: number): [number, number] {
    const x = input / combTunings.length;
    const wet: [number, number] = [0, 0];
    for (let ch = 0; ch < 2; ch++) {
      let sum = 0;
      for (const c of this.combs[ch]) sum += c.process(x);
      for (const a of this.allpasses[ch]) sum = a.process(sum);
      wet[ch] = sum;
    }
    return wet;
  }
}

// ---- FDN (exercício 29) -------------------------------------------------------------------------

export const fdnSize = 8;
export const fdnDelaysMs = [101.3, 113.9, 127.1, 139.7, 151.1, 163.3, 179.9, 197.3];

export function householder(v: Float64Array | number[]): void {
  let sum = 0;
  for (let i = 0; i < v.length; i++) sum += v[i];
  const c = (sum * 2) / v.length;
  for (let i = 0; i < v.length; i++) v[i] -= c;
}

export function hadamard(v: Float64Array | number[]): void {
  for (let half = 1; half < v.length; half *= 2) {
    for (let start = 0; start < v.length; start += 2 * half) {
      for (let i = start; i < start + half; i++) {
        const a = v[i];
        const b = v[i + half];
        v[i] = a + b;
        v[i + half] = a - b;
      }
    }
  }
  const s = 1 / Math.sqrt(v.length);
  for (let i = 0; i < v.length; i++) v[i] *= s;
}

export class Fdn {
  private readonly lines: Float32Array[];
  private readonly indices = new Int32Array(fdnSize);
  private readonly gains = new Float64Array(fdnSize);
  private readonly delayed = new Float64Array(fdnSize);
  private readonly mixed = new Float64Array(fdnSize);

  constructor(sampleRate: number, rt60: number, delaysMs = fdnDelaysMs) {
    this.lines = delaysMs.map((ms) => new Float32Array(Math.max(1, Math.floor((ms / 1000) * sampleRate))));
    this.lines.forEach((line, i) => (this.gains[i] = feedbackForRt60(line.length, sampleRate, rt60)));
  }

  /** Um passo: devolve as N saídas (vista sobre um array interno — copia se precisares de as guardar). */
  processFrame(input: ArrayLike<number>): Float64Array {
    for (let i = 0; i < fdnSize; i++) this.delayed[i] = this.lines[i][this.indices[i]];
    this.mixed.set(this.delayed);
    householder(this.mixed);
    for (let i = 0; i < fdnSize; i++) {
      this.lines[i][this.indices[i]] = input[i] + this.gains[i] * this.mixed[i];
      if (++this.indices[i] >= this.lines[i].length) this.indices[i] = 0;
    }
    return this.delayed;
  }

  private readonly monoFrame = new Float64Array(fdnSize);

  process(input: number): [number, number] {
    this.monoFrame.fill(input / Math.sqrt(fdnSize));
    return downmix(this.processFrame(this.monoFrame));
  }
}

export function downmix(out: ArrayLike<number>): [number, number] {
  let l = 0;
  let r = 0;
  for (let i = 0; i < fdnSize; i++) {
    l += (i % 2 === 0 ? 1 : -1) * out[i];
    r += (Math.floor(i / 2) % 2 === 0 ? 1 : -1) * out[i];
  }
  const s = 1 / Math.sqrt(fdnSize);
  return [l * s, r * s];
}

// ---- Difusor (exercício 30) ---------------------------------------------------------------------

export const diffusionStepsMs = [20, 40, 80, 160];

export class DiffusionStep {
  readonly lines: Float32Array[] = [];
  readonly flips: boolean[] = [];
  private readonly indices = new Int32Array(fdnSize);
  private readonly out = new Float64Array(fdnSize);

  constructor(sampleRate: number, maxDelayMs: number, seed: number) {
    const rnd = seededRandom(seed);
    const segment = ((maxDelayMs / 1000) * sampleRate) / fdnSize;
    for (let i = 0; i < fdnSize; i++) {
      this.lines.push(new Float32Array(Math.max(1, Math.floor(segment * (i + rnd())))));
      this.flips.push(rnd() < 0.5);
    }
  }

  process(input: ArrayLike<number>): Float64Array {
    for (let i = 0; i < fdnSize; i++) {
      const line = this.lines[i];
      const delayed = line[this.indices[i]];
      line[this.indices[i]] = input[i];
      if (++this.indices[i] >= line.length) this.indices[i] = 0;
      this.out[i] = this.flips[i] ? -delayed : delayed;
    }
    hadamard(this.out);
    return this.out;
  }
}

export class DiffuserFdn {
  readonly steps: DiffusionStep[];
  readonly fdn: Fdn;
  private readonly frame = new Float64Array(fdnSize);

  constructor(sampleRate: number, rt60: number, numSteps = 4) {
    this.steps = diffusionStepsMs.slice(0, numSteps).map((ms, s) => new DiffusionStep(sampleRate, ms, 1000 + 17 * s));
    this.fdn = new Fdn(sampleRate, rt60);
  }

  process(inL: number, inR: number): [number, number] {
    const s = 1 / Math.sqrt(fdnSize);
    for (let i = 0; i < fdnSize; i++) this.frame[i] = (i % 2 === 0 ? inL : inR) * s;
    let f: ArrayLike<number> = this.frame;
    for (const step of this.steps) f = step.process(f);
    return downmix(this.fdn.processFrame(f));
  }
}

// ---- Pitch shifter e shimmer (exercício 31) -----------------------------------------------------

export const crossfadeGain = (p: number): number => Math.sin(Math.PI * p) ** 2;

export class PitchShifter {
  private readonly buffer: Float32Array;
  private readonly window: number;
  private write = 0;
  phase = 0;
  ratio = 2;

  constructor(sampleRate: number, windowMs = 50) {
    this.window = (windowMs / 1000) * sampleRate;
    this.buffer = new Float32Array(Math.floor(this.window) + 4);
  }

  setSemitones(st: number): void {
    this.ratio = Math.pow(2, st / 12);
  }

  private read(delay: number): number {
    const size = this.buffer.length;
    let pos = this.write - delay;
    if (pos < 0) pos += size;
    const i0 = Math.floor(pos);
    const frac = pos - i0;
    const a = this.buffer[i0];
    const b = this.buffer[(i0 + 1) % size];
    return a + frac * (b - a);
  }

  process(x: number): number {
    this.buffer[this.write] = x;
    const p2 = this.phase + 0.5 >= 1 ? this.phase - 0.5 : this.phase + 0.5;
    const out = crossfadeGain(this.phase) * this.read(1 + this.phase * this.window) + crossfadeGain(p2) * this.read(1 + p2 * this.window);
    this.phase += (1 - this.ratio) / this.window;
    if (this.phase < 0) this.phase += 1;
    else if (this.phase >= 1) this.phase -= 1;
    if (++this.write >= this.buffer.length) this.write = 0;
    return out;
  }
}

export class Shimmer {
  readonly fdn: Fdn;
  readonly shifter: PitchShifter;
  private lastWet = 0;
  private lp = 0;
  private readonly lpCoeff: number;

  constructor(sampleRate: number, rt60: number, public amount: number, semitones: number) {
    this.fdn = new Fdn(sampleRate, rt60);
    this.shifter = new PitchShifter(sampleRate);
    this.shifter.setSemitones(semitones);
    this.lpCoeff = Math.exp((-2 * Math.PI * 6000) / sampleRate);
  }

  process(input: number): [number, number] {
    const shifted = this.shifter.process(this.lastWet);
    this.lp = shifted + this.lpCoeff * (this.lp - shifted);
    const feedback = this.amount * Math.tanh(this.lp);
    const wet = this.fdn.process(input + feedback);
    this.lastWet = 0.5 * (wet[0] + wet[1]);
    return wet;
  }
}

// ---- Utilitários ----------------------------------------------------------------------------------

/** Aplica uma reverb a um loop estéreo, duas passagens (a cauda da primeira continua no início da
    segunda, para o loop não ter cortes). */
export function processLoopTwice(
  input: Float32Array[],
  mix: number,
  process: (l: number, r: number) => [number, number],
): Float32Array[] {
  const length = input[0].length;
  const out = [new Float32Array(length), new Float32Array(length)];
  for (let pass = 0; pass < 2; pass++) {
    for (let i = 0; i < length; i++) {
      const l = input[0][i];
      const r = input[1 % input.length][i];
      const [wl, wr] = process(l, r);
      if (pass === 1) {
        out[0][i] = (1 - mix) * l + mix * wl;
        out[1][i] = (1 - mix) * r + mix * wr;
      }
    }
  }
  return out;
}

/** Curva de decaimento de Schroeder (EDC) em dB, normalizada a 0 dB no início. */
export function energyDecayDb(h: Float32Array): Float32Array {
  const edc = new Float64Array(h.length);
  let total = 0;
  for (let i = h.length - 1; i >= 0; i--) {
    total += h[i] * h[i];
    edc[i] = total;
  }
  const out = new Float32Array(h.length);
  for (let i = 0; i < h.length; i++) out[i] = edc[0] > 0 ? Math.max(-120, 10 * Math.log10(edc[i] / edc[0] + 1e-30)) : -120;
  return out;
}

/** RT60 a partir da EDC: declive entre −5 e −25 dB (T20), extrapolado para 60 dB. */
export function estimateRt60(edcDb: Float32Array, sampleRate: number): number {
  const at = (db: number) => {
    for (let i = 0; i < edcDb.length; i++) if (edcDb[i] <= db) return i / sampleRate;
    return edcDb.length / sampleRate;
  };
  return 3 * (at(-25) - at(-5));
}

/** Reduz um sinal longo a um envelope (máximo de |x| por bloco) para desenhar depressa. */
export function envelope(x: Float32Array, blocks: number): { t: Float32Array; v: Float32Array } {
  const size = Math.max(1, Math.floor(x.length / blocks));
  const n = Math.floor(x.length / size);
  const t = new Float32Array(n);
  const v = new Float32Array(n);
  for (let b = 0; b < n; b++) {
    let m = 0;
    for (let i = b * size; i < (b + 1) * size; i++) m = Math.max(m, Math.abs(x[i]));
    t[b] = b * size;
    v[b] = m;
  }
  return { t, v };
}
