// Fontes de áudio comuns às páginas (loop, ruído, seno, dente de serra), com cache.

import { bandlimitedSaw, demoLoop, noise, sine } from "./dsp";
import { select } from "./ui";

export type SourceKind = "loop" | "noise" | "sine" | "saw";

const labels: Record<SourceKind, string> = {
  loop: "loop de demonstração",
  noise: "ruído branco",
  sine: "seno 440 Hz",
  saw: "dente de serra 110 Hz",
};

const cache = new Map<string, Float32Array[]>();

/** Fonte estéreo (2 canais) ao sample rate dado. O loop tem 4.8 s; as outras 2 s. */
export function renderSource(kind: SourceKind, sampleRate: number, level = 0.5): Float32Array[] {
  const key = `${kind}@${sampleRate}`;
  let channels = cache.get(key);

  if (!channels) {
    switch (kind) {
      case "loop": channels = demoLoop(sampleRate); break;
      case "noise": channels = [noise(sampleRate * 2, 0.5, 3), noise(sampleRate * 2, 0.5, 4)]; break;
      case "sine": { const s = sine(440, sampleRate, sampleRate * 2, 0.7); channels = [s, s]; break; }
      case "saw": { const s = bandlimitedSaw(110, sampleRate, sampleRate * 2, 0.6); channels = [s, s]; break; }
    }
    cache.set(key, channels);
  }

  return channels.map((ch) => ch.map((v) => v * level));
}

export function sourceSelect(value: SourceKind, onChange: (kind: SourceKind) => void, kinds: SourceKind[] = ["loop", "noise", "sine", "saw"]): HTMLElement {
  return select("Fonte", kinds.map((k) => ({ value: k, label: labels[k] })), value, onChange);
}
