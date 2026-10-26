# 06 · Decibéis

> **Módulo 2 — Ganho, dB, pan e smoothing** · ⏱ ~15 min · Visualização: [Decibéis](http://localhost:5173/#/decibels)

## Objetivos

- Converter entre ganho linear e decibéis (e saber porque é que há 20 e há 10).
- Tratar o "−∞ dB" sem rebentar com `log(0)`.
- Fazer a conversão no sítio certo (não por amostra).

## Teoria

### Porquê decibéis

O ouvido percebe mudanças de nível de forma aproximadamente **relativa** (proporcional), e o áudio
cobre uma gama enorme — de 1.0 até 0.00001 é normal. Uma escala logarítmica torna isto manejável:
cada **fator** passa a ser uma **soma**.

### Amplitude vs. potência

Um decibel compara **potências**: `dB = 10 · log10(P / P_ref)`. Mas as amostras são **amplitudes**, e a
potência é proporcional ao quadrado da amplitude. Como `log10(a²) = 2 · log10(a)`:

```
dB = 20 · log10(ganho)            ganho = 10^(dB / 20)        (amplitudes: amostras, ganhos)
dB = 10 · log10(razão de potência)                            (potências: energia, RMS²)
```

Valores para decorar:

| ganho (amplitude) | dB | |
|---|---|---|
| 2 | +6.02 | dobro da amplitude |
| 1.414 (√2) | +3.01 | dobro da potência |
| 1 | 0 | igual |
| 0.5 | −6.02 | metade da amplitude |
| 0.1 | −20 | um décimo |
| 0.001 | −60 | "quase nada" |

**Somar dB = multiplicar ganhos**: −3 dB seguido de −9 dB é −12 dB.

Em plugins, **dBFS** é a escala em que 0 dB = 1.0 (o máximo do conversor). Um seno que chega a ±1.0
está a 0 dBFS de pico.

### −∞ dB

`log10(0)` é −∞. Na prática define-se um piso: o `juce::Decibels` usa **−100 dB** por omissão
(`decibelsToGain (dB, minusInfinityDb = -100)` devolve 0 a partir daí, e `gainToDecibels` nunca
devolve menos do que isso). Vais fazer o mesmo.

### Onde fazer a conversão

`std::pow` é caro comparado com uma multiplicação. A conversão dB → linear faz-se quando o parâmetro
muda (ou uma vez por bloco), e o ciclo das amostras só multiplica.

## O exercício

Em `Processor.cpp`: TODO(1)–(3) as três funções de conversão, TODO(4) `setGainDecibels`, TODO(5)
`processBlock`. Um dos testes compara as tuas funções com `juce::Decibels` em toda a gama.

## Armadilhas comuns

- **20 vs. 10**: 20 para amplitudes/ganhos, 10 para potências.
- **`log10(0)`** ou de um ganho negativo → −∞ ou NaN que se propagam.
- **`std::pow(10, dB/20)` dentro do ciclo das amostras**.
- **Divisão inteira**: `dB / 20` com `int` dá resultados errados (ex.: −6/20 = 0).

## Como verificar

```powershell
./learn check 06
./learn run 06
```

No standalone, mexe no ganho depressa: ouves um "click"/"zipper" porque o ganho muda de uma vez em
cada bloco. É exatamente o problema do exercício 08.

## A versão JUCE

`juce::Decibels::decibelsToGain()` / `gainToDecibels()`, e `juce::dsp::Gain<float>` (com smoothing,
exercício 09).

## Para saber mais

- Julius O. Smith III, [*Mathematics of the DFT* — "Decibels"](https://ccrma.stanford.edu/~jos/mdft/Decibels.html) —
  a definição, amplitude vs. potência, e as escalas dB SPL/dBFS.
- [juce::Decibels — documentação](https://docs.juce.com/master/classjuce_1_1Decibels.html) — as funções do JUCE e o piso de −100 dB.
