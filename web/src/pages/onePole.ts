import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, gainToDb, onePole } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;

/** |H| do passa-baixo de um polo com a = 1 − e^(−2π·fc/fs). */
function lowPassMagnitude(f: number, fc: number): number {
  const a = 1 - Math.exp((-TWO_PI * fc) / fs);
  const w = (TWO_PI * f) / fs;
  const b = 1 - a;
  return a / Math.hypot(1 - b * Math.cos(w), b * Math.sin(w));
}

function highPassMagnitude(f: number, fc: number): number {
  const a = 1 - Math.exp((-TWO_PI * fc) / fs);
  const w = (TWO_PI * f) / fs;
  const b = 1 - a;
  // H_HP = 1 − H_LP = b·(1 − z⁻¹)/(1 − b·z⁻¹)
  return (b * Math.hypot(1 - Math.cos(w), Math.sin(w))) / Math.hypot(1 - b * Math.cos(w), b * Math.sin(w));
}

export const onePolePage: PageDef = {
  id: "one-pole",
  title: "Filtro de um polo",
  module: 4,
  summary: "`y += a·(x − y)`: a saída persegue a entrada. Uma constante de tempo, um polo, −6 dB por oitava.",
  render(root) {
    let fc = 500;
    const info = readout();

    const step = new Plot(el("div"), {
      height: 220,
      x: { min: 0, max: 5, label: "tempo (ms)" },
      y: { min: 0, max: 1.05, ticks: [0, 0.632, 1] },
      draw: (pl) => {
        const tauMs = 1000 / (TWO_PI * fc);
        pl.x = { min: 0, max: 5 * tauMs, label: "tempo (ms)" };
        pl.grid();
        const n = Math.ceil((5 * tauMs * fs) / 1000);
        const input = new Float32Array(n).fill(1);
        const y = onePole(input, fc, fs);
        const xs = Array.from({ length: n }, (_, i) => ((i + 1) * 1000) / fs);
        pl.fn((t) => 1 - Math.exp(-t / tauMs), cssColor("text-muted"), 4);
        pl.line(xs, y, cssColor("signal"), 1.6);
        pl.vline(tauMs, cssColor("signal-2"));
        pl.hline(1 - Math.exp(-1), cssColor("signal-2"));
        pl.text(tauMs * 1.05, 0.15, `τ = ${tauMs.toFixed(2)} ms`, cssColor("signal-2"));
      },
    });

    const response = new Plot(el("div"), {
      height: 240,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -48, max: 3, ticks: [-48, -36, -24, -12, -6, -3, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        pl.fn((f) => gainToDb(lowPassMagnitude(f, fc)), cssColor("signal"), 2);
        pl.fn((f) => gainToDb(highPassMagnitude(f, fc)), cssColor("signal-2"), 2);
        pl.vline(fc, cssColor("text-muted"));
      },
    });

    const update = () => {
      const a = 1 - Math.exp((-TWO_PI * fc) / fs);
      info.set(
        `a = 1 − e^(−2π·${fc.toFixed(0)}/${fs}) = <strong>${a.toFixed(5)}</strong> (aproximação 2π·fc/fs = ${((TWO_PI * fc) / fs).toFixed(5)}) · ` +
          `polo em z = <strong>${(1 - a).toFixed(5)}</strong> · ganho em fc: <strong>${gainToDb(lowPassMagnitude(fc, fc)).toFixed(2)} dB</strong>`,
      );
      step.redraw();
      response.redraw();
    };

    let kind: SourceKind = "noise";
    let mode: "lp" | "hp" = "lp";
    let audioCutoff = 800;
    const player = new LoopPlayer((rate) => renderSource(kind, rate).map((ch) => onePole(ch, audioCutoff, rate, mode === "hp")));

    root.append(
      el("h2", {}, "1. Resposta ao degrau"),
      p("A entrada salta de 0 para 1. A saída aproxima-se exponencialmente: ao fim de uma constante de tempo τ = 1/(2π·fc) está a 63.2%. A curva grossa cinzenta é o circuito RC analógico — com a = 1 − e^(−2π·fc/fs) as amostras caem exatamente em cima dela."),
      panel(
        step.canvas,
        legend([
          { label: "filtro digital", color: "signal" },
          { label: "RC analógico: 1 − e^(−t/τ)", color: "text-muted" },
        ]),
        controls(slider({ label: "Corte", min: 20, max: 20000, value: fc, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { fc = v; update(); } })),
        info,
      ),
      el("h2", {}, "2. Resposta em frequência"),
      p("Passa-baixo e passa-alto (= entrada − passa-baixo) cruzam-se perto de −3 dB. Acima do corte o passa-baixo cai 6 dB por cada duplicação da frequência (uma oitava)."),
      panel(response.canvas, legend([ { label: "passa-baixo", color: "signal" }, { label: "passa-alto", color: "signal-2" } ])),
      el("h2", {}, "3. Ouvir"),
      panel(
        controls(
          sourceSelect(kind, (k) => { kind = k; player.refresh(); }),
          select("Tipo", [ { value: "lp", label: "passa-baixo" }, { value: "hp", label: "passa-alto" } ], mode, (v) => { mode = v; player.refresh(); }),
          slider({ label: "Corte", min: 20, max: 20000, value: audioCutoff, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { audioCutoff = v; player.refresh(); } }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("O mesmo filtro, com um corte de poucos Hz aplicado ao valor de um parâmetro (em vez do áudio), é o smoothing exponencial. Com um corte de 5–20 Hz como passa-alto, é um DC blocker (exercício 23)."),
      sources([
        { title: "J. O. Smith — Introduction to Digital Filters: One-Pole", url: "https://ccrma.stanford.edu/~jos/filters/One_Pole.html", why: "o filtro, o polo e a resposta" },
        { title: "EarLevel Engineering — A one-pole filter", url: "https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/", why: "a fórmula do coeficiente e usos práticos" },
      ]),
    );

    update();
  },
};
