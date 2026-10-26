// Pequena biblioteca de DSP em TypeScript, com os mesmos algoritmos dos exercícios C++.
// Serve as visualizações e o áudio das páginas (não é otimizada, é legível).

export const TWO_PI = 2 * Math.PI;

export const dbToGain = (db: number): number => (db <= -100 ? 0 : Math.pow(10, db / 20));
export const gainToDb = (gain: number): number => (gain <= 0 ? -100 : Math.max(-100, 20 * Math.log10(gain)));
export const clamp = (x: number, lo: number, hi: number): number => Math.min(hi, Math.max(lo, x));

/** Gerador pseudo-aleatório com seed (mulberry32): mesmo resultado em cada render. */
export function seededRandom(seed: number): () => number {
  let a = seed >>> 0;
  return () => {
    a = (a + 0x6d2b79f5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export function sine(frequency: number, sampleRate: number, length: number, amplitude = 1, phase = 0): Float32Array {
  const out = new Float32Array(length);
  const inc = (TWO_PI * frequency) / sampleRate;
  for (let n = 0; n < length; n++) out[n] = amplitude * Math.sin(phase + inc * n);
  return out;
}

/** Dente de serra ingénuo (exercício 03): rampa de -1 a 1 sem limitar a banda. */
export function naiveSaw(frequency: number, sampleRate: number, length: number, amplitude = 1): Float32Array {
  const out = new Float32Array(length);
  let phase = 0;
  const inc = frequency / sampleRate;
  for (let n = 0; n < length; n++) {
    out[n] = amplitude * (2 * phase - 1);
    phase += inc;
    if (phase >= 1) phase -= 1;
  }
  return out;
}

/** Dente de serra com banda limitada (soma de harmónicos abaixo de Nyquist): a referência sem aliasing. */
export function bandlimitedSaw(frequency: number, sampleRate: number, length: number, amplitude = 1): Float32Array {
  const out = new Float32Array(length);
  const maxHarmonic = Math.floor(sampleRate / 2 / frequency);
  for (let k = 1; k <= maxHarmonic; k++) {
    // Série de Fourier da rampa de -1 a 1: -(2/π)·Σ sin(kωt)/k
    const a = (-2 / (Math.PI * k)) * amplitude;
    const inc = (TWO_PI * frequency * k) / sampleRate;
    for (let n = 0; n < length; n++) out[n] += a * Math.sin(inc * n);
  }
  return out;
}

export function noise(length: number, amplitude = 1, seed = 1): Float32Array {
  const rnd = seededRandom(seed);
  const out = new Float32Array(length);
  for (let n = 0; n < length; n++) out[n] = amplitude * (2 * rnd() - 1);
  return out;
}

/** Frequência ouvida depois de amostrar (exercício 03). */
export function aliasedFrequency(frequency: number, sampleRate: number): number {
  const folded = Math.abs(frequency) % sampleRate;
  return folded > sampleRate / 2 ? sampleRate - folded : folded;
}

/** Quantização mid-tread para N bits (exercício 04). */
export function quantise(x: number, bits: number): number {
  const step = 1 / Math.pow(2, bits - 1);
  return clamp(Math.round(x / step) * step, -1, 1 - step);
}

/** Aplica um ganho com rampa linear de rampSamples amostras a partir de cada mudança (exercício 08). */
export function rampedGain(targets: Float32Array, rampSamples: number): Float32Array {
  const out = new Float32Array(targets.length);
  let current = targets[0] ?? 0;
  let target = current;
  let step = 0;
  let countdown = 0;
  for (let n = 0; n < targets.length; n++) {
    if (targets[n] !== target) {
      target = targets[n];
      if (rampSamples <= 0) {
        current = target;
        countdown = 0;
      } else {
        countdown = rampSamples;
        step = (target - current) / countdown;
      }
    }
    if (countdown > 0) {
      countdown--;
      current = countdown > 0 ? current + step : target;
    }
    out[n] = current;
  }
  return out;
}

/** FFT complexa radix-2, in-place. re/im com comprimento potência de 2. */
export function fft(re: Float64Array, im: Float64Array): void {
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      [re[i], re[j]] = [re[j], re[i]];
      [im[i], im[j]] = [im[j], im[i]];
    }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = -TWO_PI / len;
    const wr = Math.cos(ang);
    const wi = Math.sin(ang);
    for (let i = 0; i < n; i += len) {
      let cr = 1;
      let ci = 0;
      for (let k = 0; k < len / 2; k++) {
        const ar = re[i + k + len / 2] * cr - im[i + k + len / 2] * ci;
        const ai = re[i + k + len / 2] * ci + im[i + k + len / 2] * cr;
        re[i + k + len / 2] = re[i + k] - ar;
        im[i + k + len / 2] = im[i + k] - ai;
        re[i + k] += ar;
        im[i + k] += ai;
        const ncr = cr * wr - ci * wi;
        ci = cr * wi + ci * wr;
        cr = ncr;
      }
    }
  }
}

/** Espectro de magnitude (dB, relativo a um seno de amplitude 1) com janela de Hann. */
export function magnitudeSpectrumDb(signal: Float32Array, fftSize = 8192, offset = 0): Float32Array {
  const re = new Float64Array(fftSize);
  const im = new Float64Array(fftSize);
  for (let i = 0; i < fftSize; i++) {
    const w = 0.5 - 0.5 * Math.cos((TWO_PI * i) / fftSize);
    re[i] = (signal[offset + i] ?? 0) * w;
  }
  fft(re, im);
  const out = new Float32Array(fftSize / 2);
  const norm = 4 / fftSize; // ganho coerente da janela de Hann = 0.5
  for (let k = 0; k < fftSize / 2; k++) out[k] = gainToDb(Math.hypot(re[k], im[k]) * norm);
  return out;
}

export function rms(x: Float32Array, start = 0, end = x.length): number {
  let s = 0;
  for (let i = start; i < end; i++) s += x[i] * x[i];
  return Math.sqrt(s / Math.max(1, end - start));
}

/** Loop de demonstração (igual ao do standalone C++): bombo, tarola, hi-hat e "guitarra" Karplus-Strong. */
export function demoLoop(sampleRate: number): Float32Array[] {
  const bpm = 100;
  const beat = (60 / bpm) * sampleRate;
  const total = Math.floor(8 * beat);
  const left = new Float32Array(total);
  const right = new Float32Array(total);
  const rnd = seededRandom(42);
  const at = (b: number) => Math.floor(b * beat);

  const add = (start: number, ch: Float32Array, value: number) => {
    if (start >= 0 && start < total) ch[start] += value;
  };

  const kick = (start: number) => {
    let phase = 0;
    for (let n = 0; n < 0.45 * sampleRate; n++) {
      const t = n / sampleRate;
      phase += (TWO_PI * (45 + 75 * Math.exp(-t * 30))) / sampleRate;
      const v = 0.9 * Math.sin(phase) * Math.exp(-t * 7);
      add(start + n, left, v);
      add(start + n, right, v);
    }
  };

  const snare = (start: number) => {
    let phase = 0;
    for (let n = 0; n < 0.25 * sampleRate; n++) {
      const t = n / sampleRate;
      phase += (TWO_PI * 185) / sampleRate;
      const v = 0.35 * (2 * rnd() - 1) * Math.exp(-t * 18) + 0.3 * Math.sin(phase) * Math.exp(-t * 30);
      add(start + n, left, v);
      add(start + n, right, v);
    }
  };

  const hat = (start: number, level: number) => {
    let prev = 0;
    for (let n = 0; n < 0.05 * sampleRate; n++) {
      const t = n / sampleRate;
      const x = 2 * rnd() - 1;
      const v = level * (x - prev) * Math.exp(-t * 90);
      prev = x;
      add(start + n, left, 0.7 * v);
      add(start + n, right, v);
    }
  };

  const pluck = (start: number, freq: number) => {
    const size = Math.max(2, Math.round(sampleRate / freq));
    const line = new Float32Array(size).map(() => 2 * rnd() - 1);
    let idx = 0;
    const len = Math.floor(0.9 * sampleRate);
    for (let n = 0; n < len; n++) {
      const next = (idx + 1) % size;
      const current = line[idx];
      line[idx] = 0.996 * 0.5 * (current + line[next]);
      idx = next;
      const fade = n > len - 256 ? (len - n) / 256 : 1;
      add(start + n, left, 0.3 * current * fade);
      add(start + n, right, 0.225 * current * fade);
    }
  };

  for (let b = 0; b < 8; b++) {
    if (b % 2 === 0) kick(at(b));
    else snare(at(b));
    hat(at(b), 0.25);
    hat(at(b + 0.5), 0.15);
  }
  kick(at(6.5));

  const riff = [110, 0, 130.81, 164.81, 0, 196, 164.81, 0, 110, 0, 130.81, 146.83, 0, 130.81, 98, 0];
  riff.forEach((f, step) => {
    if (f > 0) pluck(at(step * 0.5), f);
  });

  let peak = 0;
  for (let i = 0; i < total; i++) peak = Math.max(peak, Math.abs(left[i]), Math.abs(right[i]));
  const g = peak > 0 ? 0.9 / peak : 1;
  for (let i = 0; i < total; i++) {
    left[i] *= g;
    right[i] *= g;
  }
  return [left, right];
}

// ---- Filtros (módulo 4) -----------------------------------------------------------------------

export interface Biquad {
  b0: number;
  b1: number;
  b2: number;
  a1: number;
  a2: number;
}

export type BiquadType = "lowPass" | "highPass" | "bandPass" | "notch" | "peaking" | "lowShelf" | "highShelf";

/** Coeficientes do Audio EQ Cookbook (RBJ), normalizados por a0 — como no exercício 15. */
export function rbj(type: BiquadType, sampleRate: number, f0: number, q: number, gainDb = 0): Biquad {
  const w0 = (TWO_PI * f0) / sampleRate;
  const c = Math.cos(w0);
  const alpha = Math.sin(w0) / (2 * q);
  const A = Math.pow(10, gainDb / 40);
  const k = 2 * Math.sqrt(A) * alpha;
  let r: [number, number, number, number, number, number];
  switch (type) {
    case "lowPass": r = [(1 - c) / 2, 1 - c, (1 - c) / 2, 1 + alpha, -2 * c, 1 - alpha]; break;
    case "highPass": r = [(1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha]; break;
    case "bandPass": r = [alpha, 0, -alpha, 1 + alpha, -2 * c, 1 - alpha]; break;
    case "notch": r = [1, -2 * c, 1, 1 + alpha, -2 * c, 1 - alpha]; break;
    case "peaking": r = [1 + alpha * A, -2 * c, 1 - alpha * A, 1 + alpha / A, -2 * c, 1 - alpha / A]; break;
    case "lowShelf":
      r = [A * (A + 1 - (A - 1) * c + k), 2 * A * (A - 1 - (A + 1) * c), A * (A + 1 - (A - 1) * c - k),
           A + 1 + (A - 1) * c + k, -2 * (A - 1 + (A + 1) * c), A + 1 + (A - 1) * c - k];
      break;
    case "highShelf":
      r = [A * (A + 1 + (A - 1) * c + k), -2 * A * (A - 1 + (A + 1) * c), A * (A + 1 + (A - 1) * c - k),
           A + 1 - (A - 1) * c + k, 2 * (A - 1 - (A + 1) * c), A + 1 - (A - 1) * c - k];
      break;
  }
  const [b0, b1, b2, a0, a1, a2] = r;
  return { b0: b0 / a0, b1: b1 / a0, b2: b2 / a0, a1: a1 / a0, a2: a2 / a0 };
}

/** |H(e^jω)| de um biquad. */
export function biquadMagnitude(c: Biquad, frequency: number, sampleRate: number): number {
  const w = (TWO_PI * frequency) / sampleRate;
  const cos1 = Math.cos(w), sin1 = Math.sin(w), cos2 = Math.cos(2 * w), sin2 = Math.sin(2 * w);
  const nr = c.b0 + c.b1 * cos1 + c.b2 * cos2;
  const ni = -(c.b1 * sin1 + c.b2 * sin2);
  const dr = 1 + c.a1 * cos1 + c.a2 * cos2;
  const di = -(c.a1 * sin1 + c.a2 * sin2);
  return Math.hypot(nr, ni) / Math.hypot(dr, di);
}

/** Processa um sinal com um biquad (TDF-II). */
export function processBiquad(input: Float32Array, c: Biquad): Float32Array {
  const out = new Float32Array(input.length);
  let s1 = 0, s2 = 0;
  for (let i = 0; i < input.length; i++) {
    const x = input[i];
    const y = c.b0 * x + s1;
    s1 = c.b1 * x - c.a1 * y + s2;
    s2 = c.b2 * x - c.a2 * y;
    out[i] = y;
  }
  return out;
}

/** Filtro de um polo (exercício 14). */
export function onePole(input: Float32Array, cutoff: number, sampleRate: number, highPass = false): Float32Array {
  const a = 1 - Math.exp((-TWO_PI * cutoff) / sampleRate);
  const out = new Float32Array(input.length);
  let state = 0;
  for (let i = 0; i < input.length; i++) {
    state += a * (input[i] - state);
    out[i] = highPass ? input[i] - state : state;
  }
  return out;
}

export type SvfMode = "lowPass" | "bandPass" | "highPass";

/** SVF TPT (exercício 16) com o corte modulado por amostra: cutoffAt(n) devolve o corte da amostra n. */
export function processSvf(input: Float32Array, sampleRate: number, q: number, mode: SvfMode, cutoffAt: (n: number) => number): Float32Array {
  const out = new Float32Array(input.length);
  const R2 = 1 / Math.max(0.1, q);
  let s1 = 0, s2 = 0;
  for (let n = 0; n < input.length; n++) {
    const fc = clamp(cutoffAt(n), 10, 0.49 * sampleRate);
    const g = Math.tan((Math.PI * fc) / sampleRate);
    const h = 1 / (1 + R2 * g + g * g);
    const hp = h * (input[n] - s1 * (g + R2) - s2);
    const bp = g * hp + s1;
    s1 = g * hp + bp;
    const lp = g * bp + s2;
    s2 = g * bp + lp;
    out[n] = mode === "lowPass" ? lp : mode === "bandPass" ? bp : hp;
  }
  return out;
}

// ---- Modulação (módulo 5) ---------------------------------------------------------------------

export type LfoShape = "sine" | "triangle" | "saw" | "square";

/** As formas do LFO do exercício 18 (fase em ciclos). */
export function lfoValue(shape: LfoShape, phase: number): number {
  const p = phase - Math.floor(phase);
  switch (shape) {
    case "sine": return Math.sin(TWO_PI * p);
    case "triangle": return p < 0.25 ? 4 * p : p < 0.75 ? 2 - 4 * p : 4 * p - 4;
    case "saw": return p < 0.5 ? 2 * p : 2 * p - 2;
    case "square": return p < 0.5 ? 1 : -1;
  }
}

/** Delay modulado com feedback e interpolação linear, um canal (chorus/flanger, exercícios 20–21).
    delayAt(n) devolve o atraso em amostras (>= 1). Processa o loop duas vezes para não haver corte
    na repetição. */
export function modulatedDelay(input: Float32Array, delayAt: (n: number) => number, feedback: number, mix: number, maxDelay: number): Float32Array {
  const size = Math.ceil(maxDelay) + 4;
  const line = new Float32Array(size);
  let w = 0;
  const out = new Float32Array(input.length);
  for (let pass = 0; pass < 2; pass++) {
    for (let n = 0; n < input.length; n++) {
      const d = Math.max(1, delayAt(n));
      const whole = Math.floor(d);
      const frac = d - whole;
      // posição de escrita da próxima amostra é (w + 1); a amostra de há k passos está em w + 1 - k
      const a = line[(w + 1 - whole + size * 4) % size];
      const b = line[(w - whole + size * 4) % size];
      const wet = a + frac * (b - a);
      w = (w + 1) % size;
      line[w] = input[n] + feedback * wet;
      if (pass === 1) out[n] = (1 - mix) * input[n] + mix * wet;
    }
  }
  return out;
}

// ---- Distorção (módulo 6) ---------------------------------------------------------------------

export type ShaperKind = "hardClip" | "softClip" | "tanh" | "asymmetric";

/** As curvas do exercício 22. */
export function shape(kind: ShaperKind, x: number): number {
  switch (kind) {
    case "hardClip": return clamp(x, -1, 1);
    case "softClip": return x >= 1 ? 1 : x <= -1 ? -1 : 1.5 * (x - (x * x * x) / 3);
    case "tanh": return Math.tanh(x);
    case "asymmetric": return x >= 0 ? Math.tanh(x) : 0.5 * Math.tanh(2 * x);
  }
}

/** Passa-baixo FIR por janela (sinc com janela de Blackman). cutoff em fração do sample rate (0..0.5). */
export function designLowpassFir(numTaps: number, cutoff: number): Float64Array {
  const h = new Float64Array(numTaps);
  const m = (numTaps - 1) / 2;
  let sum = 0;
  for (let i = 0; i < numTaps; i++) {
    const t = i - m;
    const sinc = t === 0 ? 2 * cutoff : Math.sin(TWO_PI * cutoff * t) / (Math.PI * t);
    const w = 0.42 - 0.5 * Math.cos((TWO_PI * i) / (numTaps - 1)) + 0.08 * Math.cos((2 * TWO_PI * i) / (numTaps - 1));
    h[i] = sinc * w;
    sum += h[i];
  }
  for (let i = 0; i < numTaps; i++) h[i] /= sum;
  return h;
}

/** Aplica uma função não-linear com oversampling de fator L (inteiro): interpolação FIR polifásica,
    a função a L·fs, filtro FIR e decimação. Com L = 1 aplica a função diretamente. */
export function oversampled(input: Float32Array, factor: number, fn: (x: number) => number): Float32Array {
  if (factor <= 1) return input.map(fn);

  const taps = 32 * factor + 1;
  const h = designLowpassFir(taps, 0.45 / factor);
  const n = input.length;
  const up = new Float64Array(n * factor);

  // Interpolação: só os coeficientes que caem em amostras não-nulas (as originais) contam
  for (let j = 0; j < up.length; j++) {
    let acc = 0;
    for (let k = j % factor; k < taps; k += factor) {
      const src = (j - k) / factor;
      if (src >= 0 && src < n) acc += h[k] * input[src];
    }
    up[j] = fn(acc * factor);
  }

  // Filtro de decimação: só se calculam as amostras que ficam
  const out = new Float32Array(n);
  for (let i = 0; i < n; i++) {
    const j = i * factor + (taps - 1); // compensa o atraso dos dois filtros (≈ (taps−1)/factor amostras)
    let acc = 0;
    for (let k = 0; k < taps; k++) {
      const s = j - k;
      if (s >= 0 && s < up.length) acc += h[k] * up[s];
    }
    out[i] = acc;
  }
  return out;
}
