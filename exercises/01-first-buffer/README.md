# 01 · O primeiro buffer

> **Módulo 1 — Fundamentos** · ⏱ ~5 min · Visualização: [Buffers e blocos](http://localhost:5173/#/buffers)
> (corre `./learn web` primeiro)

## Objetivos

- Perceber o que é uma **amostra** e como o JUCE guarda áudio num `juce::AudioBuffer<float>`.
- Percorrer um buffer à mão: **canais × amostras**.
- Perceber porque é que o código de áudio recebe o som em **blocos** de tamanho variável.

## Teoria

### Amostras

O som digital é uma sequência de números: o valor da pressão do ar (ou da tensão elétrica no
microfone) medido muitas vezes por segundo. Cada número é uma **amostra** (*sample*). Quantas
amostras por segundo é o **sample rate** (44 100 Hz, 48 000 Hz, ...) — vamos a isso no exercício 02.

No JUCE, as amostras são `float` e a escala nominal vai de **−1.0 a +1.0**: 1.0 é o máximo que o
conversor digital-analógico consegue reproduzir (**0 dBFS**, "decibéis relativos ao *full scale*").
Dentro do teu código podes passar de 1.0 sem problema (é um `float`), mas o que chegar à saída
acima de 1.0 é cortado (*clipping*).

Multiplicar todas as amostras por 0.5 baixa o nível para metade da amplitude, o que corresponde a
20·log₁₀(0.5) ≈ **−6.02 dB**. Os decibéis ficam para o exercício 06.

### O `AudioBuffer`

Um `juce::AudioBuffer<float>` é uma tabela de **canais × amostras**. Cada canal é um array contíguo
de `float` (o JUCE guarda os canais separados — *non-interleaved* — e não intercalados L R L R).

| método | o que faz |
|---|---|
| `getNumChannels()` | número de canais (1 = mono, 2 = estéreo, ...) |
| `getNumSamples()` | número de amostras **deste** bloco |
| `getReadPointer (ch)` | `const float*` para ler o canal `ch` |
| `getWritePointer (ch)` | `float*` para escrever no canal `ch` |
| `getSample (ch, n)` / `setSample (ch, n, v)` | acesso a uma amostra (com verificação de limites em Debug) |

Um detalhe real do JUCE: o `AudioBuffer` tem uma *flag* interna que diz "estou todo a zeros", usada
para saltar trabalho. `getWritePointer()` desliga essa flag; `getReadPointer()` não. Por isso, se só
vais **ler**, usa `getReadPointer()` (a documentação do próprio JUCE pede isto).

### Blocos e o callback de áudio

A placa de som não pede uma amostra de cada vez: pede um **bloco** de, por exemplo, 480 amostras
(10 ms a 48 kHz). O sistema operativo chama o teu código numa thread de alta prioridade — a
**thread de áudio** — e tu tens de devolver o bloco processado antes de o anterior acabar de tocar.

Num plugin JUCE isto corresponde a dois métodos do `juce::AudioProcessor`:

- `prepareToPlay (sampleRate, maximumExpectedSamplesPerBlock)` — chamado antes de o áudio começar.
  É aqui que se aloca memória e se calcula o que depende do sample rate.
- `processBlock (buffer, midi)` — chamado na thread de áudio para cada bloco.

O processador deste exercício tem a mesma forma (sem a parte de MIDI). O que a documentação do
`AudioProcessor::processBlock` diz e que muita gente ignora:

- o número de amostras **não é garantido** ser igual em todos os callbacks, e pode ser **maior ou
  menor** do que o valor dado no `prepareToPlay()` (um host com bugs pode exceder o máximo);
- alguns hosts chamam com **0 amostras**;
- o buffer pode ter **mais canais** do que os que o teu plugin usa.

Conclusão: usa sempre `buffer.getNumSamples()` e `buffer.getNumChannels()`, nunca números fixos.

## O exercício

Abre `Processor.cpp` e completa:

1. **TODO(1)** — `processBlock()`: multiplica cada amostra de cada canal por `gain` (0.5).
2. **TODO(2)** — `findPeak()`: devolve o maior |x| de todas as amostras de todos os canais.

Escreve os ciclos à mão (dois `for` encaixados). Depois de passar os testes, vê a secção
"A versão JUCE" mais abaixo.

## Armadilhas comuns

- **Tamanho fixo**: `for (int n = 0; n < 512; ++n)` funciona no teu PC e rebenta noutro host.
  O teste "funciona com blocos de qualquer tamanho" processa blocos de 1, 7, 13, 333... amostras.
- **Assumir estéreo**: `getWritePointer (1)` num buffer mono escreve fora da memória.
- **Processar duas vezes**: se o mesmo bloco for multiplicado duas vezes, o ganho fica 0.25.
- **`getWritePointer` só para ler**: funciona, mas desliga a otimização de "buffer limpo".

## Como verificar

```powershell
./learn check 01     # compila e corre os testes
./learn run 01       # abre o standalone: liga o "Bypass (A/B)" e ouve a diferença de 6 dB
```

No standalone, o espectro mostra a entrada (cinzento) e a saída (azul): a curva azul fica ~6 dB
abaixo da cinzenta em todas as frequências — um ganho não muda a forma do espectro.

## A versão JUCE

Depois de passares os testes: o `AudioBuffer` já tem `applyGain (0.5f)` e
`getMagnitude (startSample, numSamples)`, e o `juce::FloatVectorOperations::multiply` faz o mesmo
ciclo com instruções SIMD (módulo 9). No código real usam-se estes; escrever os ciclos uma vez serve
para perceber o que eles fazem.

## Para saber mais

- [juce::AudioBuffer — documentação da API](https://docs.juce.com/master/classjuce_1_1AudioBuffer.html) —
  todos os métodos e a explicação da flag "isClear".
- [juce::AudioProcessor — documentação da API](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html) —
  as garantias (e a falta delas) de `prepareToPlay` e `processBlock`.
- [Tutorial JUCE: Create a basic Audio/MIDI plugin, Part 1](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/) —
  onde vivem `prepareToPlay` e `processBlock` num plugin (usa o Projucer, mas os conceitos são os mesmos).
- Julius O. Smith III, [*Mathematics of the DFT* — "Sampling Theory"](https://ccrma.stanford.edu/~jos/mdft/Sampling_Theory.html) —
  o que significa representar um sinal por amostras (base para o exercício 02/03).
