# 19 · Tremolo

> **Módulo 5 — Modulação** · ⏱ ~25 min · Visualização: [LFO e modulação](http://localhost:5173/#/lfo)

## Objetivos

- Modular a amplitude com o teu LFO (exercício 18).
- Controlar a **profundidade** e o desfasamento **estéreo**.
- Reutilizar código de outro exercício (o `#include "../18-lfo/Processor.h"`).

## Teoria

### Modulação de amplitude

```
ganho = 1 − profundidade · unipolar(LFO)        unipolar(v) = (v + 1) / 2
```

Com profundidade 1 o ganho vai de 1 a 0 (o som desaparece em cada ciclo); com 0.5, vai de 1 a 0.5.
A forma do LFO muda o carácter: seno = pulsação suave; quadrada = "chop" (com clicks, ver exercício 18);
triângulo = entre os dois.

Em frequência, multiplicar por um seno lento cria **bandas laterais** a ±f_LFO de cada componente do
sinal. A 5 Hz isso ouve-se como pulsação; acima de ~20–30 Hz começa a ouvir-se como um timbre novo
(*ring modulation*/AM a frequências de áudio).

### Estéreo

O LFO do canal direito tem um desvio de fase: com 180°, quando a esquerda está no máximo a direita
está no mínimo — `ganhoL + ganhoR = 2 − profundidade`, constante — e o som parece mover-se de um
lado para o outro (um auto-pan). É assim que o tremolo "Harmonic"/estéreo de muitos pedais funciona.

### Profundidade com smoothing

A profundidade vem de um parâmetro: muda-a de repente e o ganho salta (click). Por isso é um
`juce::SmoothedValue` que avança uma vez por amostra.

## O exercício

Em `Processor.cpp`: **TODO** `process`. O resto (os setters, o `prepare` que prepara os dois LFOs e o
smoothing) já está feito. O teu LFO do exercício 18 é usado aqui tal como o escreveste.

## Armadilhas comuns

- **`getNextSample()` do mesmo LFO para os dois canais**: o LFO avança ao dobro da velocidade e os
  canais ficam com valores diferentes sem querer.
- **Ganho `profundidade · LFO`** em vez de `1 − ...`: com profundidade 0 o som desaparece.
- **LFO bipolar** usado diretamente: o ganho fica negativo (inverte a fase do som).

## Como verificar

```powershell
./learn check 19
./learn run 19
```

## Para saber mais

- Julius O. Smith III, ["Sinusoidal Amplitude Modulation (AM)"](https://ccrma.stanford.edu/~jos/mdft/Sinusoidal_Amplitude_Modulation_AM.html) —
  as bandas laterais da modulação de amplitude.
- O curso oficial do JUCE que já tens neste repositório (`complete/`, `todo/`) é um tremolo completo como plugin —
  depois deste exercício vale a pena comparar com `complete/tremolo_plugin/`.
