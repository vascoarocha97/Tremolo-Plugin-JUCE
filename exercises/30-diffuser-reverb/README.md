# 30 · Difusor + FDN

> **Módulo 7 — Reverb** · ⏱ ~60 min · Visualização: [FDN](http://localhost:5173/#/fdn) (secção "Difusor")

## Objetivos

- Construir um **difusor multicanal**: atrasos diferentes por canal + inversões de polaridade + Hadamard.
- Implementar a transformada de Hadamard rápida (borboletas, O(N·log N)).
- Pôr o difusor à frente da tua FDN do exercício 29 e ouvir a diferença no ataque.

## Teoria

### O problema do exercício 29

A FDN com atrasos de 100–200 ms faz uma cauda longa e lisa, mas começa **esparsa**: o teste do 29 mostrou
que nos primeiros 200 ms quase nenhuma amostra tem som. Numa sala real, o início já é uma nuvem densa.

A ideia do artigo da Signalsmith: **separar** os dois trabalhos. A FDN só tem de fazer a cauda longa; um
**difusor** à entrada transforma cada impulso numa nuvem de ecos *antes* de chegar à FDN.

### Um passo de difusão

Com N = 8 canais, cada passo faz três coisas:

```
8 canais ──► atraso diferente por canal ──► inverter polaridade de alguns ──► Hadamard ──► 8 canais
```

1. **Atrasos**: um impulso que estava ao mesmo tempo nos 8 canais passa a estar em 8 instantes diferentes.
2. **Polaridades**: sem isto, a mesma combinação de sinais repetia-se em todos os passos.
3. **Hadamard**: cada um desses 8 ecos é espalhado pelos 8 canais.

Resultado: **8× mais ecos** por passo. Com 4 passos, 8⁴ = 4096 caminhos diferentes por canal.

Todas as três operações preservam a energia (um atraso não muda energia; inverter o sinal também não; a
Hadamard normalizada é ortogonal), por isso o difusor é um "allpass multicanal": não colore o som,
só o espalha no tempo. Não há feedback — não pode tocar nem explodir.

### Escolher os atrasos

Em vez de 8 números aleatórios no intervalo [0, L], divide-se o intervalo em 8 **segmentos iguais** e
escolhe-se um ponto aleatório dentro de cada um: ficam aleatórios mas bem distribuídos. Os passos têm
comprimentos 20, 40, 80 e 160 ms (cada um o dobro do anterior), como no exemplo do artigo; uma seed fixa
por passo torna tudo reprodutível.

(O artigo também **baralha** os canais em cada passo; aqui só invertemos polaridades, que chega para os
testes e para o ouvido. Experimenta acrescentar o baralhar como extensão.)

### Hadamard rápida

A matriz de Hadamard 8×8 tem só ±1. Multiplicar por ela ingenuamente custa 64 multiplicações; a versão
recursiva (H₂ₙ = [[Hₙ, Hₙ], [Hₙ, −Hₙ]]) faz-se com **borboletas**:

```
para half = 1, 2, 4:
    para cada bloco de 2·half elementos:
        para cada par (a, b) = (v[i], v[i + half]):
            (a, b) ← (a + b, a − b)
no fim: × 1/√N
```

São N·log₂N = 24 somas/subtrações. O fator 1/√N torna-a ortonormal: H·H = I.

### Juntar tudo

```
L, R ─► upmix para 8 canais ─► 4 passos de difusão ─► FDN (exercício 29, processFrame) ─► downmix ─► L, R
```

O `process` (já feito) usa `processFrame` da tua FDN com as 8 entradas difundidas.

Repara que a saída continua a ser lida das linhas da FDN: os primeiros ~100 ms (o atraso mais curto)
continuam em silêncio, mas o que vem a seguir já é denso. O artigo sugere, como extensão, juntar
*early reflections* tiradas do próprio difusor para preencher esse intervalo.

## O exercício

Em `Processor.cpp`: **TODO(1)** `Mixing::hadamard`, **TODO(2)** `DiffusionStep::prepare` (atrasos e
polaridades), **TODO(3)** `DiffusionStep::process`. Usa a tua FDN do exercício 29.

## Armadilhas comuns

- **Esquecer o 1/√N**: a energia multiplica-se por 8 a cada passo (o teste de energia apanha).
- **Atrasos todos iguais**: a Hadamard "desfaz-se" e não há difusão nenhuma.
- **Ler e escrever pela ordem errada** na linha de atraso (como no exercício 27).
- **Gerar números aleatórios no `process`**: tudo o que é aleatório escolhe-se no `prepare`.

## Como verificar

```powershell
./learn check 30
./learn run 30     # "Impulsos": compara o ataque com o do exercício 29
```

## Para saber mais

- Geraint Luff (Signalsmith Audio), ["Let's Write A Reverb"](https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/) — o difusor multicanal deste exercício, com exemplos de áudio (e a apresentação no ADC 21).
- Julius O. Smith III, ["Hadamard Matrix" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Hadamard_Matrix.html) — a matriz e o seu uso em FDNs.
- Sean Costello (Valhalla DSP), ["Reverbs: Diffusion, allpass delays, and metallic artifacts"](https://valhalladsp.com/2011/01/21/reverbs-diffusion-allpass-delays-and-metallic-artifacts/) — a abordagem clássica à difusão (allpasses em série) e os seus problemas.
- Jon Dattorro, ["Effect Design, Part 1: Reverberator and Other Filters" (JAES 1997)](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf) — a reverb "plate" clássica: difusão com allpasses à entrada e um "tanque" em forma de 8.
