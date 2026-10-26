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
