# 22 · Waveshaper

> **Módulo 6 — Distorção** · ⏱ ~30 min · Visualização: [Waveshaping e harmónicos](http://localhost:5173/#/waveshaping)

## Objetivos

- Implementar quatro curvas de distorção "sem memória": hard clip, soft clip cúbico, tanh e assimétrica.
- Relacionar a **forma da curva** com os **harmónicos** que aparecem.
- Ter funções que o fuzz (23) e o oversampling (26) vão reutilizar.

## Teoria

### Waveshaping

Um waveshaper aplica uma função a cada amostra: `y = f(ganho · x)`. Não tem estado (é *memoryless*):
a saída de agora só depende da entrada de agora. Com sinais pequenos, uma curva que passa pela origem
com inclinação ~1 é quase linear; com **drive** alto, o sinal é empurrado para a zona curva e é
"esmagado" — aparecem harmónicos.

| curva | fórmula | carácter |
|---|---|---|
| hard clip | `clamp(x, −1, 1)` | áspero, muitos harmónicos altos (o "canto" é uma descontinuidade da derivada) |
| soft clip cúbico | `1.5·(x − x³/3)` em [−1, 1], ±1 fora | mais suave; a derivada é contínua em ±1 |
| tanh | `tanh(x)` | suave, satura assintoticamente; clássico para "saturação de fita/válvula" |
| assimétrica | `tanh(x)` para x ≥ 0, `0.5·tanh(2x)` para x < 0 | lado negativo satura mais cedo |

### Simetria e harmónicos

- Uma curva **ímpar** (`f(−x) = −f(x)`) aplicada a um seno só cria harmónicos **ímpares** (3.º, 5.º...).
  Soa mais "dura"/"oca" (o som de uma onda quadrada é feito de harmónicos ímpares).
- Uma curva **assimétrica** cria também harmónicos **pares** (2.º, 4.º...) — muitas vezes descritos como
  mais "quentes" — **e um desvio DC**: aqui o lado positivo chega a +1 e o negativo só a −0.5, por isso
  a média fica positiva. O DC não se ouve, mas come headroom e cria clicks ao ligar/desligar o efeito:
  por isso o fuzz (exercício 23) tem um **DC blocker**.

Os testes medem isto com um seno de 1 kHz: com tanh o 2.º harmónico fica abaixo de −80 dB; com a curva
assimétrica aparece acima de −30 dB.

### O problema que vem a seguir: aliasing

Os harmónicos não param em 20 kHz: um hard clip de um seno de 5 kHz tem harmónicos a 15, 25, 35, 45 kHz...
A 48 kHz, os que passam de 24 kHz **dobram para baixo** (exercício 03) para frequências que não são
harmónicas. É o "som digital sujo" das distorções baratas. Os exercícios 25 e 26 medem e resolvem isto.

## O exercício

Em `Processor.cpp`: **TODO(1)–(4)** as quatro curvas e **TODO(5)** `Waveshaper::process` (drive e
saída suavizados; `Shapers::apply` escolhe a curva).

## Armadilhas comuns

- **Soft clip sem os limites**: `x − x³/3` volta para baixo acima de 1 (é um polinómio!).
- **Esquecer o fator 1.5**: o soft clip cúbico sem escala satura em ±2/3.
- **Ganhos em dB aplicados como lineares**.
- **Drive suavizado multiplicativo a partir de 0**: o `SmoothedValue` multiplicativo não pode chegar a 0
  (por isso os ganhos aqui nunca são 0).

## Como verificar

```powershell
./learn check 22
./learn run 22     # "Seno" a 200 Hz e vê os harmónicos de cada curva no espectro
```

## A versão JUCE

`juce::dsp::WaveShaper<float>` aplica uma função qualquer (`functionToUse`) a um bloco. Útil, mas a
parte interessante — que curva — continua a ser tua.

## Para saber mais

- Julius O. Smith III, [PASP — "Cubic Soft Clipper"](https://ccrma.stanford.edu/~jos/pasp/Cubic_Soft_Clipper.html) — a curva cúbica e porque é suave.
- J. Parker, V. Zavalishin, E. Le Bivic, ["Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution" (DAFx 2016)](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf) —
  o aliasing das curvas de distorção e uma técnica alternativa ao oversampling (antiderivative antialiasing).
- [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) — o módulo `chowdsp_waveshapers` tem clippers com ADAA usados em plugins reais
  (**GPLv3**: bom para ler e aprender, não para copiar para um produto fechado).
- [juce::dsp::WaveShaper — documentação](https://docs.juce.com/master/structjuce_1_1dsp_1_1WaveShaper.html).
