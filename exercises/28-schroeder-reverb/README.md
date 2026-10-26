# 28 · Reverb de Schroeder

> **Módulo 7 — Reverb** · ⏱ ~45 min · Visualização: [Reverb](http://localhost:5173/#/reverb)

## Objetivos

- Montar uma reverb completa com os teus combs e allpasses do exercício 27 (estrutura do Freeverb).
- Calcular o feedback de cada comb a partir de um **RT60** pedido.
- Perceber como se mede uma reverb: RT60, curva de decaimento de Schroeder, descorrelação L/R.

## Teoria

### A estrutura

```
              ┌─ comb 1 ─┐
              ├─ comb 2 ─┤
entrada ──────┼─   ...  ─┼── Σ ── allpass 1 ── allpass 2 ── allpass 3 ── allpass 4 ── saída
              └─ comb 8 ─┘
```

- **Combs em paralelo** fazem a cauda: cada um gera ecos regulares com o seu período; com atrasos
  diferentes (e sem divisores comuns simples), os picos dos 8 pentes não coincidem e o resultado soa
  muito menos metálico do que um só.
- **Allpasses em série** multiplicam a densidade: cada eco que entra sai como uma série de ecos.

É a reverb de Schroeder (1962) com as afinações do Freeverb (Jezar, 2000), que também é a base do
`juce::Reverb`: atrasos dos combs 1116…1617 amostras e dos allpasses 556, 441, 341, 225, a 44.1 kHz.
Para outros sample rates escalam-se por `fs / 44100`.

### Estéreo

O canal direito usa os mesmos atrasos **+ 23 amostras** (o *stereo spread* do Freeverb). Os dois canais
ficam com ecos em instantes ligeiramente diferentes: **descorrelacionados**. É isso que dá a sensação de
espaço à volta da cabeça; com L = R a reverb soa "dentro" da cabeça, em mono.

### RT60 → feedback

O **RT60** é o tempo que a cauda leva a cair 60 dB (para um milésimo da amplitude). Num comb de D amostras,
cada volta multiplica o eco por g, e há `T60 · fs / D` voltas em T60 segundos. Queremos:

```
g^(T60 · fs / D) = 0.001
```

Resolver isto em ordem a g é o TODO(1). Nota que **cada comb tem um g diferente**: um comb mais comprido
dá menos voltas por segundo, por isso cada volta tem de perder mais.

(O Freeverb usa um único "room size" para todos os combs; com a fórmula acima o decaimento passa a ser um
número em segundos com significado físico.)

### Como se mede: a curva de Schroeder

A resposta a um impulso de uma reverb é ruidosa; medir o RT60 olhando para o pico é pouco fiável. Schroeder
propôs integrar a energia **de trás para a frente** (*energy decay curve*, EDC):

```
EDC(t) = Σ_{τ ≥ t} h²(τ)
```

A EDC em dB é quase uma reta; o teste mede o declive entre −5 e −25 dB (T20) e extrapola para 60 dB.

## O exercício

Em `Processor.cpp`: **TODO(1)** `feedbackForRt60` e **TODO(2)** o ciclo de combs/allpasses no `process`.
Os combs e allpasses são os **teus** (do exercício 27): se o 27 não passar, este também não.

## Armadilhas comuns

- **Mesmo g para todos os combs**: os combs curtos decaem mais devagar do que os compridos, o RT60
  deixa de bater certo.
- **Somar os allpasses em paralelo**: devem estar em série — cada um recebe a saída do anterior.
- **Esquecer o ganho de entrada** (1/8): 8 combs somados ficam ~18 dB acima do sinal.
- **Combs/allpasses partilhados entre L e R**: cada canal tem os seus (estado diferente).

## Como verificar

```powershell
./learn check 28
./learn run 28     # "Impulsos" ou "Loop de demonstração": varia decaimento e damping
```

## Para saber mais

- Julius O. Smith III, ["Schroeder Reverberators" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html) — a estrutura original e as suas variantes.
- Julius O. Smith III, ["Freeverb" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html) — a estrutura, as afinações e o stereo spread.
- Julius O. Smith III, ["Energy Decay Curve" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Energy_Decay_Curve.html) — a integração de Schroeder usada nos testes.
- [juce::Reverb — documentação](https://docs.juce.com/master/classjuce_1_1Reverb.html) — a versão JUCE do Freeverb (compara o código com o teu: `modules/juce_audio_basics/utilities/juce_Reverb.h`).
- M. R. Schroeder, "Natural Sounding Artificial Reverberation", *Journal of the Audio Engineering Society*, 10(3), 1962 — o artigo original (o JOS resume-o).
