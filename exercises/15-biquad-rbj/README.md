# 15 · Biquad (Audio EQ Cookbook)

> **Módulo 4 — Filtros** · ⏱ ~45 min · Visualização: [Biquad e resposta em frequência](http://localhost:5173/#/biquad)

## Objetivos

- Implementar um **biquad** (filtro IIR de 2.ª ordem) na forma **Transposed Direct Form II**.
- Usar as fórmulas do **Audio EQ Cookbook** (Robert Bristow-Johnson): passa-baixo, passa-alto,
  passa-banda, notch, peaking e shelves.
- Calcular a resposta em frequência `|H(e^jω)|` a partir dos coeficientes.

## Teoria

### O biquad

```
y[n] = b0·x[n] + b1·x[n−1] + b2·x[n−2] − a1·y[n−1] − a2·y[n−2]

         b0 + b1·z⁻¹ + b2·z⁻²
H(z) = ------------------------        ("bi-quadrático": dois polinómios de 2.º grau)
         1  + a1·z⁻¹ + a2·z⁻²
```

Cinco números chegam para todos os filtros "de EQ" clássicos. Filtros de ordem maior fazem-se com
vários biquads em série (é o que o `juce::dsp::FilterDesign` faz).

### Transposed Direct Form II (TDF-II)

A mesma equação pode ser calculada de várias formas. A TDF-II usa só **dois estados** e comporta-se
bem em `float`:

```
y  = b0·x + s1
s1 = b1·x − a1·y + s2
s2 = b2·x − a2·y
```

Repara na ordem: o `y` usa o `s1` antigo; o `s1` novo usa o `s2` antigo.

### As fórmulas do cookbook

Todas partem de três intermédias: `w0 = 2π·f0/Fs`, `alpha = sin(w0)/(2Q)` e, para peaking e
shelves, `A = 10^(dBgain/40)` (raiz quadrada do ganho linear). Depois cada tipo tem os seus
`b0..a2` — estão no cookbook (link abaixo), e vais escrevê-las. No fim divide-se tudo por `a0`.

Estas fórmulas vêm de protótipos analógicos convertidos com o *bilinear transform* com
pré-distorção em f0: por isso a frequência f0 cai exatamente onde pedes. Propriedades que os testes
verificam:

- passa-baixo/alto: `|H(f0)| = Q` (com Q = 0.7071, −3.01 dB);
- passa-banda ("constant 0 dB peak gain"): 0 dB em f0; notch: zero em f0;
- peaking: exatamente `dBgain` em f0;
- shelves: o ganho total de um lado, **metade (em dB)** em f0.

O JUCE usa as mesmas fórmulas (`juce::dsp::IIR::ArrayCoefficients`): um teste compara a resposta
dos teus coeficientes com a do JUCE em todo o espectro.

### Resposta em frequência

Para saber o ganho a uma frequência *f*, avalia-se `H(z)` em `z = e^(jω)`, `ω = 2π·f/fs`, ou seja,
`z⁻¹ = e^(−jω)`. Em C++, `std::complex<double>` e `std::polar (1.0, −ω)` fazem isto em 3 linhas. É
o que desenha as curvas de um EQ.

### Modular um biquad

Mudar os coeficientes de um biquad em cada amostra (por exemplo, um corte varrido por um LFO) pode
dar ruído ou até instabilidade momentânea, porque o estado foi calculado com os coeficientes
anteriores. Para modulação rápida usa-se o **SVF** (exercício 16).

## O exercício

Em `Processor.cpp`:

1. **TODO(1)** as intermédias `w0`, `alpha`, `A`.
2. **TODO(2)** as sete fórmulas.
3. **TODO(3)** `magnitudeAt`.
4. **TODO(4)** `Biquad::processSample` (TDF-II).

O `BiquadFilter` (um biquad por canal, recalcula os coeficientes só quando algum parâmetro muda) já
está feito.

## Armadilhas comuns

- **Esquecer de normalizar por `a0`**: o filtro fica com ganho e polos errados.
- **Sinal de `a1`/`a2`**: no cookbook o denominador é `a0 + a1·z⁻¹ + a2·z⁻²`; na equação de
  diferenças eles aparecem **subtraídos**.
- **`A` = 10^(dB/20)** em vez de 10^(dB/40): o peaking fica com o dobro do ganho em dB.
- **Usar `float` nas fórmulas**: para cortes muito baixos (20 Hz a 96 kHz) a precisão de `float` no
  cálculo dos coeficientes estraga o filtro. Calcula em `double`, processa em `float`.

## Como verificar

```powershell
./learn check 15
./learn run 15     # "Ruído branco" e cada tipo de filtro; repara na forma no espectro
```

## Para saber mais

- R. Bristow-Johnson, [Audio EQ Cookbook (W3C)](https://www.w3.org/TR/audio-eq-cookbook/) — as fórmulas.
- Julius O. Smith III, [*Introduction to Digital Filters* — "BiQuad Section"](https://ccrma.stanford.edu/~jos/filters/BiQuad_Section.html) e
  ["Transposed Direct-Forms"](https://ccrma.stanford.edu/~jos/filters/Transposed_Direct_Forms.html) — a estrutura e as formas de implementação.
- Julius O. Smith III, [PASP — "Bilinear Transformation"](https://ccrma.stanford.edu/~jos/pasp/Bilinear_Transformation.html) — de onde vem o `tan` e a pré-distorção.
- [juce::dsp::IIR::ArrayCoefficients](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1ArrayCoefficients.html) — as mesmas fórmulas no JUCE.
