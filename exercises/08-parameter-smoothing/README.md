# 08 · Parameter smoothing

> **Módulo 2 — Ganho, dB, pan e smoothing** · ⏱ ~30 min · Visualização: [Smoothing e clicks](http://localhost:5173/#/smoothing)

## Objetivos

- Perceber **porque é que** mudar um parâmetro de repente faz um click.
- Escrever um smoother linear com o mesmo comportamento do `juce::SmoothedValue`.
- Aplicá-lo a um ganho, avançando uma vez por amostra (e não por canal).

## Teoria

### De onde vem o click

Um click é uma **descontinuidade**: a forma de onda salta de um valor para outro entre duas amostras.
Um salto instantâneo tem energia em todas as frequências (pensa no espectro de um impulso ou de um
degrau), e o ouvido ouve-o como um estalo, por muito pequeno que seja o salto.

Os parâmetros chegam da GUI ou da automação do host **entre blocos**. Se o ganho passar de 1.0 para
0.0 no início de um bloco, a amostra anterior estava a 1·x e a seguinte a 0·x: um degrau. Se o
utilizador arrastar um slider, o ganho muda em pequenos degraus bloco a bloco — o "zipper noise"
(som de fecho-éclair).

### Rampa linear

A solução é interpolar: quando o alvo muda, o valor vai do atual até ao alvo em **N amostras**
(tipicamente 10–50 ms), um passo igual em cada amostra:

```
passo = (alvo − atual) / N
a cada amostra: atual += passo      (na última amostra: atual = alvo, exatamente)
```

Detalhes que o `juce::SmoothedValue` trata e tu vais tratar:

- se o alvo muda **a meio** de uma rampa, começa uma rampa nova **a partir do valor atual** (não do
  alvo antigo) com N amostras completas;
- na última amostra atribui-se o alvo diretamente (somar `passo` N vezes em `float` acumula erro);
- `skip(n)` avança *n* amostras de uma vez (útil quando o processamento está em bypass);
- `reset (sampleRate, segundos)` define N e **salta** para o alvo — chama-se no `prepareToPlay`.

### Uma vez por amostra, não por canal

O smoother representa o **tempo**. Num bloco estéreo, a amostra *n* do canal esquerdo e a do direito
são o mesmo instante: têm de receber o mesmo ganho. Por isso o ciclo de fora é o das amostras e o de
dentro o dos canais — ou então calcula-se a rampa uma vez para um array e aplica-se a cada canal.

### Outras formas de suavizar

- **Multiplicativa / exponencial** (`ValueSmoothingTypes::Multiplicative`): rampa linear em dB, soa
  mais natural em fades longos. Não pode chegar a 0 (a razão seria infinita).
- **Filtro de um polo** sobre o valor do parâmetro: aproxima-se do alvo assintoticamente. Muito usado
  na indústria; vais escrever um filtro de um polo no exercício 14.

## O exercício

Em `Processor.cpp`: TODO(1) `setTargetValue`, TODO(2) `getNextValue`, TODO(3) `skip`,
TODO(4) `SmoothedGainProcessor::processBlock`. Um dos testes compara a tua sequência de valores,
amostra a amostra, com a do `juce::SmoothedValue<float, Linear>`.

## Armadilhas comuns

- **Recomeçar a rampa do alvo antigo** em vez do valor atual → salto.
- **`getNextValue()` dentro do ciclo dos canais** → a rampa estéreo dura metade e L ≠ R.
- **Esquecer o `reset()` no `prepareToPlay`** → N = 0 e não há smoothing nenhum.
- **Comparar `float` com `==` para saber se chegou ao alvo** depois de somas sucessivas: por isso se
  conta com um inteiro (`countdown`).

## Como verificar

```powershell
./learn check 08
./learn run 08     # fonte "Seno" a 200 Hz; alterna o ganho entre 0 e 1 com e sem "Smoothing"
```

## Para saber mais

- [juce::SmoothedValue — documentação](https://docs.juce.com/master/classjuce_1_1SmoothedValue.html) — os dois tipos de smoothing
  e os métodos `reset`, `setTargetValue`, `getNextValue`, `skip`.
- [Tutorial JUCE: Control audio levels](https://juce.com/tutorials/tutorial_synth_level_control/) — mostra o problema
  (ganho aplicado por bloco pode estalar) num exemplo simples.
- [Tutorial JUCE: Build a sine wave synthesiser](https://juce.com/tutorials/tutorial_sine_synth/) — como evitar
  artefactos quando um parâmetro muda.
