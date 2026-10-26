# 14 · Filtro de um polo

> **Módulo 4 — Filtros** · ⏱ ~30 min · Visualização: [Filtro de um polo](http://localhost:5173/#/one-pole)

## Objetivos

- Escrever o filtro IIR mais simples e perceber o que é um **polo** e uma **constante de tempo**.
- Calcular o coeficiente a partir de uma frequência de corte (e saber porque é aquela fórmula).
- Obter um passa-alto "de graça" a partir do passa-baixo.

## Teoria

### A equação

```
y[n] = y[n−1] + a · (x[n] − y[n−1])        com 0 < a ≤ 1
```

Em cada amostra a saída anda uma fração `a` do caminho até à entrada. Com `a = 1` a saída é a
entrada; com `a` pequeno, a saída "arrasta-se": as variações rápidas (altas frequências) são
atenuadas, as lentas (graves, DC) passam. É um **passa-baixo de 1.ª ordem**: −6 dB por oitava acima
do corte. É também, literalmente, o "smoothing exponencial" que se usa para suavizar parâmetros.

Reescrita como `y[n] = a·x[n] + (1 − a)·y[n−1]`, vê-se que é um filtro **IIR** (resposta infinita
ao impulso: a saída depende das saídas anteriores) com um **polo** em `z = 1 − a`. Enquanto o polo
estiver dentro do círculo unitário (0 < a < 2, aqui 0 < a ≤ 1), o filtro é estável.

### Do corte ao coeficiente

O análogo analógico é um circuito RC, cuja resposta a um degrau é `1 − e^(−t/τ)` com `τ = RC =
1/(2π·fc)`. Se quisermos que o filtro digital tenha **exatamente** essa resposta ao degrau
amostrada, o fator por amostra tem de ser `e^(−T/τ)` com `T = 1/fs`:

```
a = 1 − e^(−2π·fc/fs)
```

Para fc muito abaixo de fs, `a ≈ 2π·fc/fs` (o primeiro termo de Taylor), e é essa a aproximação que
aparece em muitos códigos — funciona mal perto de Nyquist. Este mapeamento chama-se *impulse
invariant*. Repara que o −3 dB não fica exatamente em fc (fica perto, para fc ≪ fs); se precisares
de um corte exato, a alternativa é o *bilinear transform* com pré-distorção (o
`juce::dsp::IIR::Coefficients::makeFirstOrderLowPass` e o `juce::dsp::FirstOrderTPTFilter` fazem isso).

A **constante de tempo** τ é o tempo que a resposta ao degrau demora a chegar a 1 − 1/e ≈ 63.2%
do valor final. O teste mede isto.

### Passa-alto

`passa-alto = x − passa-baixo`: o que o passa-baixo deixou para trás. Os dois somam sempre o original
(filtros **complementares**). Com um corte de 5–20 Hz, este passa-alto é um *DC blocker* — vais
precisar dele no fuzz (exercício 23).

## O exercício

Em `Processor.cpp`: **TODO(1)** `setCutoff` (com os limites) e **TODO(2)** `processSample`.
O `OnePoleProcessor` (um filtro por canal) já está feito.

## Armadilhas comuns

- **`a = 2π·fc/fs` sem o `exp`**: para cortes altos dá `a > 1` e o filtro fica instável ou estranho.
- **Um único estado para todos os canais**: o canal direito "herda" o esquerdo.
- **Recalcular `exp` em cada amostra** quando o corte não muda.
- **Denormais**: quando a entrada fica a zero, o estado decai para valores minúsculos (denormais) que
  podem ser muito lentos em CPU. O host do curso usa `juce::ScopedNoDenormals`; vês isto a fundo no
  exercício 38.

## Como verificar

```powershell
./learn check 14
./learn run 14     # "Ruído branco": no espectro a inclinação é −6 dB/oitava
```

## Para saber mais

- Julius O. Smith III, [*Introduction to Digital Filters* — "One-Pole"](https://ccrma.stanford.edu/~jos/filters/One_Pole.html) —
  o filtro de um polo, o seu polo e a resposta em frequência.
- Nigel Redmon (EarLevel Engineering), ["A one-pole filter"](https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/) —
  a mesma fórmula do coeficiente, com a derivação e usos práticos (DC blocker, smoothing).
- [juce::dsp::IIR::Coefficients](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1Coefficients.html) —
  `makeFirstOrderLowPass/HighPass` (bilinear transform) para comparar.
