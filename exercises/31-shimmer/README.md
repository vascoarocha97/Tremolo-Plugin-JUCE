# 31 · Shimmer

> **Módulo 7 — Reverb** · ⏱ ~60 min · Visualização: [Shimmer](http://localhost:5173/#/shimmer)

## Objetivos

- Escrever o pitch shifter mais simples que existe: **duas leituras de uma delay line** com crossfade.
- Pô-lo **dentro do ciclo de feedback** de uma reverb (a tua FDN do exercício 29).
- Perceber porque é que um ciclo com pitch shift precisa de filtro e de limitador.

## Teoria

### Mudar o tom com uma delay line

No exercício 20 (chorus) viste que um atraso que **varia** muda o tom: se o atraso diminui, a leitura
"anda mais depressa" do que a escrita e a frequência sobe (efeito Doppler). Com atraso `d[n]`, a leitura
anda à velocidade `1 − d'[n]` amostras por amostra. Para multiplicar a frequência por `ratio`:

```
velocidade de leitura = ratio    ⇒    d' = 1 − ratio  (amostras por amostra)
```

Para uma oitava acima (ratio = 2), o atraso tem de **diminuir** 1 amostra por amostra. Não pode diminuir
para sempre: quando chega a 0, salta de volta para o máximo (a "janela", aqui 50 ms). Esse salto é um
clique.

### Duas leituras e crossfade

Solução clássica: **duas** leituras desfasadas de meia janela. Quando uma está perto do salto, a outra
está a meio caminho. Pesa-se cada uma com uma janela de Hann em função da sua fase `p ∈ [0, 1)`:

```
gain(p) = sin²(π·p)          gain(p) + gain(p + ½) = sin² + cos² = 1
```

A leitura que vai saltar (p ≈ 0 ou 1) tem ganho 0 nesse instante; o volume total fica constante.

```
fase:      0 ────────────► 1 (salta para 0)
atraso:    1 + p · janela
leitura A: fase p              ganho sin²(π p)
leitura B: fase p + ½ (mod 1)  ganho sin²(π (p + ½))
```

Não é um pitch shifter de alta qualidade (há modulação de amplitude e "fantasmas" à frequência da janela —
técnicas melhores: PSOLA, phase vocoder; ver a apresentação da Signalsmith), mas é barato e, **dentro de uma reverb**, tanto os artefactos como a latência (em média meia
janela, ~25 ms) perdem-se na cauda.

### O ciclo do shimmer

```
entrada ──► (+) ──► FDN (exercício 29) ──┬──► saída
             ▲                           │
             └── × shimmer ◄── tanh ◄── passa-baixo ◄── pitch +12 ◄── (atraso de 1 amostra)
```

Cada volta pelo ciclo sobe uma oitava: 440 → 880 → 1760 → 3520 Hz... A cauda vai "subindo" e brilhando.
Valores de +12 semitons e feedback ≥ 0.5 dão o som clássico (ver a página da Valhalla Shimmer).

Dois cuidados num ciclo destes:

- **Passa-baixo** (um polo a ~6 kHz): cada oitava acima perde mais do que a anterior, por isso a subida
  acaba por se apagar em vez de se acumular perto de Nyquist (onde também fazia aliasing).
- **`tanh`**: a FDN tem picos de ressonância; com shimmer alto, o ganho do ciclo pode passar de 1 nalgumas
  frequências. O tanh (exercício 22) garante que, no pior caso, o ciclo satura em vez de explodir.
  Num plugin, uma saída que pode crescer sem limite é um bug — e um risco para os ouvidos de alguém.

## O exercício

Em `Processor.cpp`: **TODO(1)** `setSemitones`, **TODO(2)** `crossfadeGain`, **TODO(3)**
`PitchShifter::processSample`, **TODO(4)** o ciclo do shimmer no `ShimmerReverb::process`. A leitura com
interpolação linear (`readDelayed`) já está feita. Usa a tua FDN do exercício 29.

## Armadilhas comuns

- **Sinal errado no avanço da fase**: o tom desce em vez de subir.
- **Fase fora de [0, 1)**: para ratio > 1 a fase anda para trás — o wrap tem de funcionar nos dois sentidos.
- **Usar a saída da FDN da amostra atual no feedback**: é um ciclo sem atraso, impossível de calcular;
  usa a da amostra anterior (`lastWet`).
- **Esquecer o limitador**: com shimmer = 1 e decaimento longo o som cresce até ao infinito (ou NaN).

## Como verificar

```powershell
./learn check 31
./learn run 31     # "Loop de demonstração", decaimento 6–10 s, sobe o Shimmer; experimenta +7 e −12
```

## Para saber mais

- Valhalla DSP, [ValhallaShimmer](https://valhalladsp.com/shop/reverb/valhalla-shimmer/) — o manual descreve o pitch shift no ciclo de feedback e o som "Eno/Lanois" (+12 semitons, feedback ≥ 0.5).
- Geraint Luff (Signalsmith Audio), "Four Ways To Write A Pitch-Shifter" (ADC 2022) — e o [código de exemplo](https://github.com/Signalsmith-Audio/pitch-time-example-code) usado nas demos.
- Jon Dattorro, ["Effect Design, Part 1" (JAES 1997)](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf) — uma reverb clássica com modulação no ciclo, base de muitos shimmers.
- U. Zölzer (ed.), *DAFX: Digital Audio Effects*, 2.ª ed., Wiley, 2011 — capítulo sobre modificação de tom e tempo (pitch shifting com delay lines e alternativas).
