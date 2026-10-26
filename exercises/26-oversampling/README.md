# 26 · Oversampling

> **Módulo 6 — Distorção** · ⏱ ~45 min · Visualização: [Oversampling](http://localhost:5173/#/oversampling)

## Objetivos

- Reduzir o aliasing de uma distorção com `juce::dsp::Oversampling`.
- Perceber a **latência** que o oversampling introduz e como se comunica ao host.
- Preparar tudo no `prepare` para o `process` não alocar.

## Teoria

### A ideia

O aliasing de uma distorção vem de harmónicos acima de Nyquist que dobram para baixo. Se o
processamento não-linear correr a um sample rate N vezes maior, há espaço para esses harmónicos:

```
1. upsample  (fs -> N·fs): intercala zeros e filtra (interpolação)
2. distorce a N·fs: os harmónicos até N·fs/2 são representados sem dobrar
3. downsample (N·fs -> fs): filtra TUDO acima do fs/2 original, e só depois descarta amostras
```

O passo 3 é o que remove os harmónicos que, sem oversampling, teriam dobrado. Os que ainda passam de
N·fs/2 continuam a dobrar (lá em cima) — por isso 2× ajuda, 4× ajuda mais, 8×/16× para distorções muito
agressivas. O teste mede o nível nas frequências de alias de um hard clip de 5 kHz (3, 7 e 13 kHz) e
exige uma redução de pelo menos 20 dB com 4×.

### `juce::dsp::Oversampling`

```cpp
juce::dsp::Oversampling<float> os (numCanais, factorLog2, filterHalfBandPolyphaseIIR, isMaxQuality, useIntegerLatency);
os.initProcessing (maxBlockSize);                 // no prepare: reserva os buffers
auto up = os.processSamplesUp (block);            // AudioBlock N vezes maior (memória do próprio os)
// ... processar `up` ...
os.processSamplesDown (block);                    // escreve no bloco original
```

- O fator é **log₂**: 2 → 4×.
- Filtros: `filterHalfBandPolyphaseIIR` (pouca latência, fase não linear) ou
  `filterHalfBandFIREquiripple` (fase linear, mais latência). Para distorção num plugin de guitarra, a
  IIR é a escolha comum; num mastering, a FIR.
- Tudo o que corre "lá em cima" vê um sample rate N vezes maior: smoothers, filtros, LFOs dentro da
  parte sobreamostrada têm de ser preparados com `fs·N`.

### Latência

Os filtros atrasam o sinal. `getLatencyInSamples()` diz quanto (ao sample rate original; pode ser
fracionário, a menos que se peça `useIntegerLatency`). Num plugin, o processador chama
`setLatencySamples (...)` e o host atrasa as outras pistas para tudo continuar alinhado (*plugin delay
compensation*). O standalone do exercício faz isso.

### Custos e alternativas

Oversampling multiplica o custo da parte não-linear por N (e acrescenta os filtros). Alternativas
estudadas e usadas: **ADAA** (*antiderivative antialiasing*, ver o artigo da DAFx 2016) e escolher
curvas mais suaves (o tanh tem menos aliasing do que o hard clip — exercício 25).

## O exercício

Em `Processor.cpp`: **TODO(1)** `prepare` e **TODO(2)** `process`. Usas as tuas curvas do exercício 22.

## Armadilhas comuns

- **Criar o `Oversampling` no `process`** ou chamar `initProcessing` lá: aloca.
- **Blocos maiores do que o `maximumBlockSize`** do `initProcessing`: o JUCE faz `jassert`.
- **Smoothing ao sample rate errado**: a rampa dura N vezes menos do que pensas.
- **Esquecer a latência**: num DAW, a pista com o plugin fica atrasada em relação às outras.

## Como verificar

```powershell
./learn check 26
./learn run 26     # "Seno" a 3–5 kHz, hard clip, drive alto: compara 1× e 8× no espectro
```

## Para saber mais

- [juce::dsp::Oversampling — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html) — construtor, tipos de filtro, latência.
- J. Parker, V. Zavalishin, E. Le Bivic, ["Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution" (DAFx 2016)](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf) — ADAA, a alternativa ao oversampling.
- Julius O. Smith III, ["Digital Audio Resampling"](https://ccrma.stanford.edu/~jos/resample/) — a teoria por trás de subir e descer o sample rate.
- O exemplo oficial `examples/Plugins/DSPModulePluginDemo.h` usa `juce::dsp::Oversampling` à volta de uma distorção.
