# 09 · Ganho suave e thread-safe

> **Módulo 2 — Ganho, dB, pan e smoothing** · ⏱ ~30 min · Visualização: [Smoothing e clicks](http://localhost:5173/#/smoothing)

## Objetivos

- Passar um parâmetro da GUI para a thread de áudio com `std::atomic` (e ler uma vez por bloco).
- Usar o `juce::SmoothedValue` do JUCE (já sabes o que faz por dentro).
- Conhecer a convenção dos processadores `juce::dsp`: `prepare`, `process`, `reset`.

## Teoria

### A convenção `juce::dsp`

Os blocos do módulo `juce_dsp` (filtros, delays, ganho, compressor...) têm todos a mesma forma:

| método | quando | o que recebe |
|---|---|---|
| `prepare (const ProcessSpec&)` | antes de processar (pode alocar) | `sampleRate`, `maximumBlockSize`, `numChannels` |
| `process (const ProcessContextReplacing<float>&)` | thread de áudio | o bloco a processar "no sítio" |
| `reset()` | quando o estado deve ser limpo (sem alocar) | — |

O `ProcessContextReplacing` embrulha um `juce::dsp::AudioBlock<float>`: uma **vista** (não é dono da
memória) sobre canais × amostras, com `getNumChannels()`, `getNumSamples()` (ambos `size_t`) e
`getChannelPointer (ch)`. Tem ainda `isBypassed`: por convenção, um processador em bypass não mexe
no áudio — mas o tempo continua a passar (é por isso que o `juce::dsp::Gain` faz `gain.skip(...)` em
bypass).

Como todos os blocos têm esta forma, podes encadeá-los com `juce::dsp::ProcessorChain<A, B, C>`, que
chama `prepare/process/reset` de cada um por ordem. Um dos testes encaixa o teu `SmoothedGain` numa
cadeia com o `juce::dsp::Gain`.

### O parâmetro vem de outra thread

O ganho é mudado pela GUI (ou pela automação do host, noutra thread). Se a GUI escrever diretamente
num `float` que a thread de áudio está a ler, é uma **data race**: comportamento indefinido em C++.
O `std::atomic<float>` resolve: `store()` numa thread, `load()` na outra, sem locks.

E repara num detalhe do próprio JUCE: `juce::dsp::Gain::setGainLinear()` chama
`SmoothedValue::setTargetValue()`, que **não é thread-safe** (mexe em vários membros não atómicos).
Por isso o padrão comum é: a GUI escreve num atómico; o `process()` lê o atómico **uma vez no início
do bloco** e é aí, na thread de áudio, que chama `setTargetValue()`. Ler uma vez por bloco também
garante que o valor não muda a meio do bloco.

> Com a `juce::AudioProcessorValueTreeState` (módulo 8) vais ter isto feito por ti:
> `getRawParameterValue()` devolve um `std::atomic<float>*`.

### Ordem de memória (nota de C++)

`load()`/`store()` sem argumentos usam `std::memory_order_seq_cst`, a ordem mais forte. Para um valor
isolado como este podia usar-se `memory_order_relaxed`, mas a diferença é desprezável aqui e o padrão
é mais seguro. Fica a saber que existe.

## O exercício

O `.h` está completo. Em `Processor.cpp`: TODO(1) `prepare`, TODO(2) `reset`, TODO(3)
`setGainDecibels`, TODO(4) `process`. Abaixo de −60 dB o ganho é 0 (silêncio).

O wrapper do standalone (`app/ExerciseApp.cpp`) usa um `AudioProcessorParameter::Listener` para
chamar `setGainDecibels` na thread em que o parâmetro mudou — vale a pena ler.

## Armadilhas comuns

- **Ler o atómico em cada amostra**: funciona, mas o valor pode mudar a meio do bloco e é trabalho
  desnecessário.
- **Chamar `SmoothedValue::setTargetValue` na thread da GUI**: data race (o teste com duas threads
  pode não apanhar — as data races nem sempre se manifestam).
- **`size_t` vs. `int`**: o `AudioBlock` usa `size_t`; misturar com `int` dá avisos (ou bugs com
  números negativos).
- **Bypass que "congela" o smoothing**: quando o bypass desliga, a rampa retoma de um valor velho.

## Como verificar

```powershell
./learn check 09
./learn run 09
```

## A versão JUCE

`juce::dsp::Gain<float>` (`setGainDecibels`, `setRampDurationSeconds`) — com a ressalva de
thread-safety acima.

## Para saber mais

- [Tutorial JUCE: Introduction to DSP](https://juce.com/tutorials/tutorial_dsp_introduction/) — `ProcessSpec`, `AudioBlock`,
  `ProcessContext`, `ProcessorChain`.
- [juce::dsp::ProcessContextReplacing](https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessContextReplacing.html) e
  [juce::dsp::AudioBlock](https://docs.juce.com/master/classjuce_1_1dsp_1_1AudioBlock.html) — documentação.
- [juce::dsp::Gain](https://docs.juce.com/master/classjuce_1_1dsp_1_1Gain.html) e
  [juce::dsp::ProcessorChain](https://docs.juce.com/master/classjuce_1_1dsp_1_1ProcessorChain.html) — documentação.
- [std::atomic — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic) — `load`, `store`, `is_always_lock_free` e as ordens de memória.
