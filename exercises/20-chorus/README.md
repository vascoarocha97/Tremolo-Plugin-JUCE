# 20 · Chorus

> **Módulo 5 — Modulação** · ⏱ ~40 min · Visualização: [Chorus e flanger](http://localhost:5173/#/chorus-flanger)

## Objetivos

- Construir um chorus: um **delay modulado** (exercício 11) por um **LFO** (exercício 18).
- Perceber a relação entre profundidade/velocidade do LFO e a desafinação.

## Teoria

Um chorus mistura o sinal com uma cópia atrasada **5–30 ms** cujo atraso oscila devagar (0.1–3 Hz).

- Atraso de 5–30 ms: curto demais para se ouvir como eco, longo demais para criar um filtro pente
  agressivo (isso é o flanger, exercício 21).
- A variação do atraso **desafina** a cópia (Doppler, ver exercício 11): quando o atraso cresce a
  cópia fica mais grave, quando diminui mais aguda. A desafinação máxima é ≈ `2π · profundidade · f_LFO`
  (2 ms a 1 Hz ≈ 1.3%, ~20 cents).
- Original + cópia ligeiramente desafinada e atrasada = a sensação de "várias vozes" (como um coro,
  onde ninguém canta exatamente afinado nem ao mesmo tempo).

```
atraso(n) = centro + profundidade · LFO(n)          (em ms, convertido para amostras)
cópia     = delayLine.popSample (canal, atraso)
delayLine.pushSample (canal, entrada)
saída     = (1 − mistura) · entrada + mistura · cópia
```

Em estéreo, os LFOs de L e R estão desfasados **90°**: a desafinação de cada lado é diferente em cada
instante, o que alarga a imagem estéreo.

O atraso é lido com interpolação linear (`DelayLineInterpolationTypes::Linear`): precisa de valores
fracionários, e muda em cada amostra. Lembra-te do custo (exercício 11): um pouco de perda de agudos
que varia com a fração.

## O exercício

Em `Processor.cpp`: **TODO** `process`. O `prepare` (delay line de 50 ms, LFOs desfasados 90°,
smoothing) e os setters já estão feitos.

## Armadilhas comuns

- **Atraso de 0 amostras** com leitura antes da escrita (ver exercício 12): por isso o mínimo é 1.
- **Escrever na linha antes de ler**: o atraso efetivo fica uma amostra mais curto (inofensivo aqui,
  mas os testes medem o atraso).
- **Profundidade maior do que o centro**: o atraso ficaria negativo — o `setCentreDelayMs` e o
  `jmax (1, …)` protegem.

## Como verificar

```powershell
./learn check 20
./learn run 20     # experimenta o loop e a fonte "Dente de serra"
```

## A versão JUCE

`juce::dsp::Chorus<float>` (com `setRate`, `setDepth`, `setCentreDelay`, `setFeedback`, `setMix`) é um
chorus pronto; o código está em `modules/juce_dsp/widgets/juce_Chorus.cpp` — vale a pena comparar
depois de fazeres o teu.

## Para saber mais

- Julius O. Smith III, [PASP — "Chorus Effect"](https://ccrma.stanford.edu/~jos/pasp/Chorus_Effect.html) — o chorus como delay modulado.
- Julius O. Smith III, [PASP — "Vibrato Simulation"](https://ccrma.stanford.edu/~jos/pasp/Vibrato_Simulation.html) — o vibrato por delay modulado (o mesmo mecanismo, só com a cópia).
- [juce::dsp::Chorus — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Chorus.html).
