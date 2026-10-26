# 39 · Glitch

> **Módulo 10 — Projeto final** · ⏱ ~60 min · Visualização: [O pedal](http://localhost:5173/#/pedal)

## Objetivos

- Implementar um *stutter* / *beat-repeat*: congelar os últimos milissegundos e repeti-los.
- Fazer **crossfades** em todos os sítios onde nascem cliques: na junção de cada repetição, ao ligar e
  ao desligar.
- Preparar uma peça para o pedal do exercício 40.

## Teoria

### A ideia

Um histórico circular grava sempre a entrada (como a delay line do exercício 10). Ao ligar, copia-se
a **fatia** — os últimos `slice` ms — e passa-se a tocá-la em ciclo:

```
entrada:   ... a b c d e f g h | (liga aqui)
fatia:               e f g h
saída:     ... a b c d e f g h e f g h e f g h e f g h ...
```

Copiar até ~1 s de áudio no início de um bloco é uma cópia de memória (sem alocar): rápido o
suficiente. A alternativa é congelar um índice no histórico, mas então o histórico não pode ser
reescrito enquanto a repetição dura.

### Clique 1: a junção

A fatia raramente começa e acaba no mesmo valor: a cada repetição há um **degrau** (`h` → `e`). A
solução usada aqui é guardar também um **pré-rolo**: as `F` amostras que tocaram imediatamente antes da
fatia (`… c d`). Nas últimas `F` amostras de cada repetição faz-se um crossfade do fim da fatia para o
pré-rolo. Como o pré-rolo desemboca naturalmente no início da fatia (`d` → `e` era contínuo no sinal
original), quando a posição volta a 0 não há degrau.

```
fatia:      [ pré-rolo (F) ][ e f g ...................... h ]
repetição:                   e f g ..... (crossfade fim ↔ pré-rolo) → e f g ...
```

### Lei do crossfade

Com uma lei **linear** (`(1 − w)·a + w·b`, ganhos que somam 1), sinais parecidos (correlacionados)
mantêm o nível. Sinais diferentes (não correlacionados — ex.: duas partes de uma música) perdem até
3 dB a meio. A lei de **potência constante** (`cos`/`sin`, ganhos cujos quadrados somam 1) mantém o nível
para sinais não correlacionados mas dá +3 dB para sinais iguais. Para junções curtas (poucos ms) a
linear é a escolha habitual; o teste "periódico" exige que o resultado seja exatamente o mesmo em cada
repetição — qualquer lei serve.

### Cliques 2 e 3: ligar e desligar

Passar do sinal seco para a repetição (e de volta) também é um degrau. Um `SmoothedValue` de 0 a 1
(duração `fade`) mistura os dois: `saída = (1 − g)·seco + g·repetição`. Ao desligar, rampa de volta a 0;
quando chega a 0, deixa de repetir.

## O exercício

Em `Processor.cpp`: **TODO(1)** `captureSlice`, **TODO(2)** `readLoop` (o crossfade da junção),
**TODO(3)** as transições ao ligar/desligar no `process`.

## Armadilhas comuns

- **Índices negativos** no histórico circular: `(writePos - total + i) % size` pode ser negativo em C++.
- **Capturar a fatia a meio do bloco** de forma inconsistente: aqui captura-se sempre no início do
  `process`, com o histórico até ao fim do bloco anterior.
- **Esquecer que o histórico continua a gravar** durante a repetição: ao desligar, o seco volta sem salto.
- **Rampa de desligar que recomeça** a cada bloco (um `setTargetValue` repetido): verifica o alvo atual.

## Como verificar

```powershell
./learn check 39
./learn run 39     # "Loop de demonstração", liga "Repetir"; põe o crossfade no mínimo para ouvir a diferença
```

## Para saber mais

- Geraint Luff (Signalsmith Audio), código de ["Four Ways To Write A Pitch-Shifter" (ADC22)](https://github.com/Signalsmith-Audio/pitch-time-example-code) — repetir e cruzar troços de áudio é a base de muitos efeitos de tempo/tom.
- Julius O. Smith III, ["Delay Lines" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Delay_Lines.html) — buffers circulares, a base do histórico.
- Ross Bencina, ["Real-time audio programming 101"](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) — porque é que a cópia é feita para um buffer já alocado.
- [juce::SmoothedValue — documentação](https://docs.juce.com/master/classjuce_1_1SmoothedValue.html) — as rampas de ligar/desligar.
