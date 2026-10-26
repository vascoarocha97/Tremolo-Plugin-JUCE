import type { PageDef } from "./types";
import { Plot, cssColor, audioFrequencyTicks, formatHz } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, biquadMagnitude, gainToDb, processBiquad, rbj, type Biquad, type BiquadType } from "../lib/dsp";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

const fs = 48000;

/** Raízes de z² + p·z + q (podem ser complexas). */
function quadraticRoots(p: number, q: number): Array<[number, number]> {
  const disc = p * p - 4 * q;
  if (disc >= 0) {
    const s = Math.sqrt(disc);
    return [[(-p + s) / 2, 0], [(-p - s) / 2, 0]];
  }
  const s = Math.sqrt(-disc);
  return [[-p / 2, s / 2], [-p / 2, -s / 2]];
}

export const biquadPage: PageDef = {
  id: "biquad",
  title: "Biquad e resposta em frequência",
  module: 4,
  summary: "Cinco coeficientes, dois polos, dois zeros. As fórmulas do Audio EQ Cookbook dão todos os filtros de um EQ.",
  render(root) {
    let type: BiquadType = "peaking";
    let f0 = 1000;
    let q = 1;
    let gain = 9;
    let coefficients: Biquad = rbj(type, fs, f0, q, gain);
    const info = readout();

    const response = new Plot(el("div"), {
      height: 260,
      x: { min: 20, max: 24000, log: true, ticks: audioFrequencyTicks, format: formatHz },
      y: { min: -36, max: 24, ticks: [-36, -24, -12, 0, 12, 24], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        pl.hline(0, cssColor("text-muted"), [], 1);
        pl.fn((f) => gainToDb(biquadMagnitude(coefficients, f, fs)), cssColor("signal"), 2.2, 800);
        pl.vline(f0, cssColor("signal-2"));
      },
    });

    const zPlane = new Plot(el("div"), {
      height: 280,
      x: { min: -1.6, max: 1.6, ticks: [-1, 0, 1] },
      y: { min: -1.2, max: 1.2, ticks: [-1, 0, 1] },
      draw: (pl) => {
        // Mesma escala nos dois eixos, para o círculo unitário ser um círculo
        const aspect = (pl.plotRight - pl.plotLeft) / (pl.plotBottom - pl.plotTop);
        pl.x = { min: -1.2 * aspect, max: 1.2 * aspect, ticks: [-1, 0, 1] };
        pl.grid();
        const ctx = pl.ctx;
        ctx.strokeStyle = cssColor("text-muted");
        ctx.beginPath();
        ctx.ellipse(pl.X(0), pl.Y(0), pl.X(1) - pl.X(0), pl.Y(0) - pl.Y(1), 0, 0, TWO_PI);
        ctx.stroke();

        // zeros: raízes de b0·z² + b1·z + b2
        const zeros = quadraticRoots(coefficients.b1 / coefficients.b0, coefficients.b2 / coefficients.b0);
        const poles = quadraticRoots(coefficients.a1, coefficients.a2);

        ctx.lineWidth = 2;
        for (const [re, im] of zeros) {
          ctx.strokeStyle = cssColor("signal-3");
          ctx.beginPath();
          ctx.arc(pl.X(re), pl.Y(im), 6, 0, TWO_PI);
          ctx.stroke();
        }
        for (const [re, im] of poles) {
          ctx.strokeStyle = cssColor("signal-2");
          const x = pl.X(re), y = pl.Y(im);
          ctx.beginPath();
          ctx.moveTo(x - 6, y - 6); ctx.lineTo(x + 6, y + 6);
          ctx.moveTo(x + 6, y - 6); ctx.lineTo(x - 6, y + 6);
          ctx.stroke();
        }
        // ângulo de f0 no círculo
        const w0 = (TWO_PI * f0) / fs;
        pl.line([0, Math.cos(w0)], [0, Math.sin(w0)], cssColor("text-muted"), 1, [3, 3]);
        pl.text(1.05, 0.05, "0 Hz", cssColor("text-muted"));
        pl.text(-1.05, 0.05, "fs/2", cssColor("text-muted"), "right");
      },
    });

    const update = () => {
      coefficients = rbj(type, fs, f0, q, gain);
      const c = coefficients;
      const poleRadius = Math.sqrt(Math.max(0, c.a2));
      info.set(
        `b0 = <strong>${c.b0.toFixed(5)}</strong>, b1 = <strong>${c.b1.toFixed(5)}</strong>, b2 = <strong>${c.b2.toFixed(5)}</strong>, ` +
          `a1 = <strong>${c.a1.toFixed(5)}</strong>, a2 = <strong>${c.a2.toFixed(5)}</strong> · ` +
          `|H(f0)| = <strong>${gainToDb(biquadMagnitude(c, f0, fs)).toFixed(2)} dB</strong> · raio dos polos = ${poleRadius.toFixed(4)} (estável se < 1)`,
      );
      response.redraw();
      zPlane.redraw();
      player.refresh();
    };

    let kind: SourceKind = "noise";
    const player = new LoopPlayer((rate) => {
      const c = rbj(type, rate, Math.min(f0, 0.49 * rate), q, gain);
      return renderSource(kind, rate, 0.35).map((ch) => processBiquad(ch, c));
    });

    root.append(
      el("h2", {}, "1. A curva de um EQ"),
      p("Escolhe o tipo e mexe na frequência, no Q e no ganho. A linha laranja é f0. Para o passa-baixo/alto com Q = 0.707, repara no −3 dB exato em f0; no peaking, no ganho exato em f0; nos shelves, em metade do ganho em f0."),
      panel(
        response.canvas,
        controls(
          select<BiquadType>("Tipo", [
            { value: "lowPass", label: "passa-baixo" },
            { value: "highPass", label: "passa-alto" },
            { value: "bandPass", label: "passa-banda (0 dB)" },
            { value: "notch", label: "notch" },
            { value: "peaking", label: "peaking" },
            { value: "lowShelf", label: "low shelf" },
            { value: "highShelf", label: "high shelf" },
          ], type, (v) => { type = v; update(); }),
          slider({ label: "f0", min: 20, max: 20000, value: f0, log: true, step: 1, format: (v) => `${v} Hz`, onInput: (v) => { f0 = v; update(); } }),
          slider({ label: "Q", min: 0.1, max: 20, value: q, log: true, format: (v) => v.toFixed(3), onInput: (v) => { q = v; update(); } }),
          slider({ label: "Ganho", min: -24, max: 24, step: 0.5, value: gain, format: (v) => `${v} dB`, onInput: (v) => { gain = v; update(); } }),
        ),
        info,
      ),
      el("h2", {}, "2. Polos e zeros no plano z"),
      p("Cada biquad tem 2 polos (×) e 2 zeros (○). O ângulo de um ponto no círculo unitário é a frequência (0 à direita, fs/2 à esquerda). Um polo perto do círculo faz um pico nessa frequência (sobe o Q e vê o polo aproximar-se); um zero em cima do círculo faz um zero na resposta (o notch). Polos fora do círculo = filtro instável."),
      panel(zPlane.canvas, legend([ { label: "polos", color: "signal-2" }, { label: "zeros", color: "signal-3" } ])),
      el("h2", {}, "3. Ouvir"),
      panel(controls(sourceSelect(kind, (k) => { kind = k; player.refresh(); }), playButton(player, "Ouvir"))),
      callout("Implementação (exercício 15): Transposed Direct Form II — `y = b0·x + s1; s1 = b1·x − a1·y + s2; s2 = b2·x − a2·y`. Calcula os coeficientes em `double` e só quando os parâmetros mudam."),
      sources([
        { title: "R. Bristow-Johnson — Audio EQ Cookbook (W3C)", url: "https://www.w3.org/TR/audio-eq-cookbook/", why: "as fórmulas de todos os tipos" },
        { title: "J. O. Smith — BiQuad Section", url: "https://ccrma.stanford.edu/~jos/filters/BiQuad_Section.html", why: "polos, zeros e a estrutura do biquad" },
        { title: "J. O. Smith — Pole-Zero Analysis", url: "https://ccrma.stanford.edu/~jos/filters/Pole_Zero_Analysis_I.html", why: "como ler um diagrama de polos e zeros" },
        { title: "juce::dsp::IIR::ArrayCoefficients", url: "https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1ArrayCoefficients.html", why: "as mesmas fórmulas no JUCE, sem alocar (exercício 17)" },
      ]),
    );

    update();
  },
};
