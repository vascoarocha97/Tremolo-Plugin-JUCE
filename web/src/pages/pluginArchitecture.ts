import type { PageDef } from "./types";
import { Plot, cssColor } from "../lib/plot";
import { LoopPlayer } from "../lib/audio";
import { button, callout, controls, el, legend, p, panel, playButton, readout, select, slider, sources, toggle } from "../lib/ui";
import { renderSource, sourceSelect, type SourceKind } from "../lib/sources";

// ---- Parâmetros do eco (iguais ao exercício 32) --------------------------------------------------

interface ParamDef {
  id: string;
  name: string;
  min: number;
  max: number;
  def: number;
  centre?: number;
  unit?: string;
  choices?: string[];
}

const params: ParamDef[] = [
  { id: "gain", name: "Ganho", min: -24, max: 12, def: 0, unit: "dB" },
  { id: "time", name: "Tempo", min: 1, max: 1000, def: 350, centre: 250, unit: "ms" },
  { id: "feedback", name: "Feedback", min: 0, max: 0.95, def: 0.4 },
  { id: "tone", name: "Tom", min: 500, max: 20000, def: 6000, centre: 3000, unit: "Hz" },
  { id: "mix", name: "Mistura", min: 0, max: 1, def: 0.3 },
  { id: "mode", name: "Modo", min: 0, max: 1, def: 0, choices: ["Mono", "Ping-pong"] },
  { id: "bypass", name: "Bypass", min: 0, max: 1, def: 0, choices: ["false", "true"] },
];

/** skew tal que convertFrom0to1(0.5) = centre (como NormalisableRange::setSkewForCentre). */
function skewFor(def: ParamDef): number {
  if (def.centre === undefined) return 1;
  return Math.log(0.5) / Math.log((def.centre - def.min) / (def.max - def.min));
}

function fromNormalised(def: ParamDef, x: number, skew = skewFor(def)): number {
  const proportion = skew !== 1 && x > 0 ? Math.exp(Math.log(x) / skew) : x;
  return def.min + (def.max - def.min) * proportion;
}

function toNormalised(def: ParamDef, v: number, skew = skewFor(def)): number {
  const proportion = (v - def.min) / (def.max - def.min);
  return skew !== 1 && proportion > 0 ? Math.pow(proportion, skew) : proportion;
}

function formatValue(def: ParamDef, v: number): string {
  if (def.choices) return def.choices[Math.round(v)];
  if (Math.abs(v) >= 100) return v.toFixed(0);
  return v.toFixed(2);
}

export const pluginArchitecturePage: PageDef = {
  id: "plugin-architecture",
  title: "Arquitetura de um plugin",
  module: 8,
  summary: "Host, processador, parâmetros e editor: quem vive em que thread, como um valor do slider chega ao áudio, e o que fica gravado no projeto.",
  render(root) {
    // ---- 1. Do slider ao áudio ---------------------------------------------------------------
    let blockSize = 512;
    let smoothingMs = 20;
    const fs = 48000;
    const span = 0.25; // s mostrados
    const flowInfo = readout();

    // "Gesto" do utilizador: sobe de −12 para 0 dB entre 30 e 90 ms e salta para −18 dB aos 150 ms.
    const gesture = (t: number) => (t < 0.03 ? -12 : t < 0.09 ? -12 + (12 * (t - 0.03)) / 0.06 : t < 0.15 ? 0 : -18);

    const flow = new Plot(el("div"), {
      height: 260,
      x: { min: 0, max: span * 1000, label: "tempo (ms)" },
      y: { min: -20, max: 2, ticks: [-18, -12, -6, 0], format: (v) => `${v} dB` },
      draw: (pl) => {
        pl.grid();
        const n = Math.floor(span * fs);
        const step = 8;
        const xs: number[] = [];
        const gui: number[] = [];
        const audio: number[] = [];
        const smooth: number[] = [];
        let blockValue = gesture(0);
        let current = blockValue;
        let target = blockValue;
        let increment = 0;
        let countdown = 0;
        const rampSamples = Math.max(1, Math.round((smoothingMs / 1000) * fs));
        for (let i = 0; i < n; i++) {
          if (i % blockSize === 0) {
            blockValue = gesture(i / fs); // o processBlock lê o atomic no início do bloco
            if (blockValue !== target) {
              target = blockValue;
              countdown = smoothingMs > 0 ? rampSamples : 0;
              increment = countdown > 0 ? (target - current) / countdown : 0;
              if (countdown === 0) current = target;
            }
          }
          if (countdown > 0) {
            current += increment;
            if (--countdown === 0) current = target;
          }
          if (i % step === 0) {
            xs.push((i / fs) * 1000);
            gui.push(gesture(i / fs));
            audio.push(blockValue);
            smooth.push(current);
          }
        }
        pl.line(xs, gui, cssColor("text-muted"), 1.2, [5, 4]);
        pl.line(xs, audio, cssColor("signal-2"), 1.8);
        pl.line(xs, smooth, cssColor("signal"), 2.2);
        for (let b = 0; b * blockSize < n; b++) pl.vline(((b * blockSize) / fs) * 1000, cssColor("grid"), [1, 5]);
      },
    });

    const updateFlow = () => {
      flowInfo.set(
        `bloco de <strong>${blockSize}</strong> amostras = <strong>${((blockSize / fs) * 1000).toFixed(1)} ms</strong> entre leituras do atomic · ` +
          (smoothingMs > 0 ? `rampa de <strong>${smoothingMs} ms</strong> no SmoothedValue` : `<strong>sem smoothing</strong>: degraus = "zipper noise"`),
      );
      flow.redraw();
    };

    // ---- 2. Skew ---------------------------------------------------------------------------------
    const timeDef = params[1];
    let centre = 250;
    const skewInfo = readout();
    const skewPlot = new Plot(el("div"), {
      height: 240,
      x: { min: 0, max: 1, ticks: [0, 0.25, 0.5, 0.75, 1], label: "posição do botão (valor normalizado 0..1)" },
      y: { min: 0, max: 1000, ticks: [0, 250, 500, 750, 1000], format: (v) => `${v} ms` },
      draw: (pl) => {
        pl.grid();
        const skew = Math.log(0.5) / Math.log((centre - timeDef.min) / (timeDef.max - timeDef.min));
        pl.fn((x) => fromNormalised(timeDef, x, 1), cssColor("text-muted"), 1.4, 300, [5, 4]);
        pl.fn((x) => fromNormalised(timeDef, x, skew), cssColor("signal"), 2, 300);
        pl.vline(0.5, cssColor("grid"));
      },
    });
    const updateSkew = () => {
      const skew = Math.log(0.5) / Math.log((centre - timeDef.min) / (timeDef.max - timeDef.min));
      const at = (x: number) => fromNormalised(timeDef, x, skew).toFixed(0);
      skewInfo.set(
        `<code>setSkewForCentre (${centre})</code> → skew = <strong>${skew.toFixed(3)}</strong> · posições 0.25 / 0.5 / 0.75 → ` +
          `<strong>${at(0.25)} / ${at(0.5)} / ${at(0.75)} ms</strong> (linear: 251 / 500 / 750 ms)`,
      );
      skewPlot.redraw();
    };

    // ---- 3. Estado ---------------------------------------------------------------------------------
    const values = new Map(params.map((d) => [d.id, d.def]));
    const xmlView = el("pre", { class: "code-block" });
    const updateXml = () => {
      const lines = params.map((d) => `  <PARAM id="${d.id}" value="${+(values.get(d.id) ?? d.def).toFixed(4)}"/>`);
      xmlView.textContent = `<?xml version="1.0" encoding="UTF-8"?>\n\n<PARAMETERS>\n${lines.join("\n")}\n</PARAMETERS>`;
    };
    const stateSliders = params.map((d) =>
      d.choices
        ? select(d.name, d.choices.map((c, i) => ({ value: String(i), label: c })), String(d.def), (v) => { values.set(d.id, Number(v)); updateXml(); })
        : slider({
            label: d.name,
            min: 0,
            max: 1,
            step: 0.001,
            value: toNormalised(d, d.def),
            format: (x) => `${formatValue(d, fromNormalised(d, x))}${d.unit ? " " + d.unit : ""}`,
            onInput: (x) => { values.set(d.id, fromNormalised(d, x)); updateXml(); },
          }),
    );

    // Estado antigo sem o "tone": o que acontece?
    let resetFirst = true;
    const oldStateInfo = readout();
    const loadOld = () => {
      const before = values.get("tone") ?? 6000;
      const result = resetFirst ? 6000 : before;
      oldStateInfo.set(
        `Estado antigo sem <code>tone</code> carregado. O tom passou de <strong>${before.toFixed(0)} Hz</strong> para <strong>${result.toFixed(0)} Hz</strong> ` +
          (resetFirst ? "(valor por omissão: correto)." : "(ficou o valor que estava — vindo de outro projeto!)."),
      );
    };

    // ---- 4. Ouvir: zipper noise ------------------------------------------------------------------
    let source: SourceKind = "sine";
    let audioBlock = 512;
    let smoothing = false;
    let modRate = 6;
    const player = new LoopPlayer((rate) => {
      const input = renderSource(source, rate, 0.5);
      const length = input[0].length;
      const ramp = Math.max(1, Math.round(0.02 * rate));
      const out = input.map(() => new Float32Array(length));
      let current = 1;
      let target = 1;
      let increment = 0;
      let countdown = 0;
      for (let i = 0; i < length; i++) {
        if (i % audioBlock === 0) {
          // automação: um "LFO" de ganho entre −12 e 0 dB que o host manda bloco a bloco
          const db = -6 + 6 * Math.sin((2 * Math.PI * modRate * i) / rate);
          target = Math.pow(10, db / 20);
          if (smoothing) {
            countdown = ramp;
            increment = (target - current) / ramp;
          } else {
            current = target;
            countdown = 0;
          }
        }
        if (countdown > 0) {
          current += increment;
          if (--countdown === 0) current = target;
        }
        for (let ch = 0; ch < input.length; ch++) out[ch][i] = input[ch][i] * current;
      }
      return out;
    });

    root.append(
      el("h2", {}, "1. Do slider ao áudio"),
      p("O utilizador arrasta o slider do ganho (tracejado). O attachment escreve no parâmetro; o `processBlock` só lê o atomic **uma vez por bloco** — o valor que o áudio vê muda aos degraus. O `SmoothedValue` transforma os degraus numa rampa."),
      panel(
        flow.canvas,
        legend([{ label: "slider (message thread)", color: "text-muted" }, { label: "lido no processBlock", color: "signal-2" }, { label: "depois do SmoothedValue", color: "signal" }]),
        controls(
          select("Bloco", ["64", "256", "512", "1024", "2048"].map((v) => ({ value: v, label: `${v} amostras` })), String(blockSize), (v) => { blockSize = Number(v); updateFlow(); }),
          slider({ label: "Smoothing", min: 0, max: 50, step: 1, value: smoothingMs, format: (v) => (v === 0 ? "nenhum" : `${v} ms`), onInput: (v) => { smoothingMs = v; updateFlow(); } }),
        ),
        flowInfo,
      ),
      el("h2", {}, "2. Valor normalizado e skew"),
      p("O host só conhece valores **0..1** (é isso que grava na automação). A `NormalisableRange` converte para o valor real. Com skew, o meio do curso do botão fica onde o ouvido precisa de resolução."),
      panel(
        skewPlot.canvas,
        legend([{ label: "linear", color: "text-muted" }, { label: "com skew", color: "signal" }]),
        controls(slider({ label: "Centro", min: 20, max: 500, step: 5, value: centre, format: (v) => `${v} ms`, onInput: (v) => { centre = v; updateSkew(); } })),
        skewInfo,
      ),
      el("h2", {}, "3. O que fica gravado no projeto"),
      p("`getStateInformation` → `apvts.copyState().createXml()` → bytes. Mexe nos parâmetros e vê o XML (no plugin, o JUCE acrescenta um cabeçalho binário à volta disto). Os valores gravados são **reais**, não normalizados."),
      panel(controls(...stateSliders), xmlView),
      p("Um projeto gravado por uma versão antiga do plugin pode não ter um parâmetro novo. O `replaceState` do APVTS deixa esse parâmetro com o valor que **tinha** — que pode vir do projeto anterior. Mexe no Tom acima e carrega:"),
      panel(
        controls(
          toggle("repor omissões antes do replaceState (exercício 33)", resetFirst, (v) => { resetFirst = v; }),
          button("Carregar estado antigo", loadOld),
        ),
        oldStateInfo,
      ),
      el("h2", {}, "4. Ouvir: zipper noise"),
      p("Um \"LFO\" de ganho que o host atualiza bloco a bloco. Sem smoothing, cada degrau é um clique — com blocos grandes ouve-se um zumbido à frequência dos blocos (o "zipper noise"). Com smoothing de 20 ms desaparece."),
      panel(
        controls(
          sourceSelect(source, (v) => { source = v; player.refresh(); }, ["sine", "loop", "noise"]),
          select("Bloco", ["64", "512", "2048"].map((v) => ({ value: v, label: `${v} amostras` })), String(audioBlock), (v) => { audioBlock = Number(v); player.refresh(); }),
          slider({ label: "Velocidade", min: 0.5, max: 12, step: 0.5, value: modRate, format: (v) => `${v} Hz`, onInput: (v) => { modRate = v; player.refresh(); } }),
          toggle("SmoothedValue (20 ms)", smoothing, (v) => { smoothing = v; player.refresh(); }),
          playButton(player, "Ouvir"),
        ),
      ),
      callout("Regra de ouro: a GUI e o áudio **nunca** se chamam diretamente. Falam através dos parâmetros (atomics). Nada de locks, alocações ou chamadas à GUI no `processBlock`."),
      sources([
        { title: "Tutorial JUCE: Saving and loading your plug-in state", url: "https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/", why: "APVTS, attachments e estado em XML" },
        { title: "juce::AudioProcessorValueTreeState — documentação", url: "https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html", why: "a classe central do módulo 8" },
        { title: "juce::NormalisableRange — documentação", url: "https://docs.juce.com/master/classjuce_1_1NormalisableRange.html", why: "conversão 0..1 ↔ valor real e skew" },
        { title: "JUCE — CMake API (juce_add_plugin)", url: "https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md", why: "como se compila um VST3" },
        { title: "pluginval (Tracktion)", url: "https://github.com/Tracktion/pluginval", why: "validar o plugin como um host exigente" },
      ]),
    );

    updateFlow();
    updateSkew();
    updateXml();
  },
};
