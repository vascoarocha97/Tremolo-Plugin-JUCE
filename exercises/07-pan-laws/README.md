# 07 · Leis de pan

> **Módulo 2 — Ganho, dB, pan e smoothing** · ⏱ ~20 min · Visualização: [Leis de pan](http://localhost:5173/#/pan-laws)

## Objetivos

- Colocar uma fonte mono na imagem estéreo.
- Perceber porque é que há várias **leis de pan** e o que cada uma conserva.
- Comparar com o `juce::dsp::Panner` (e descobrir uma diferença importante).

## Teoria

Fazer pan é dar ganhos diferentes à esquerda (L) e à direita (R). A questão é: que ganhos no meio?

### Linear (−6 dB no centro)

`L = (1 − p)/2`, `R = (1 + p)/2`, com *p* de −1 a +1. A soma L + R é sempre 1: se o sinal for
somado a mono (os dois canais com a mesma forma de onda, em fase), o nível mantém-se. Mas os nossos
ouvidos em estéreo somam mais a **potência** do que a amplitude, e com L = R = 0.5 a potência total é
0.5 → o som parece **afundar no centro** (o "buraco no meio").

### Potência constante (−3 dB no centro)

Escolhe-se L e R tais que **L² + R² = 1** em todas as posições. Pensa em *p* como um ângulo θ entre
0 (esquerda) e π/2 (direita): `L = cos θ`, `R = sin θ`. No centro, L = R = 0.707 (−3 dB). O nível
percebido mantém-se constante ao mover o pan — mas somado a mono o centro fica +3 dB acima dos lados.

### Compromisso (−4.5 dB no centro)

Entre as duas: os ganhos de potência constante elevados a 1.5 (0.707^1.5 = 0.595 → −4.5 dB). É a
forma usada pelo `juce::dsp::PannerRule::sin4p5dB`. Há outras definições de −4.5 dB (por exemplo, a
média geométrica entre linear e potência constante) — diferem nas posições intermédias, não no centro.
Não há uma lei "certa": DAWs e consolas diferentes usam leis diferentes.

### O `juce::dsp::Panner` normaliza o centro

Lendo `juce_Panner.cpp` (JUCE 9.0.4): depois de calcular os ganhos, o Panner multiplica-os por um
*boost* para o **centro ficar a 0 dB** (×√2 na regra `sin3dB`, ×2^(3/4) na `sin4p5dB`, ×2 na
`linear`). Consequência: nos extremos o lado ativo fica **acima** de 0 dB (+3 dB na `sin3dB`). Um dos
testes compara as tuas curvas com as do Panner tendo isto em conta.

## O exercício

Em `Processor.cpp`: TODO(1)–(3) as três leis em `Pan::computeGains`, TODO(4) `processBlock`
(a fonte mono é a média de L e R; os ganhos calculam-se uma vez por bloco).

## Armadilhas comuns

- **`cos(π/2)` em `float` não é 0**: dá −4.4·10⁻⁸. Na lei de −4.5 dB, `pow(-4.4e-8, 1.5)` é **NaN**
  — e um NaN no áudio contamina tudo o que vem depois (filtros, reverbs...). Escreve os ganhos de
  forma a que os extremos sejam exatamente 0 (ex.: com `sin` de um ângulo que é exatamente 0 no
  extremo), ou limita a base a ≥ 0 antes do `pow`.
- **Pan em [0, 1] vs. [−1, 1]**: decide e converte uma vez.
- **Somar L + R sem dividir por 2** para obter o mono → +6 dB.

## Como verificar

```powershell
./learn check 07
./learn run 07     # com auscultadores: move o pan com cada lei e ouve o centro
```

## Para saber mais

- Carnegie Mellon University, *Introduction to Computer Music*, ["Loudness Concepts & Panning Laws"](https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/) —
  as três leis, porquê o "buraco no meio" e quando usar cada uma.
- [juce::dsp::Panner — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Panner.html) — as regras disponíveis no JUCE
  (a normalização do centro só se vê no código-fonte, `modules/juce_dsp/processors/juce_Panner.cpp`).
