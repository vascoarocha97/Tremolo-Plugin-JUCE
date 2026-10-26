# 12 · Eco com feedback

> **Módulo 3 — Delay** · ⏱ ~30 min · Visualização: [Delay com feedback](http://localhost:5173/#/feedback-delay)

## Objetivos

- Construir o eco clássico: delay + **feedback** + **dry/wet**.
- Perceber porque é que o feedback tem de ser < 1 (estabilidade).
- Usar a `juce::dsp::DelayLine` (agora que sabes o que tem por dentro).

## Teoria

### Feedback

Um delay simples dá um único eco. Para ter ecos repetidos, parte da saída da delay line volta a
entrar nela:

```
eco      = linha.ler (D)
linha.escrever (entrada + fb · eco)
saída    = (1 − mix) · entrada + mix · eco
```

Um impulso produz ecos em D, 2D, 3D, ... com amplitudes 1, fb, fb², fb³ — uma progressão geométrica.
Isto é um **filtro pente com feedback** (*feedback comb filter*): vais reencontrá-lo nas reverbs
(exercício 27).

### Estabilidade

Se `|fb| ≥ 1`, cada volta tem a mesma energia ou mais: os ecos nunca acabam ou crescem até ao
infinito (e um `float` a infinito vira NaN na primeira operação seguinte). Com fb < 1, o pior caso
para um sinal constante é um ganho de `1 / (1 − fb)` — 20 (+26 dB) com fb = 0.95. É por isso que
o feedback é **limitado** (aqui a 0.95) e que muitos delays de hardware têm um limitador ou
saturação dentro do ciclo.

### A ordem ler → escrever

Lê-se o eco **antes** de escrever a amostra nova. Com a `juce::dsp::DelayLine`, `popSample` antes de
`pushSample` dá um atraso de exatamente D amostras desde que D ≥ 1 (com D = 0 lerias uma posição
ainda por escrever) — por isso `delayMsToSamples` limita a 1 amostra. Lendo `juce_DelayLine.cpp`: o
`pushSample` escreve na posição de escrita e recua-a; o `popSample (canal, atraso)` chama `setDelay`
e lê em `posiçãoDeLeitura + atraso`, recuando depois a posição de leitura.

### Mix

`(1 − m)·seco + m·molhado` é um crossfade **linear**: a meio, se os dois sinais forem diferentes
(não correlacionados), a potência baixa ~3 dB. O `juce::dsp::DryWetMixer` oferece outras curvas
(incluindo potência constante) e ainda compensa a latência do caminho molhado.

### Smoothing de tudo

Atraso, feedback e mix mudam com `juce::SmoothedValue`. O atraso desliza em 100 ms: ao mudá-lo
ouve-se o "pitch bend" típico dos delays de fita.

## O exercício

Em `Processor.cpp`:

1. **TODO(1)** `prepare` — `setMaximumDelayInSamples` e `prepare` da `DelayLine`.
2. **TODO(2)** `setFeedback` — o limite.
3. **TODO(3)** `process` — o ciclo ler → escrever → misturar.

## Armadilhas comuns

- **Escrever antes de ler** com D pequeno: o atraso fica uma amostra mais curto.
- **Feedback aplicado à saída misturada** (com o dry lá dentro): o dry entra no ciclo.
- **Esquecer que `setMaximumDelayInSamples` aloca**: chamá-lo no `process` é proibido.
- **Um único `SmoothedValue` avançado por canal**.

## Como verificar

```powershell
./learn check 12
./learn run 12     # "Impulsos" para ver a progressão geométrica; o loop para ouvir
```

## Para saber mais

- Julius O. Smith III, [PASP — "Feedback Comb Filters"](https://ccrma.stanford.edu/~jos/pasp/Feedback_Comb_Filters.html) —
  o eco com feedback como filtro, a sua estabilidade e a resposta em frequência.
- [juce::dsp::DelayLine](https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html) — `pushSample`, `popSample`, `setMaximumDelayInSamples`.
- [juce::dsp::DryWetMixer](https://docs.juce.com/master/classjuce_1_1dsp_1_1DryWetMixer.html) — mistura seco/molhado com várias leis e compensação de latência.
- O exemplo oficial `examples/Plugins/DSPModulePluginDemo.h` (no repositório do JUCE, em
  `build/_deps/juce-src/examples/`) usa `juce::dsp::DelayLine` num plugin completo.
