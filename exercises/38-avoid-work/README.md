# 38 · Evitar trabalho

> **Módulo 9 — Performance** · ⏱ ~40 min · Visualização: [Performance](http://localhost:5173/#/performance) · **compila em Release**

## Objetivos

- Recalcular coeficientes só quando os parâmetros mudam.
- Perceber o que são **denormais** e desligá-los com `juce::ScopedNoDenormals`.
- Saltar o processamento de blocos em silêncio sem mudar o resultado.

## Teoria

### 1. Coeficientes só quando mudam

Um biquad precisa de `tan`, divisões... para calcular coeficientes (exercício 15). O
`processBlock` do plugin lê os parâmetros e chama `setCutoff` em **todos** os blocos (na thread de
áudio), quase sempre com o mesmo valor. Guarda o valor e
marca "precisa de recalcular" só quando muda; recalcula no início do próximo `process`. Vários setters
entre dois blocos → um só recálculo.

(Com *smoothing* de parâmetros, os coeficientes mudam durante a rampa; aí recalcula-se a cada N
amostras, não a cada bloco nem a cada amostra.)

### 2. Denormais

Um `float` normal guarda ~7 algarismos e um expoente; o menor normal é ~1.18·10⁻³⁸. Abaixo disso há os
números **desnormalizados** (*subnormal*), até ~1.4·10⁻⁴⁵, com cada vez menos precisão. Muitos
processadores tratam-nos por um caminho lento (microcódigo) — o teste de demonstração mede a diferença
nesta máquina (aqui: ~20×).

Em áudio aparecem sempre que uma estrutura com feedback (filtro IIR, reverb, delay com feedback) fica a
decair em silêncio: o estado vai encolhendo até à zona desnormalizada e pode lá ficar preso
(`0.9999 · s` arredonda de volta para `s` no menor desnormal). Resultado clássico: o plugin gasta mais
CPU quando paras a música.

Soluções:

- **FTZ/DAZ** (*flush-to-zero*, *denormals-are-zero*): bits do registo de controlo do SSE que tratam
  denormais como zero. `juce::ScopedNoDenormals` liga-os no construtor e repõe o estado anterior no
  destrutor (são por thread). Os wrappers de plugin do JUCE não o fazem por ti: o template de plugin do
  JUCE põe `juce::ScopedNoDenormals noDenormals;` no início do `processBlock`.
- Somar um ruído/DC minúsculo (ex.: 10⁻²⁰) no ciclo de feedback: técnica antiga, portátil.

### 3. Silêncio

Se a entrada é silêncio **e** o estado do filtro é exatamente zero, a saída é zero — não há nada para
calcular. Com FTZ ligado, um estado a decair acaba mesmo em zero, por isso o atalho dispara pouco depois
de o som parar. Para muitos plugins numa sessão (pistas paradas), isto poupa imenso.

O limiar de "silêncio" (aqui −160 dBFS) tem de estar abaixo de qualquer coisa audível; e o resultado tem
de ser **igual** ao de um filtro que processa tudo (o teste compara com uma referência).

## O exercício

Em `Processor.cpp`: **TODO(1)** setters que só marcam quando o valor muda, **TODO(2)** desligar os
denormais no `process`, **TODO(3)** `canSkip`.

## Armadilhas comuns

- **Saltar quando a entrada é silêncio mas o estado não é zero**: corta a cauda do filtro.
- **Chamar os setters de outra thread** (ex.: a GUI): `cutoff` e o "precisa de recalcular" são
  variáveis simples, só seguras na thread de áudio. A GUI fala com o áudio através dos parâmetros
  (módulo 8).
- **Ligar FTZ/DAZ "para sempre"** (sem RAII) na thread do host: afeta o código dos outros.

## Como verificar

```powershell
./learn check 38
./learn run 38     # fonte "Silêncio" ou "Impulsos": vê os "Blocos saltados" a subir
```

A demonstração dos denormais aparece como aviso (`warning`) no output do teste; para a ver sempre:
`build\exercises\38-avoid-work\Release\ex38_tests.exe [demo]`.

## Para saber mais

- Laurent de Soras, ["Denormal numbers in floating point signal processing applications" (PDF)](http://ldesoras.free.fr/doc/articles/denormal-en.pdf) — o artigo clássico sobre denormais em áudio e as soluções.
- [Wikipedia — Subnormal number](https://en.wikipedia.org/wiki/Subnormal_number) — o formato e as consequências de performance.
- [juce::ScopedNoDenormals — documentação](https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html) — FTZ/DAZ com RAII.
- [juce::dsp::IIR — `ArrayCoefficients`](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1ArrayCoefficients.html) — coeficientes sem alocar (usados aqui).
- Ross Bencina, ["Real-time audio programming 101"](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) — o pior caso é o que interessa.
