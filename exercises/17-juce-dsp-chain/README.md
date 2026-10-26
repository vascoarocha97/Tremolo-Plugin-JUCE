# 17 · Cadeias `juce::dsp`

> **Módulo 4 — Filtros** · ⏱ ~40 min · Visualização: [Biquad e resposta em frequência](http://localhost:5173/#/biquad)

## Objetivos

- Montar uma cadeia de processamento com `juce::dsp::ProcessorChain`.
- Usar `juce::dsp::IIR::Filter` em vários canais com `juce::dsp::ProcessorDuplicator`.
- Atualizar coeficientes de filtros na thread de áudio **sem alocar memória**.

## Teoria

### As peças

| classe | o que é |
|---|---|
| `IIR::Filter<float>` | um filtro IIR de **um canal**, com o seu estado; os coeficientes são partilhados via `Coefficients::Ptr` |
| `IIR::Coefficients<float>` | os coeficientes (objeto com contagem de referências) |
| `ProcessorDuplicator<Filter, Coefficients>` | cria um `Filter` por canal no `prepare`, todos a apontar para o mesmo `state` (os coeficientes) |
| `ProcessorChain<A, B, C>` | chama `prepare/process/reset` de A, B e C por ordem; `chain.get<i>()` dá acesso ao i-ésimo |
| `Gain<float>` | ganho com rampa (smoothing) |

### A armadilha das alocações

`IIR::Coefficients<float>::makeLowPass (...)` devolve um **`Ptr` para um objeto novo no heap**.
Chamá-lo no `process` aloca memória — e a atribuição `filter.coefficients = novo` pode ainda
**libertar** o objeto antigo na thread de áudio (se for a última referência). Ambas as coisas são
proibidas (exercício 05).

A alternativa, no JUCE 9: `IIR::ArrayCoefficients<float>::makeLowPass (...)` devolve um
`std::array<float, 6>` (na stack, sem alocar). E `Coefficients::operator= (const std::array&)`
copia os valores para o objeto que **já existe**: lendo `juce_IIRFilter_Impl.h`, o `assignImpl` usa
`clearQuick()` e `ensureStorageAllocated (8)`, ou seja, reaproveita a memória. Portanto:

```cpp
*chain.get<0>().state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sampleRate, freq);
```

atualiza os coeficientes de todos os canais sem tocar no allocator. O último teste muda os cortes
200 vezes com o contador de alocações ligado.

> Atenção à concorrência: isto é seguro porque é feito **na thread de áudio** (os setters só guardam
> os valores e marcam "precisa de atualizar"; a atualização é feita no início do `process`). Se a GUI
> escrevesse nos coeficientes enquanto o áudio os lê, seria uma data race.

### Ordem no `prepare`

O `IIR::Filter::prepare` usa a **ordem** dos coeficientes para reservar o estado. Por isso os
coeficientes calculam-se **antes** de `chain.prepare (spec)`.

### O valor inicial do `juce::dsp::Gain`

Lendo `juce_Gain.h`: o ganho é um `SmoothedValue<FloatType>` construído por omissão, ou seja,
**começa a 0** (silêncio) até alguém chamar `setGainDecibels`/`setGainLinear`. Se a tua cadeia ficar
muda, é isto. Define o ganho antes do `prepare`.

## O exercício

Em `Processor.cpp`: **TODO(1)** `prepare`, **TODO(2)** `updateCoefficients`, **TODO(3)** `process`.
Os setters (que só guardam valores e marcam a atualização) já estão feitos.

## Armadilhas comuns

- **`makeHighPass` (Ptr) em vez de `ArrayCoefficients`** no `process`: funciona e passa nos testes
  de som — mas não no de alocações.
- **`chain.prepare` antes de haver coeficientes**.
- **Gain a 0** (ver acima).
- **Passa-alto e passa-baixo trocados**: o "corte de graves" é um passa-**alto**.

## Como verificar

```powershell
./learn check 17
./learn run 17
```

## Para saber mais

- [Tutorial JUCE: Introduction to DSP](https://juce.com/tutorials/tutorial_dsp_introduction/) — `ProcessorChain`, `ProcessorDuplicator` e filtros IIR num exemplo completo.
- [juce::dsp::IIR::Filter](https://docs.juce.com/master/classjuce_1_1dsp_1_1IIR_1_1Filter.html),
  [IIR::Coefficients](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1Coefficients.html),
  [IIR::ArrayCoefficients](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1ArrayCoefficients.html),
  [ProcessorDuplicator](https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessorDuplicator.html),
  [ProcessorChain](https://docs.juce.com/master/classjuce_1_1dsp_1_1ProcessorChain.html) — documentação.
- O exemplo oficial `examples/Plugins/DSPModulePluginDemo.h` (em `build/_deps/juce-src/examples/`) monta uma cadeia `juce::dsp` completa.
