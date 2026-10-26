# 11 · Delay fracionário

> **Módulo 3 — Delay** · ⏱ ~30 min · Visualização: [Interpolação e delay fracionário](http://localhost:5173/#/fractional-delay)

## Objetivos

- Ler uma delay line **entre amostras** com interpolação linear.
- Perceber o custo da interpolação linear (é um filtro passa-baixo).
- Mudar o tempo de delay sem clicks — e ouvir o efeito Doppler que daí resulta.

## Teoria

### Porque é que um atraso inteiro não chega

10 ms a 44.1 kHz são 441 amostras; 10.01 ms são 441.441. E num chorus (exercício 20) o atraso muda
continuamente: se só pudesse ser inteiro, mudaria aos saltos de uma amostra — cada salto é uma
pequena descontinuidade, e uma sequência deles soa a ruído. Precisamos de valores **entre** amostras.

### Interpolação linear

Para um atraso `d = i + f` (parte inteira *i*, fração *f* ∈ [0, 1)), traça-se uma reta entre as duas
amostras vizinhas:

```
y = x[n − i] + f · (x[n − i − 1] − x[n − i])      =  (1 − f)·x[n − i] + f·x[n − i − 1]
```

Com *f* = 0.25, um impulso sai dividido em 0.75 e 0.25 (é o primeiro teste).

### O preço: é um filtro

Para *f* = 0.5 a fórmula é a média de duas amostras consecutivas — um passa-baixo simples, com ganho
`|H(f)| = cos(π·f/fs)`: quase 1 a 1 kHz, 0.79 a 10 kHz e **0.26 a 20 kHz** (a 48 kHz). Um dos testes
mede exatamente isto. Em *f* = 0 ou 1 não há perda; o pior caso é a meio. Quando o atraso é modulado,
isto vira uma modulação de brilho. Alternativas com melhor resposta: **Lagrange de 3.ª ordem** (o
`juce::dsp::DelayLineInterpolationTypes::Lagrange3rd`) e **allpass/Thiran** (planos em magnitude, mas
com estado — complicados de modular).

### Mudar o atraso com smoothing (e o Doppler)

Se o atraso passar de 10 ms para 30 ms de uma vez, a leitura salta 960 amostras para trás: um click.
Com o atraso a deslizar (aqui com um `juce::SmoothedValue` de 50 ms), o ponto de leitura move-se
mais devagar ou mais depressa do que a escrita — o som sai **transposto** enquanto o atraso muda.
É o efeito Doppler, e é exatamente o mecanismo do chorus, do flanger e do pitch shift por delay
(exercício 31).

## O exercício

Em `Processor.cpp`:

1. **TODO(1)–(2)** `push` e `read`: iguais ao exercício 10 — **copia a tua solução**.
2. **TODO(3)** `readLinear`.
3. **TODO(4)** `FractionalDelay::process` — o atraso suavizado avança uma vez por amostra (igual para
   todos os canais).

## Armadilhas comuns

- **`floor` vs. truncar com atrasos negativos**: aqui o atraso é limitado a ≥ 0 antes, por isso
  `static_cast<int>` chega.
- **Trocar as duas amostras** na interpolação: com *f* = 0.25 dá 0.25/0.75 em vez de 0.75/0.25.
- **Avançar o `SmoothedValue` por canal** (mesmo erro do exercício 08).
- **Esquecer a amostra extra** no buffer: interpolar no atraso máximo lê `máximo + 1`.

## Como verificar

```powershell
./learn check 11
./learn run 11     # "Seno" a 440 Hz, Mistura = 1, e arrasta o Atraso
```

## Para saber mais

- Julius O. Smith III, [PASP — "Delay-Line Interpolation"](https://ccrma.stanford.edu/~jos/pasp/Delay_Line_Interpolation.html) e
  ["Linear Interpolation"](https://ccrma.stanford.edu/~jos/pasp/Linear_Interpolation.html) — a fórmula e a resposta em frequência.
- Julius O. Smith III, [PASP — "Lagrange Interpolation"](https://ccrma.stanford.edu/~jos/pasp/Lagrange_Interpolation.html) — a alternativa de ordem mais alta.
- Julius O. Smith III, [PASP — "Doppler Effect"](https://ccrma.stanford.edu/~jos/pasp/Doppler_Effect.html) — porque é que um atraso variável muda a altura.
- [juce::dsp::DelayLine](https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html) — os tipos de interpolação disponíveis no JUCE.
