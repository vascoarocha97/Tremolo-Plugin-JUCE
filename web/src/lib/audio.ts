// Áudio da web app: um AudioContext partilhado, com volume geral baixo e um limitador
// (DynamicsCompressorNode) antes da saída, para nenhuma demonstração rebentar os ouvidos.

let context: AudioContext | null = null;
let masterGain: GainNode | null = null;

export function audioContext(): AudioContext {
  if (!context) {
    context = new AudioContext({ latencyHint: "interactive" });
    masterGain = context.createGain();
    masterGain.gain.value = 0.5;

    const limiter = context.createDynamicsCompressor();
    limiter.threshold.value = -3;
    limiter.knee.value = 0;
    limiter.ratio.value = 20;
    limiter.attack.value = 0.002;
    limiter.release.value = 0.1;

    masterGain.connect(limiter);
    limiter.connect(context.destination);
  }

  return context;
}

function output(): AudioNode {
  audioContext();
  return masterGain!;
}

/** Sample rate real da placa de som (as demonstrações geram áudio a esta taxa). */
export function deviceSampleRate(): number {
  return audioContext().sampleRate;
}

/** Função que gera os canais de áudio (Float32Array por canal) para um dado sample rate. */
export type Renderer = (sampleRate: number) => Float32Array[];

function toAudioBuffer(ctx: AudioContext, channels: Float32Array[]): AudioBuffer {
  const length = Math.max(1, channels[0]?.length ?? 1);
  const buffer = ctx.createBuffer(Math.max(1, channels.length), length, ctx.sampleRate);

  channels.forEach((data, ch) => {
    // Proteção: nada de NaN/inf e nada acima de 0 dBFS para a placa de som (o compressor da saída
    // não é um limitador instantâneo; este corte é).
    const safe = new Float32Array(data.length);
    for (let i = 0; i < data.length; i++) safe[i] = Number.isFinite(data[i]) ? Math.max(-1, Math.min(1, data[i])) : 0;
    buffer.copyToChannel(safe, ch);
  });

  return buffer;
}

/** Toca um buffer em loop. refresh() volta a gerar o áudio (ex.: quando um slider muda) e faz um
    crossfade curto entre o antigo e o novo, para não haver clicks. */
export class LoopPlayer {
  private source: AudioBufferSourceNode | null = null;
  private gain: GainNode | null = null;
  private listeners: Array<(playing: boolean) => void> = [];

  constructor(private readonly render: Renderer, private readonly level = 0.5) {}

  get playing(): boolean {
    return this.source !== null;
  }

  onChange(listener: (playing: boolean) => void): void {
    this.listeners.push(listener);
  }

  async start(): Promise<void> {
    const ctx = audioContext();
    await ctx.resume();

    // Se mudaste de página enquanto o contexto arrancava, o stopAll() já correu: não arranques.
    if (this.source || !activePlayers.has(this)) return;

    this.startNew(ctx, 0.02);
    this.notify();
  }

  stop(): void {
    if (!this.source || !this.gain) return;

    const ctx = audioContext();
    const now = ctx.currentTime;
    this.gain.gain.cancelScheduledValues(now);
    this.gain.gain.setValueAtTime(this.gain.gain.value, now);
    this.gain.gain.linearRampToValueAtTime(0, now + 0.03);
    this.source.stop(now + 0.05);
    this.source = null;
    this.gain = null;
    this.notify();
  }

  toggle(): void {
    if (this.playing) this.stop();
    else void this.start();
  }

  /** Gera de novo e faz crossfade (só se estiver a tocar). */
  refresh(): void {
    if (!this.source || !this.gain) return;

    const ctx = audioContext();
    const now = ctx.currentTime;
    const oldSource = this.source;
    const oldGain = this.gain;

    oldGain.gain.cancelScheduledValues(now);
    oldGain.gain.setValueAtTime(oldGain.gain.value, now);
    oldGain.gain.linearRampToValueAtTime(0, now + 0.04);
    oldSource.stop(now + 0.06);

    this.startNew(ctx, 0.04);
  }

  private startNew(ctx: AudioContext, fadeIn: number): void {
    const buffer = toAudioBuffer(ctx, this.render(ctx.sampleRate));
    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.loop = true;

    const gain = ctx.createGain();
    const now = ctx.currentTime;
    gain.gain.setValueAtTime(0, now);
    gain.gain.linearRampToValueAtTime(this.level, now + fadeIn);

    source.connect(gain);
    gain.connect(output());
    source.start(now);

    this.source = source;
    this.gain = gain;
  }

  private notify(): void {
    for (const l of this.listeners) l(this.playing);
  }
}

/** Toca uma vez (sem loop). */
export async function playOnce(channels: Float32Array[], level = 0.5): Promise<void> {
  const ctx = audioContext();
  await ctx.resume();

  const source = ctx.createBufferSource();
  source.buffer = toAudioBuffer(ctx, channels);
  const gain = ctx.createGain();
  gain.gain.value = level;
  source.connect(gain);
  gain.connect(output());
  source.start();
}

/** Para todos os LoopPlayers registados (ao mudar de página). */
const activePlayers = new Set<LoopPlayer>();

export function track(player: LoopPlayer): LoopPlayer {
  activePlayers.add(player);
  return player;
}

export function stopAll(): void {
  for (const p of activePlayers) p.stop();
  activePlayers.clear();
}
