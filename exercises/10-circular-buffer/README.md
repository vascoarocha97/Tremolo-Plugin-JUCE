# 10 · Buffer circular

> **Módulo 3 — Delay** · ⏱ ~30 min · Visualização: [Delay line circular](http://localhost:5173/#/delay-line)

## Objetivos

- Implementar uma **delay line** com um buffer circular de tamanho fixo.
- Atrasar um sinal **exatamente** N amostras, com blocos de qualquer tamanho.
- Ver a regra "alocar no `prepare`, nunca no `process`" aplicada a um caso real.

## Teoria

### O que é uma delay line

Um atraso de N amostras é `y[n] = x[n − N]`: a saída de agora é a entrada de há N amostras. Para
isso é preciso **guardar** as últimas N amostras (mais a atual). Em C# farias uma `Queue<float>`; mas
uma fila que cresce e encolhe aloca memória — proibido na thread de áudio.

### Buffer circular

A solução clássica: um array de tamanho fixo e um **índice de escrita** que avança uma posição por
amostra e, ao chegar ao fim, volta ao início (*wrap-around*). As amostras mais antigas são
sobrescritas pelas novas. Para ler a amostra de há *d* passos, recua-se *d* posições a partir do
índice de escrita, dando a volta para o fim se passar do início.

```
tamanho = N + 1            (a amostra atual e as N anteriores)
push(x):  w = (w + 1) mod tamanho;  buf[w] = x
read(d):  r = w − d;  se r < 0: r += tamanho;  devolve buf[r]
```

Detalhes:

- **`%` com números negativos** em C++ dá um resultado negativo (`-3 % 10 == -3`), não 7 como em
  matemática. Por isso o *wrap* da leitura faz-se com um `if` (ou somando o tamanho antes do `%`).
- Muitas implementações usam um tamanho **potência de 2** e uma máscara (`índice & (tamanho − 1)`)
  em vez de `%` ou `if` — é mais rápido. O `juce::dsp::DelayLine` usa `%`.
- **Uma linha por canal**: cada canal tem o seu próprio passado.
- Se o máximo for 2 s a 48 kHz, são 96 001 `float` por canal (~375 KB) — reservados no `prepare`.

### Onde isto aparece

Em quase tudo o que vem a seguir: eco (12), chorus e flanger (20, 21), filtros comb e allpass (27),
reverbs (28–31), pitch shift (31), compensação de latência num plugin real.

## O exercício

Em `Processor.cpp`:

1. **TODO(1)** `CircularDelayLine::prepare` — tamanho do buffer.
2. **TODO(2)** `push` e **TODO(3)** `read` (com o *wrap* e o limite do atraso).
3. **TODO(4)** `IntegerDelay::prepare` — uma linha por canal.
4. **TODO(5)** `IntegerDelay::process`.

Um dos testes corre 20 000 amostras de ruído com um buffer de 2001 posições (dá a volta ~10 vezes)
e verifica **amostra a amostra** que `saída[n] == entrada[n − 1234]`.

## Armadilhas comuns

- **Off-by-one no tamanho**: com tamanho N só cabem atrasos até N − 1.
- **Ler antes de escrever** (ou vice-versa) muda o atraso efetivo em 1 amostra. Decide uma ordem e
  mantém-na; aqui é *push* e depois *read*, por isso `read (0)` é a amostra atual.
- **`%` com o índice negativo**.
- **Alocar no `process`** (`std::vector` local, `resize`): o teste de alocações apanha.

## Como verificar

```powershell
./learn check 10
./learn run 10     # fonte "Impulsos": no osciloscópio vês o impulso e a cópia atrasada
```

## A versão JUCE

`juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>` faz isto, com uma
interface por canal (`pushSample`, `popSample`). Usas no exercício 12.

## Para saber mais

- Julius O. Smith III, [*Physical Audio Signal Processing* — "Delay Lines"](https://ccrma.stanford.edu/~jos/pasp/Delay_Lines.html) —
  a delay line como bloco fundamental e a sua implementação com buffer circular.
- [juce::dsp::DelayLine — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html) — a versão do JUCE.
