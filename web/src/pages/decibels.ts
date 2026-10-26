import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { controls, el, legend, p, panel, playButton, readout, select, slider, sources, callout } from "../lib/ui";
import { TWO_PI, dbToGain, demoLoop, gainToDb } from "../lib/dsp";

export const decibelsPage: PageDef = {
  id: "decibels",
  title: "Decibéis",
  module: 2,
  summary: "`dB = 20·log10(ganho)` para amplitudes, `10·log10(razão)` para potências. Somar dB é multiplicar ganhos.",
  render(root) {
    let db = -6;
    const info = readout();

    const curve = new Plot(el("div"), {
      height: 260,
      x: { min: -60, max: 12, label: "dB", ticks: [-60, -48, -36, -24, -12, -6, 0, 6, 12] },
      y: { min: 0, max: 4, label: "ganho linear", ticks: [0, 0.5, 1, 2, 3, 4] },
      draw: (pl) => {
        pl.grid();
        pl.fn((x) => dbToGain(x), cssColor("signal"), 2);
        pl.vline(db, cssColor("signal-2"), [4, 3]);
        pl.hline(dbToGain(db), cssColor("signal-2"), [4, 3]);
        pl.text(db + 0.8, dbToGain(db) + 0.15, `${db.toFixed(1)} dB = ×${dbToGain(db).toFixed(3)}`, cssColor("signal-2"));
      },
    });

    const update = () => {
      const g = dbToGain(db);
      info.set(
        `${db.toFixed(1)} dB → ganho de amplitude <strong>×${g.toFixed(4)}</strong> · razão de potência <strong>×${(g * g).toFixed(4)}</strong> · ` +
          `de volta: 20·log10(${g.toFixed(4)}) = <strong>${gainToDb(g).toFixed(2)} dB</strong>`,
      );
      curve.redraw();
    };

    const table = el(
      "table",
      { class: "exercises" },
      el(
        "tbody",
        {},
        ...[
          [2, "+6.02 dB", "dobro da amplitude"],
          [Math.SQRT2, "+3.01 dB", "dobro da potência"],
          [1, "0 dB", "igual"],
          [0.5, "−6.02 dB", "metade da amplitude"],
          [0.1, "−20 dB", "um décimo"],
          [0.001, "−60 dB", "\"quase silêncio\""],
          [0, "−∞ dB", "silêncio (o JUCE usa −100 dB como piso)"],
        ].map(([g, d, why]) => el("tr", {}, el("td", {}, `×${(+g).toPrecision(3)}`), el("td", {}, String(d)), el("td", { class: "summary" }, String(why)))),
      ),
    );

    // A/B: alterna entre 0 dB e o valor escolhido a cada 0.6 s
    let source: "sine" | "loop" = "loop";
    let loopCache: { rate: number; data: Float32Array[] } | null = null;
    const player = new LoopPlayer((rate) => {
      const half = Math.floor(rate * 0.6);
      let base: Float32Array[];
      if (source === "sine") {
        base = [new Float32Array(2 * half).map((_, i) => 0.5 * Math.sin((TWO_PI * 440 * i) / rate))];
      } else {
        if (!loopCache || loopCache.rate !== rate) loopCache = { rate, data: demoLoop(rate) };
        base = loopCache.data.map((ch) => ch.slice(0, 2 * half).map((v) => v * 0.5));
      }
      const g = dbToGain(db);
      return base.map((ch) => ch.map((v, i) => (i < half ? v : v * g)));
    });

    root.append(
      el("h2", {}, "1. A curva"),
      p("Os dB são uma escala logarítmica: cada 20 dB é um fator de 10 na amplitude, cada ~6 dB um fator de 2. Por isso um slider de ganho em dB é \"igualmente sensível\" em toda a gama — e um slider linear não."),
      panel(
        curve.canvas,
        legend([{ label: "ganho = 10^(dB/20)", color: "signal" }]),
        controls(slider({ label: "Ganho", min: -60, max: 12, step: 0.1, value: db, format: (v) => `${v.toFixed(1)} dB`, onInput: (v) => { db = v; update(); player.refresh(); } })),
        info,
      ),
      panel(table),
      el("h2", {}, "2. Ouvir: A/B"),
      p("O som alterna entre 0 dB (referência) e o valor escolhido acima, a cada 0.6 s. Repara que −6 dB não soa a \"metade do volume\": a perceção de intensidade não é linear (um valor frequentemente citado é ~10 dB para \"metade\", mas depende do som e de quem ouve)."),
      panel(
        controls(
          select("Fonte", [ { value: "loop", label: "loop de demonstração" }, { value: "sine", label: "seno 440 Hz" } ], source, (v) => { source = v; player.refresh(); }),
          playButton(player, "Ouvir A/B"),
        ),
      ),
      callout("No código: converte dB → linear **quando o parâmetro muda** (ou uma vez por bloco), nunca com `std::pow` em cada amostra."),
      sources([
        { title: "J. O. Smith — Decibels", url: "https://ccrma.stanford.edu/~jos/mdft/Decibels.html", why: "definição, amplitude vs. potência, escalas" },
        { title: "juce::Decibels — documentação", url: "https://docs.juce.com/master/classjuce_1_1Decibels.html", why: "as funções do JUCE e o piso de −100 dB" },
      ]),
    );

    update();
  },
};
