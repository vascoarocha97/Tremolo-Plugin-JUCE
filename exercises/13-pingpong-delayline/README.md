# 13 · Ping-pong com `juce::dsp::DelayLine`

> **Módulo 3 — Delay** · ⏱ ~30 min · Visualização: [Delay com feedback](http://localhost:5173/#/feedback-delay)

## Objetivos

- Ligar duas delay lines em **feedback cruzado** para os ecos saltarem entre a esquerda e a direita.
- Seguir o percurso de um impulso por um grafo de delays (útil para FDNs no módulo 7).

## Teoria

Num ping-pong, a entrada (somada a mono) entra só na linha da **esquerda**. O que sai da esquerda
alimenta a **direita**; o que sai da direita volta à **esquerda**. Cada linha atrasa D:

```
           ┌──────────── fb ◄──────────────┐
entrada ──►(+)──► [ linha L: D ] ──┬──► saída L
                                   │
                                   └─► fb ──► [ linha R: D ] ──┬──► saída R
                                                               │
                                          (volta ao início) ◄──┘
```

Um impulso sai em L no instante D (amplitude 1), em R em 2D (fb), em L em 3D (fb²), em R em 4D (fb³)...
O ciclo completo L → R → L tem ganho fb², por isso a estabilidade continua a exigir fb < 1.

Uma `juce::dsp::DelayLine` com **2 canais** dá-te as duas linhas: canal 0 = esquerda, canal 1 =
direita. O `prepare` recebe sempre 2 canais, independentemente do bloco.

Variações comuns em produtos reais (não pedidas aqui): tempos diferentes em cada lado, um controlo de
largura, filtros no ciclo de feedback (ecos cada vez mais escuros, como numa fita).

## O exercício

Em `Processor.cpp`, **TODO** `process`: o encaminhamento cruzado. Desenha o percurso de um impulso
num papel antes de escrever código — os testes verificam os instantes D, 2D, 3D, 4D e as amplitudes.

## Armadilhas comuns

- **A entrada nas duas linhas**: os ecos deixam de alternar.
- **Ler uma linha depois de já ter escrito na outra** no mesmo instante: lê primeiro as duas, depois
  escreve as duas.
- **Somar L + R sem dividir por 2** para a entrada mono.

## Como verificar

```powershell
./learn check 13
./learn run 13     # com auscultadores
```

## Para saber mais

- [juce::dsp::DelayLine](https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html) — linhas multicanal.
- Julius O. Smith III, [PASP — "Delay Lines"](https://ccrma.stanford.edu/~jos/pasp/Delay_Lines.html) — redes de delay lines como sistemas lineares.
