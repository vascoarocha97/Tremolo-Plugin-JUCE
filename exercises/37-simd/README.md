# 37 · SIMD

> **Módulo 9 — Performance** · ⏱ ~45 min · Visualização: [Performance](http://localhost:5173/#/performance) · **compila em Release**

## Objetivos

- Usar `juce::FloatVectorOperations` para operações sobre arrays.
- Escrever um algoritmo com `juce::dsp::SIMDRegister<float>`, incluindo alinhamento, cabeça e cauda.
- Perceber quando o compilador já vetoriza sozinho e quando não.

## Teoria

### Uma instrução, vários dados

Os processadores x86-64 têm registos SSE de 128 bits = 4 floats (AVX: 256 bits = 8). Uma instrução
`addps` soma 4 pares de floats de uma vez, no mesmo tempo que `addss` soma um. Em áudio, onde se faz a
mesma conta a milhares de amostras, isto é ouro.

### `juce::FloatVectorOperations`

Operações prontas e vetorizadas sobre arrays: `add`, `multiply`, `addWithMultiply`, `copyWithMultiply`,
`findMinAndMax`, `clip`, ... São o primeiro sítio a procurar. (O `AudioBuffer::addFrom`, `applyGain`,
etc. usam-nas por dentro.)

### `juce::dsp::SIMDRegister<float>`

Quando a conta não existe pronta, escreve-se com um "float com N faixas":

```cpp
using Register = juce::dsp::SIMDRegister<float>;
auto acc = Register::expand (0.0f);        // [0, 0, 0, 0]
auto v = Register::fromRawArray (ptr);     // [p0, p1, p2, p3] — ptr TEM de estar alinhado
acc += v * v;                              // 4 multiplicações e 4 somas numa instrução cada
float total = acc.sum();                   // p0² + p1² + p2² + p3²  (soma "horizontal")
```

`Register::SIMDNumElements` diz quantas faixas há (o teste mostra-o: 4 com SSE).

### Alinhamento: cabeça, corpo, cauda

`fromRawArray` exige um endereço múltiplo de 16 bytes (`Register::isSIMDAligned`). Um buffer de áudio
pode começar em qualquer lado (um sub-bloco começa a meio). O padrão é:

```
[cabeça: 1 a 1 até alinhar] [corpo: 4 de cada vez] [cauda: o resto, 1 a 1]
```

### O compilador também sabe

Em Release, o MSVC (e o GCC/Clang) **vetorizam sozinhos** ciclos simples como o `mixScalar`
(`dest[i] += g * src[i]`): o teste mostra tempos parecidos para a versão escalar e para a
`FloatVectorOperations`. Mas uma **soma acumulada** de floats (`sum += x*x`) não é vetorizada com as
opções de vírgula flutuante normais (`/fp:precise`): somar em 4 faixas muda a ordem das somas e,
em vírgula flutuante, `(a + b) + c ≠ a + (b + c)`. Aí o `SIMDRegister` ganha ~4×. Por isso os
resultados da tua versão e da escalar não são bit a bit iguais — o teste tolera uma diferença relativa
de 10⁻⁵.

## O exercício

Em `Processor.cpp`: **TODO(1)** `mix`, **TODO(2)** `peak` (com `FloatVectorOperations`), **TODO(3)**
`sumOfSquares` (com `SIMDRegister`, para qualquer alinhamento e tamanho).

## Nota sobre as medições

Os testes de velocidade assumem as opções de vírgula flutuante por omissão do MSVC (`/fp:precise`,
as deste projeto). Com `/fp:fast` (ou `-ffast-math` noutros compiladores) o compilador pode, sozinho,
tirar contas do ciclo ou vetorizar somas — a versão de referência fica mais rápida e a comparação
deixa de dizer respeito ao teu código.

## Armadilhas comuns

- **`fromRawArray` num ponteiro desalinhado**: em Debug, `jassert`; em Release, crash ou lentidão.
- **Esquecer a cauda** (n não múltiplo de 4) ou a **cabeça** (ponteiro desalinhado).
- **Ler para lá do fim** do array no último grupo de 4.
- **Assumir 4 faixas**: usa `SIMDNumElements`.

## Como verificar

```powershell
./learn check 37
./learn run 37
```

## Para saber mais

- [juce::FloatVectorOperations — documentação](https://docs.juce.com/master/classjuce_1_1FloatVectorOperations.html) — a lista completa de operações.
- [juce::dsp::SIMDRegister — documentação](https://docs.juce.com/master/structjuce_1_1dsp_1_1SIMDRegister.html) — `fromRawArray`, `isSIMDAligned`, `sum`, operadores.
- [SIMDRegisterDemo.h](https://github.com/juce-framework/JUCE/blob/master/examples/DSP/SIMDRegisterDemo.h) — exemplo oficial: um filtro IIR a processar vários canais em paralelo.
- [Microsoft — Auto-Parallelization and Auto-Vectorization](https://learn.microsoft.com/en-us/cpp/parallel/auto-parallelization-and-auto-vectorization) — o que o vetorizador do MSVC faz e quando desiste.
- Agner Fog, [Software optimization resources](https://www.agner.org/optimize/) — "Optimizing software in C++", capítulo sobre vetorização.
