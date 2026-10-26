# 18 · LFO

> **Módulo 5 — Modulação** · ⏱ ~30 min · Visualização: [LFO e modulação](http://localhost:5173/#/lfo)

## Objetivos

- Escrever um **LFO** (*low-frequency oscillator*) com quatro formas de onda.
- Controlar a **fase** (desvio entre canais) e garantir continuidade quando a velocidade muda.
- Ter uma classe que vais **reutilizar** nos exercícios 19, 20 e 21.

## Teoria

Um LFO é um oscilador como o do exercício 02, mas lento (0.05–20 Hz) e usado para **mexer noutros
parâmetros**: volume (tremolo), atraso (vibrato, chorus, flanger), pan (auto-pan), corte de um filtro
(auto-wah). Não se ouve diretamente — ouve-se o efeito.

### Fase em ciclos

Aqui a fase vai de 0 a 1 (ciclos) em vez de 0 a 2π: torna as formas não-sinusoidais simples de
escrever. Incremento por amostra = `f / fs`.

### As formas (todas com a mesma "fase" que o seno)

| forma | 0 | ¼ | ½ | ¾ |
|---|---|---|---|---|
| seno `sin(2π·p)` | 0 | 1 | 0 | −1 |
| triângulo | 0 | 1 | 0 | −1 (em linhas retas) |
| dente de serra | 0 | 0.5 | salta de 1 para −1 | −0.5 |
| quadrada | 1 | 1 | −1 | −1 |

Alinhar as formas pela mesma fase faz com que trocar de forma não desloque o ciclo no tempo.

### Desvio de fase e continuidade

- O **desvio** (`setPhaseOffset`) soma-se à fase só na leitura: dois LFOs com a mesma frequência e
  desvios diferentes ficam permanentemente desfasados (é assim que se faz o estéreo nos próximos
  exercícios).
- Mudar a **frequência** muda só o incremento: a fase continua de onde estava, sem saltos.
- `valueAt` tem de aceitar qualquer fase real (o desvio pode levá-la acima de 1): reduz com
  `p − floor(p)`.

### Aliasing de LFOs?

As formas com descontinuidades (dente de serra, quadrada) têm harmónicos infinitos — mas a 2 Hz os
harmónicos relevantes estão muito abaixo de Nyquist, e o LFO não se ouve diretamente. Na prática, a
descontinuidade de uma quadrada a mexer num ganho **ouve-se como um click**; produtos reais
suavizam-na (um filtro de um polo, exercício 14, sobre o valor do LFO).

## O exercício

Em `Processor.cpp`: **TODO(1)** o incremento, **TODO(2)** `valueAt` (as quatro formas),
**TODO(3)** `getNextSample`.

> Os exercícios 19, 20 e 21 incluem `../18-lfo/Processor.h` e usam **esta** classe: se o 18 não
> passar, esses também não vão passar.

## Armadilhas comuns

- **`std::fmod` com valores negativos** devolve negativos; `p − std::floor (p)` não.
- **Somar o desvio à fase guardada** (em vez de só na leitura): depois de `reset()` o desvio perde-se.
- **Incremento em radianos** quando a fase está em ciclos.

## Como verificar

```powershell
./learn check 18
./learn run 18     # o teu LFO a mexer o pan (auto-pan) através do juce::dsp::Panner
```

## A versão JUCE

`juce::dsp::Oscillator<float>` serve de LFO (inicializa-se com uma função e `processSample (0)` dá o
valor), mas não tem desvio de fase configurável: para estéreo desfasado é comum escrever um LFO
próprio, como este.

## Para saber mais

- Julius O. Smith III, [*Mathematics of the DFT* — "Sinusoidal Amplitude Modulation (AM)"](https://ccrma.stanford.edu/~jos/mdft/Sinusoidal_Amplitude_Modulation_AM.html) —
  o que acontece ao espectro quando um LFO modula a amplitude (base do exercício 19).
- [juce::dsp::Oscillator — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oscillator.html).
