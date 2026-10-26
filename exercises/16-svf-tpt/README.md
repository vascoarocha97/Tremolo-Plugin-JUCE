# 16 · State Variable Filter (TPT)

> **Módulo 4 — Filtros** · ⏱ ~45 min · Visualização: [State Variable Filter](http://localhost:5173/#/svf)

## Objetivos

- Implementar o **SVF** na forma *topology-preserving transform* (TPT), com três saídas.
- Perceber porque é que este filtro aguenta modulação por amostra e um biquad "ingénuo" não.

## Teoria

### O filtro de variáveis de estado

O SVF analógico são **dois integradores** em série com feedback. Os "estados" são as saídas dos
integradores, e cada um é uma saída útil:

- depois do 1.º integrador: **passa-banda** (BP);
- depois do 2.º: **passa-baixo** (LP);
- o que entra no 1.º: **passa-alto** (HP).

Com `R2 = 1/Q` (amortecimento), vale sempre `x = HP + R2·BP + LP`. Um teste verifica-o.

### TPT: preservar a topologia

Em vez de converter a função de transferência para um biquad, o método de Vadim Zavalishin
discretiza **cada integrador** com a regra trapezoidal (o mesmo que o bilinear transform) e mantém a
estrutura do circuito. Há um problema: o feedback do circuito, uma vez discretizado, forma um ciclo
**sem atraso** (a saída depende de si própria na mesma amostra). Resolve-se algebricamente: o fator
`h = 1/(1 + R2·g + g²)` é a solução desse ciclo. Daí a ordem:

```
g  = tan(π·fc/fs),   R2 = 1/Q,   h = 1/(1 + R2·g + g²)

HP = h·(x − s1·(g + R2) − s2)
BP = g·HP + s1;   s1 = g·HP + BP          (integrador trapezoidal 1)
LP = g·BP + s2;   s2 = g·BP + LP          (integrador trapezoidal 2)
```

### Porquê o trabalho todo?

Os estados `s1` e `s2` têm um significado físico (correntes/tensões do circuito) que **não depende
dos coeficientes**. Quando mudas o corte, o estado continua a fazer sentido e a saída muda
suavemente. Num biquad em forma direta, os estados são combinações de amostras passadas que só fazem
sentido para os coeficientes com que foram calculados — mudar os coeficientes depressa pode criar
transientes ou até instabilidade. Por isso SVFs TPT são o padrão em sintetizadores e em efeitos com
filtros modulados (wah, auto-filtro). O teste "estável com o corte a mudar em cada amostra" sorteia
um corte novo entre 20 Hz e 20 kHz em **cada** amostra com Q = 20.

A versão de Andrew Simper (Cytomic) é algebricamente equivalente, com outra arrumação dos cálculos.

### No JUCE

O `juce::dsp::StateVariableTPTFilter` é exatamente esta estrutura (lê
`modules/juce_dsp/processors/juce_StateVariableTPTFilter.cpp`); um teste compara a tua saída com a
dele amostra a amostra. Repara que a saída BP não é normalizada: o ganho em fc é Q (para 0 dB no
pico, multiplica por R2).

## O exercício

Em `Processor.cpp`: **TODO(1)** `setParameters` (g, R2, h, com limites) e **TODO(2)**
`processSample`. O `SvfProcessor` (com um LFO que modula o corte em oitavas, por amostra) já está feito.

## Armadilhas comuns

- **Atualizar `s1` antes de calcular o BP** (ou `s2` antes do LP): cada integrador usa o estado antigo.
- **`tan` perto de Nyquist** explode: por isso o corte é limitado a 0.49·fs.
- **Q = 0** → R2 infinito.

## Como verificar

```powershell
./learn check 16
./learn run 16     # Q alto, modulação de 2–3 oitavas, LFO rápido: soa a "auto-wah" e não rebenta
```

## Para saber mais

- Vadim Zavalishin, [*The Art of VA Filter Design* (rev. 2.1.2, PDF)](https://www.discodsp.net/VAFilterDesign_2.1.2.pdf) —
  o livro de referência sobre TPT (capítulos sobre integradores trapezoidais, ciclos sem atraso e o SVF).
  (O PDF original no site da Native Instruments, rev. 2.1.0, deu 404 em 2026-10-26; esta é a revisão
  2.1.2, num espelho anunciado no fórum KVR. O livro permite cópias integrais e não modificadas.)
- Andrew Simper (Cytomic), ["Linear Trapezoidal Integrated SVF"](https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf) —
  a versão otimizada, com as fórmulas para todos os tipos de saída.
- [juce::dsp::StateVariableTPTFilter — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1StateVariableTPTFilter.html).
