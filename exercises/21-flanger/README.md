# 21 · Flanger

> **Módulo 5 — Modulação** · ⏱ ~40 min · Visualização: [Chorus e flanger](http://localhost:5173/#/chorus-flanger)

## Objetivos

- Perceber o **filtro pente** que aparece quando se soma um sinal com uma cópia muito pouco atrasada.
- Construir um flanger: atraso curto varrido por um LFO, com **feedback** positivo ou negativo.

## Teoria

### O pente

`y = 0.5·x[n] + 0.5·x[n − D]`. Para uma frequência cujo meio período é D, a cópia chega em oposição
de fase e cancela o original: há um **vale** em

```
f = (2k + 1) / (2D)        (k = 0, 1, 2, ...)          com D = 1 ms: 500 Hz, 1500 Hz, 2500 Hz, ...
```

e picos em `k / D` (1 kHz, 2 kHz, ...). O primeiro teste mede exatamente isto. Com D de 0.1–10 ms, os
vales estão bem dentro da gama audível. Quando um LFO varre D, os vales deslizam — o som de "jato" de
avião que dá nome ao efeito (vem da técnica de travar à mão a *flange* de uma bobine de fita).

### Feedback

Como no eco (exercício 12), parte da cópia volta à delay line:

```
cópia = linha.ler (D)
linha.escrever (entrada + fb · cópia)
saída = (1 − mistura)·entrada + mistura·cópia
```

A função de transferência fica `H = (1 − m) + m·z⁻ᴰ / (1 − fb·z⁻ᴰ)`. Com fb > 0 os picos ficam mais
altos e estreitos (som mais "metálico"); com fb < 0 os picos passam para onde estavam os vales
(`(2k+1)/(2D)`) — os flangers chamam a isto "inverter". |fb| < 1 para estabilidade.

### Flanger vs. chorus vs. phaser

| efeito | atraso | modulação | resultado |
|---|---|---|---|
| flanger | 0.1–10 ms | LFO lento, com feedback | pente harmónico a varrer |
| chorus | 5–30 ms | LFO lento, sem/pouco feedback | "várias vozes", desafinação |
| phaser | (sem delay) cadeia de allpass | LFO lento | poucos vales, não harmónicos |

O JUCE tem `juce::dsp::Phaser` e `juce::dsp::Chorus` (que com atrasos curtos e feedback faz de flanger).

## O exercício

Em `Processor.cpp`: **TODO** `process`. É o chorus do exercício 20 com dois detalhes: o atraso varre
entre um mínimo e um máximo (forma triangular, já configurada no `prepare`) e há feedback.

## Armadilhas comuns

- **Feedback aplicado à saída misturada** (com o seco): muda o filtro e pode desestabilizar.
- **Atraso mínimo a 0** com leitura antes da escrita.
- **Esperar ouvir vales com mistura 1**: só com original **e** cópia é que há cancelamento (mistura
  0.5 dá os vales mais fundos).

## Como verificar

```powershell
./learn check 21
./learn run 21     # "Ruído branco": os vales a varrer veem-se no espectro
```

## Para saber mais

- Julius O. Smith III, [PASP — "Flanging"](https://ccrma.stanford.edu/~jos/pasp/Flanging.html) — o flanger como pente variável.
- Julius O. Smith III, [PASP — "Feedforward Comb Filters"](https://ccrma.stanford.edu/~jos/pasp/Feedforward_Comb_Filters.html) e
  ["Feedback Comb Filters"](https://ccrma.stanford.edu/~jos/pasp/Feedback_Comb_Filters.html) — as duas metades deste efeito.
- [juce::dsp::Phaser — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Phaser.html) — o efeito irmão, sem delay.
