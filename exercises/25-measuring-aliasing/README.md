# 25 · Medir aliasing com FFT

> **Módulo 6 — Distorção** · ⏱ ~45 min · Visualização: [FFT e janelas](http://localhost:5173/#/fft)

## Objetivos

- Usar a FFT do JUCE (`juce::dsp::FFT`) com uma janela (`juce::dsp::WindowingFunction`).
- Pôr o espectro numa escala útil (0 dB = seno de amplitude 1).
- Construir uma **medida de aliasing** para comparar distorções e provar que o oversampling funciona.

## Teoria

### A FFT

A DFT decompõe N amostras em N/2 + 1 frequências igualmente espaçadas: o **bin** k está em `k·fs/N`.
A FFT é um algoritmo rápido (N·log N) para a calcular; N tem de ser potência de 2 (`order` = log₂ N).
Com N = 16384 a 48 kHz, cada bin tem 2.93 Hz.

### Janelas

A DFT assume que o bloco se repete para sempre. Se o sinal não tiver um número inteiro de períodos no
bloco, os extremos não colam e a energia "escorre" para todos os bins (*spectral leakage*). Multiplicar
por uma **janela** que vai a zero nos extremos (Hann, Hamming, Blackman...) reduz muito esse
escorrimento, à custa de alargar um pouco cada risca. Hann é o compromisso habitual.

### A FFT do JUCE

- `juce::dsp::FFT fft (order)`; o array de trabalho tem de ter **2·N** floats (a FFT trabalha no próprio
  array).
- `performFrequencyOnlyForwardTransform (dados, true)` devolve as magnitudes `|X[k]|` nos primeiros
  N/2 + 1 valores.
- `juce::dsp::WindowingFunction<float> (N, hann, normalise = true)`: com `normalise = true` (lendo
  `juce_Windowing.cpp`) a janela é escalada para que a sua soma seja N — "o ganho DC é 1". Então um seno
  de amplitude A cujo período cabe um número inteiro de vezes na janela dá `|X| = A·N/2` no seu bin.
  Dividir por N/2 põe a escala em "amplitude" (0 dB = amplitude 1).

### Uma medida de aliasing

Um sinal periódico de fundamental f0, depois de uma distorção **sem** aliasing, só tem energia em
`k·f0`. Tudo o resto é aliasing (ou ruído). Então:

```
aliasing (dB) = 10·log10( energia fora dos harmónicos / energia nos harmónicos )
```

com "nos harmónicos" = os bins a ±halfWidth de cada `k·f0` (por causa da largura da janela). Energia é a
soma de `|X|²`, ou seja, de `10^(dB/10)`. Os testes usam frequências que caem exatamente num bin, para
que não haja escorrimento e o que sobra seja mesmo aliasing.

Resultados típicos (os testes verificam): um seno puro < −90 dB; uma onda quadrada com banda limitada
< −80 dB; um hard clip de um seno de 5 kHz a 48 kHz > −40 dB; e o tanh tem menos aliasing do que o hard clip.

### Isto não é para a thread de áudio

Estas funções alocam (`std::vector`, `FFT`, `WindowingFunction`). São ferramentas de análise: no
standalone, o editor corre-as na thread da GUI a partir de amostras recolhidas por uma FIFO lock-free
(o padrão do exercício 05).

## O exercício

Em `Processor.cpp`: **TODO(1)** `magnitudeDb`, **TODO(2)** `binFrequency`, **TODO(3)** `aliasingRatioDb`.

## Armadilhas comuns

- **Array de N floats** em vez de 2·N: a FFT escreve fora.
- **Escala**: sem janela normalizada o fator é N/4 (Hann tem ganho coerente 0.5); com janela normalizada é N/2.
- **Somar dB** em vez de potências.
- **Contar o bin 0 (DC)** como aliasing.

## Como verificar

```powershell
./learn check 25
./learn run 25     # "Seno": o editor mostra f0 e o aliasing medido pelas tuas funções; sobe o drive e a frequência
```

## Para saber mais

- Julius O. Smith III, [*Mathematics of the DFT* — "Fast Fourier Transform (FFT)"](https://ccrma.stanford.edu/~jos/mdft/Fast_Fourier_Transform_FFT.html) — a DFT e a FFT.
- Julius O. Smith III, [*Spectral Audio Signal Processing* — "Spectrum Analysis Windows"](https://ccrma.stanford.edu/~jos/sasp/Spectrum_Analysis_Windows.html) — janelas e escorrimento.
- [juce::dsp::FFT](https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html) e
  [juce::dsp::WindowingFunction](https://docs.juce.com/master/classjuce_1_1dsp_1_1WindowingFunction.html) — documentação.
