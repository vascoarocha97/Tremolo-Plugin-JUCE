// Gráficos simples em <canvas>, com eixos, grelha e cores do tema (variáveis CSS).

import { onDispose } from "./lifecycle";

export interface Axis {
  min: number;
  max: number;
  log?: boolean;
  ticks?: number[];
  format?: (value: number) => string;
  label?: string;
}

export interface PlotOptions {
  height?: number;
  x: Axis;
  y: Axis;
  draw: (plot: Plot) => void;
}

const themeQuery = window.matchMedia("(prefers-color-scheme: dark)");

export function cssColor(name: string): string {
  return getComputedStyle(document.documentElement).getPropertyValue(`--${name}`).trim() || "#888";
}

export class Plot {
  readonly canvas: HTMLCanvasElement;
  readonly ctx: CanvasRenderingContext2D;
  width = 0;
  height = 0;
  margin = { left: 48, right: 14, top: 12, bottom: 28 };
  x: Axis;
  y: Axis;
  private readonly drawFn: (plot: Plot) => void;
  private readonly observer: ResizeObserver;
  private readonly onTheme = () => this.redraw();

  constructor(parent: HTMLElement, options: PlotOptions) {
    this.canvas = document.createElement("canvas");
    this.canvas.className = "plot";
    this.canvas.style.height = `${options.height ?? 220}px`;
    parent.appendChild(this.canvas);

    const ctx = this.canvas.getContext("2d");
    if (!ctx) throw new Error("Canvas 2D indisponível");
    this.ctx = ctx;

    this.x = options.x;
    this.y = options.y;
    this.drawFn = options.draw;

    this.observer = new ResizeObserver(() => this.redraw());
    this.observer.observe(this.canvas);
    themeQuery.addEventListener("change", this.onTheme);
    onDispose(() => this.dispose());
  }

  dispose(): void {
    this.observer.disconnect();
    themeQuery.removeEventListener("change", this.onTheme);
  }

  setAxes(x?: Axis, y?: Axis): void {
    if (x) this.x = x;
    if (y) this.y = y;
    this.redraw();
  }

  redraw(): void {
    const rect = this.canvas.getBoundingClientRect();
    if (rect.width === 0) return;

    const dpr = window.devicePixelRatio || 1;
    this.width = rect.width;
    this.height = rect.height;
    this.canvas.width = Math.round(rect.width * dpr);
    this.canvas.height = Math.round(rect.height * dpr);
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.ctx.clearRect(0, 0, this.width, this.height);
    this.drawFn(this);
  }

  // ---- coordenadas -------------------------------------------------------------------------

  get plotLeft(): number { return this.margin.left; }
  get plotRight(): number { return this.width - this.margin.right; }
  get plotTop(): number { return this.margin.top; }
  get plotBottom(): number { return this.height - this.margin.bottom; }

  private norm(axis: Axis, v: number): number {
    if (axis.log) return Math.log(v / axis.min) / Math.log(axis.max / axis.min);
    return (v - axis.min) / (axis.max - axis.min);
  }

  X(v: number): number {
    return this.plotLeft + this.norm(this.x, v) * (this.plotRight - this.plotLeft);
  }

  Y(v: number): number {
    return this.plotBottom - this.norm(this.y, v) * (this.plotBottom - this.plotTop);
  }

  // ---- desenho -----------------------------------------------------------------------------

  grid(): void {
    const ctx = this.ctx;
    ctx.save();
    ctx.font = "11px " + getComputedStyle(document.body).fontFamily;
    ctx.lineWidth = 1;

    const xt = this.x.ticks ?? linearTicks(this.x.min, this.x.max, 6);
    const yt = this.y.ticks ?? linearTicks(this.y.min, this.y.max, 4);
    const fx = this.x.format ?? formatNumber;
    const fy = this.y.format ?? formatNumber;

    for (const t of xt) {
      const px = Math.round(this.X(t)) + 0.5;
      ctx.strokeStyle = cssColor("grid");
      ctx.beginPath();
      ctx.moveTo(px, this.plotTop);
      ctx.lineTo(px, this.plotBottom);
      ctx.stroke();
      ctx.fillStyle = cssColor("text-muted");
      ctx.textAlign = "center";
      ctx.fillText(fx(t), px, this.plotBottom + 15);
    }

    for (const t of yt) {
      const py = Math.round(this.Y(t)) + 0.5;
      ctx.strokeStyle = cssColor("grid");
      ctx.beginPath();
      ctx.moveTo(this.plotLeft, py);
      ctx.lineTo(this.plotRight, py);
      ctx.stroke();
      ctx.fillStyle = cssColor("text-muted");
      ctx.textAlign = "right";
      ctx.fillText(fy(t), this.plotLeft - 6, py + 4);
    }

    ctx.fillStyle = cssColor("text-muted");
    if (this.x.label) {
      ctx.textAlign = "right";
      ctx.fillText(this.x.label, this.plotRight, this.height - 2);
    }
    if (this.y.label) {
      ctx.textAlign = "left";
      ctx.fillText(this.y.label, this.plotLeft + 4, this.plotTop + 10);
    }
    ctx.restore();
  }

  /** Linha por pontos (xs, ys). Se xs for null, x = índice. */
  line(xs: ArrayLike<number> | null, ys: ArrayLike<number>, color: string, width = 1.6, dash: number[] = []): void {
    const ctx = this.ctx;
    ctx.save();
    this.clip();
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.setLineDash(dash);
    ctx.beginPath();
    for (let i = 0; i < ys.length; i++) {
      const px = this.X(xs ? xs[i] : i);
      const py = this.Y(ys[i]);
      if (i === 0) ctx.moveTo(px, py);
      else ctx.lineTo(px, py);
    }
    ctx.stroke();
    ctx.restore();
  }

  /** Desenha y = f(x) no intervalo do eixo x. */
  fn(f: (x: number) => number, color: string, width = 1.6, samples = 600, dash: number[] = []): void {
    const xs: number[] = [];
    const ys: number[] = [];
    for (let i = 0; i <= samples; i++) {
      const t = i / samples;
      const x = this.x.log ? this.x.min * Math.pow(this.x.max / this.x.min, t) : this.x.min + t * (this.x.max - this.x.min);
      xs.push(x);
      ys.push(f(x));
    }
    this.line(xs, ys, color, width, dash);
  }

  /** "Hastes" (como num gráfico de amostras): uma linha vertical do zero até ao valor e um ponto. */
  stems(xs: ArrayLike<number> | null, ys: ArrayLike<number>, color: string, radius = 3): void {
    const ctx = this.ctx;
    ctx.save();
    this.clip();
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineWidth = 1.2;
    const y0 = this.Y(Math.max(this.y.min, Math.min(this.y.max, 0)));
    for (let i = 0; i < ys.length; i++) {
      const px = this.X(xs ? xs[i] : i);
      const py = this.Y(ys[i]);
      ctx.beginPath();
      ctx.moveTo(px, y0);
      ctx.lineTo(px, py);
      ctx.stroke();
      ctx.beginPath();
      ctx.arc(px, py, radius, 0, 2 * Math.PI);
      ctx.fill();
    }
    ctx.restore();
  }

  vline(x: number, color: string, dash: number[] = [4, 4], width = 1): void {
    const ctx = this.ctx;
    ctx.save();
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.setLineDash(dash);
    ctx.beginPath();
    ctx.moveTo(this.X(x), this.plotTop);
    ctx.lineTo(this.X(x), this.plotBottom);
    ctx.stroke();
    ctx.restore();
  }

  hline(y: number, color: string, dash: number[] = [4, 4], width = 1): void {
    const ctx = this.ctx;
    ctx.save();
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.setLineDash(dash);
    ctx.beginPath();
    ctx.moveTo(this.plotLeft, this.Y(y));
    ctx.lineTo(this.plotRight, this.Y(y));
    ctx.stroke();
    ctx.restore();
  }

  text(x: number, y: number, str: string, color = cssColor("text"), align: CanvasTextAlign = "left"): void {
    const ctx = this.ctx;
    ctx.save();
    ctx.font = "12px " + getComputedStyle(document.body).fontFamily;
    ctx.fillStyle = color;
    ctx.textAlign = align;
    ctx.fillText(str, this.X(x), this.Y(y));
    ctx.restore();
  }

  rect(x0: number, y0: number, x1: number, y1: number, fill: string): void {
    const ctx = this.ctx;
    ctx.save();
    this.clip();
    ctx.fillStyle = fill;
    ctx.fillRect(this.X(x0), this.Y(y1), this.X(x1) - this.X(x0), this.Y(y0) - this.Y(y1));
    ctx.restore();
  }

  private clip(): void {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.rect(this.plotLeft, this.plotTop - 1, this.plotRight - this.plotLeft, this.plotBottom - this.plotTop + 2);
    ctx.clip();
  }
}

export function linearTicks(min: number, max: number, count: number): number[] {
  const span = max - min;
  const raw = span / count;
  const mag = Math.pow(10, Math.floor(Math.log10(raw)));
  const step = [1, 2, 2.5, 5, 10].map((m) => m * mag).find((s) => span / s <= count) ?? raw;
  const ticks: number[] = [];
  for (let t = Math.ceil(min / step) * step; t <= max + step * 1e-9; t += step) ticks.push(Math.abs(t) < step * 1e-9 ? 0 : t);
  return ticks;
}

export function formatNumber(v: number): string {
  if (Math.abs(v) >= 1000) return `${+(v / 1000).toFixed(1)}k`;
  if (Math.abs(v) < 0.01 && v !== 0) return v.toExponential(0);
  return `${+v.toFixed(2)}`;
}

export const formatHz = (v: number): string => (v >= 1000 ? `${+(v / 1000).toFixed(1)} kHz` : `${+v.toFixed(0)} Hz`);
export const audioFrequencyTicks = [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000];
