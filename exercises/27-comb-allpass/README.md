# 27 · Comb e allpass

> **Módulo 7 — Reverb** · ⏱ ~40 min · Visualização: [Comb e allpass](http://localhost:5173/#/comb-allpass)

## Objetivos

- Implementar o **filtro pente com feedback** (*feedback comb*), com e sem amortecimento.
- Implementar o **allpass de Schroeder** com uma só delay line.
- Perceber porque é que um tem picos no espectro e o outro é plano — e porque é que ambos "tocam".

## Teoria

### Uma sala é um monte de ecos

Numa sala, o som chega-te diretamente e depois refletido nas paredes, no teto, nas reflexões das
reflexões... Ao fim de umas dezenas de milissegundos são milhares de ecos por segundo, cada vez mais
fracos: a **cauda** da reverb. Em 1962, Manfred Schroeder mostrou que se podia imitar isto com poucos
blocos de delay com feedback. Esses blocos são os deste exercício.

### Comb com feedback

É o eco do exercício 12, sem dry, com um atraso curto (dezenas de ms):

```
y[n] = x[n − D] + g · y[n − D]
```

Um impulso sai como ecos em D, 2D, 3D... com ganhos 1, g, g², ... No espectro, a resposta tem **picos**
igualmente espaçados em múltiplos de fs/D — parece um pente, daí o nome. Ouvido sozinho, um comb soa
"metálico", a uma nota com frequência fs/D.

Neste exercício a linha guarda o valor que **entra** na linha e lê-se o que lá foi escrito há D passos:

```
saída     = linha[n − D]
linha[n]  = entrada + g · saída
```

### Comb com amortecimento (*lowpass-feedback comb*)

As paredes absorvem mais os agudos do que os graves. Para imitar isso, mete-se um passa-baixo de um polo
(exercício 14) **dentro** do ciclo:

```
filtrado  = (1 − d) · saída + d · filtrado
linha[n]  = entrada + g · filtrado
```

A cada volta os agudos perdem mais um pouco: a cauda escurece com o tempo, como numa sala real. É assim
que o Freeverb faz (ver *Para saber mais*). Com `d = 0` volta-se ao comb puro.

### Allpass de Schroeder

```
H(z) = (−g + z⁻ᴰ) / (1 − g · z⁻ᴰ)
```

O numerador é o "espelho" do denominador: `|H| = 1` em **todas** as frequências. Um allpass não muda o
espectro (de um som estacionário) — só **espalha** o som no tempo. A resposta a um impulso é
`−g` no instante 0 e depois `(1 − g²)`, `(1 − g²)·g`, `(1 − g²)·g²`... em D, 2D, 3D...

Com uma só delay line (guardando o sinal intermédio `w`):

```
w[n] = x[n] + g · w[n − D]
y[n] = w[n − D] − g · w[n]
```

"Plano" não quer dizer "sem cor": um impulso curto passa a ser uma série de ecos, e uma série de
allpasses curtos em cadeia soa a mola metálica (o artigo da Valhalla em *Para saber mais* explica
porquê). Por isso, numa reverb, os allpasses servem para aumentar a **densidade** de ecos, e os combs
para fazer a **cauda longa**.

> ⚠ **O "allpass" do Freeverb não é um allpass.** Segundo a análise do JOS, o código do Freeverb
> implementa `(−1 + (1 + g)·z⁻ᴺ) / (1 − g·z⁻ᴺ)`, que só é um allpass verdadeiro para
> g = (√5 − 1)/2 ≈ 0.618; o Freeverb usa g = 0.5. O `juce::Reverb` é baseado no Freeverb. Aqui
> implementamos o allpass verdadeiro.

## O exercício

Em `Processor.cpp`: **TODO(1)** `CombFilter::processSample` e **TODO(2)** `SchroederAllpass::processSample`.
A linha já vem reservada com **exatamente D posições** no `prepare`: a posição `index` é ao mesmo tempo a
mais antiga (D passos atrás) e a próxima a escrever.

## Armadilhas comuns

- **Escrever antes de ler**: o atraso fica D − 1 (ou 0). Lê sempre primeiro.
- **Esquecer o wrap-around** do índice.
- **Feedback ≥ 1**: o comb nunca para (ou explode). Os testes usam g < 1.
- **Pôr o passa-baixo fora do ciclo**: escurece o som todo por igual em vez de escurecer cada eco mais do
  que o anterior.

## Como verificar

```powershell
./learn check 27
./learn run 27     # "Impulsos": compara o comb (nota metálica) com o allpass (espalhado); muda o atraso
```

## Para saber mais

- Julius O. Smith III, ["Feedback Comb Filters" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Feedback_Comb_Filters.html) — equação, picos e resposta em frequência.
- Julius O. Smith III, ["Lowpass Feedback Comb Filter" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Lowpass_Feedback_Comb_Filter.html) — o comb com amortecimento usado no Freeverb.
- Julius O. Smith III, ["Allpass from Two Combs" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Allpass_Two_Combs.html) e ["Schroeder Allpass Sections"](https://ccrma.stanford.edu/~jos/pasp/Schroeder_Allpass_Sections.html) — de onde vem a forma do allpass.
- Julius O. Smith III, ["Freeverb Allpass Approximation" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Freeverb_Allpass_Approximation.html) — porque é que o "allpass" do Freeverb não o é.
- Sean Costello (Valhalla DSP), ["Reverbs: Diffusion, allpass delays, and metallic artifacts"](https://valhalladsp.com/2011/01/21/reverbs-diffusion-allpass-delays-and-metallic-artifacts/) — porque é que allpasses em série soam metálicos.
