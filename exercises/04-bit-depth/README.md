# 04 · Bit depth e quantização

> **Módulo 1 — Fundamentos** · ⏱ ~25 min · Visualização: [Quantização e bit depth](http://localhost:5173/#/bit-depth)

## Objetivos

- Perceber o que é **bit depth** e o que se perde ao **quantizar**.
- Medir o **SNR** de um sinal quantizado e ver de onde vem a regra "~6 dB por bit".
- Perceber o que o **dither** faz e porque é que um sinal mais fraco do que 1 bit pode sobreviver.

## Teoria

### Bit depth

Num ficheiro WAV de 16 bits ou num conversor de 24 bits, cada amostra é um **inteiro**: com N bits
há 2ᴺ níveis possíveis. Na escala do JUCE ([−1, 1)), um inteiro com sinal de N bits vai de
−2ᴺ⁻¹ a 2ᴺ⁻¹−1, por isso a distância entre níveis — **1 LSB** (*least significant bit*) — é
1/2ᴺ⁻¹. Repara que o +1.0 não é representável: o máximo é 1 − LSB.

Dentro do plugin trabalhas em `float` de 32 bits (24 bits de mantissa **mais** expoente), por isso a
quantização "a sério" acontece nas fronteiras: na conversão AD/DA e ao exportar para 16/24 bits.
E, de propósito, em efeitos como o bitcrusher (exercício 24).

### Quantizar

Quantizar é arredondar para o nível mais próximo. Aqui usa-se quantização *mid-tread* (o zero é um
nível): `q(x) = round(x / LSB) · LSB`, limitado a [−1, 1 − LSB]. O erro fica sempre entre −LSB/2 e
+LSB/2.

### ~6 dB por bit

Para sinais "ocupados", o erro comporta-se como ruído uniforme em [−LSB/2, LSB/2], cuja variância é
LSB²/12 (ver JOS, "Round-Off Error Variance"). Um seno em escala completa tem amplitude ≈ 2ᴺ⁻¹·LSB e
potência metade do quadrado da amplitude. A razão entre as duas dá:

```
SNR ≈ 6.02·N + 1.76 dB
```

16 bits → ~98 dB; 24 bits → ~146 dB (mais do que qualquer conversor real consegue). O teste mede isto
com 8, 12 e 16 bits.

### Dither

A suposição "o erro é ruído" falha para sinais pequenos: aí o erro **depende do sinal** — soa a
distorção, e um sinal com menos de meio LSB **desaparece** (vira silêncio). O teste "sem dither, um
sinal abaixo de meio LSB desaparece" mostra-o.

**Dither** é somar um pouco de ruído **antes** de quantizar. Com ruído **TPDF** (distribuição
triangular, ±1 LSB, que se obtém somando ou subtraindo dois números aleatórios uniformes
independentes), a média e a potência do erro deixam de depender do sinal (Lipshitz, Wannamaker &
Vanderkooy, 1992). O resultado: em vez de distorção tens um ruído de fundo constante e o sinal
fraco **continua lá, enterrado no ruído** (o teste mede a amplitude do seno à saída).

O preço: o ruído total passa de LSB²/12 para LSB²/4 (o dither TPDF acrescenta LSB²/6), ou seja,
~4.8 dB mais ruído. Na prática aplica-se dither **uma vez**, no fim, ao exportar para 16 bits.
(Há ainda o *noise shaping*, que empurra esse ruído para frequências menos audíveis — fica fora
deste exercício.)

## O exercício

Em `Processor.cpp`:

1. **TODO(1)** `Quantiser::stepSize()`.
2. **TODO(2)** `Quantiser::quantise()`.
3. **TODO(3)** `BitDepthReducer::processBlock()` — quantiza todos os canais.
4. **TODO(4)** — o dither TPDF antes de quantizar, quando está ligado.

O gerador aleatório é um `juce::Random` com *seed* fixa no `prepareToPlay`, para os testes serem
reproduzíveis. Repara que gerar números aleatórios assim não aloca nem bloqueia (ao contrário de, por
exemplo, `std::random_device`, que pode ir ao sistema operativo).

## Armadilhas comuns

- **LSB errado por um fator de 2**: 2/2ᴺ e 1/2ᴺ⁻¹ são o mesmo; 1/2ᴺ não é.
- **Esquecer o limite superior**: `round(1.0 / LSB)` dá um nível que não existe (+2ᴺ⁻¹).
- **Dither depois de quantizar**: tem de ser antes.
- **Dither uniforme (RPDF)** em vez de triangular: a média do erro fica independente do sinal, mas a
  potência não (o ruído "respira" com o sinal).

## Como verificar

```powershell
./learn check 04
./learn run 04
```

No standalone: com 4–6 bits ouve-se o ruído/distorção da quantização. Depois baixa o "Nível" da
fonte para −40 dB, põe 8 bits e compara dither ligado/desligado.

## Para saber mais

- Julius O. Smith III, [*Mathematics of the DFT* — "Round-Off Error Variance"](https://ccrma.stanford.edu/~jos/mdft/Round_Off_Error_Variance.html) —
  de onde vem o LSB²/12.
- S. Lipshitz, R. Wannamaker, J. Vanderkooy, ["Quantization and Dither: A Theoretical Survey" (JAES, 1992)](https://hajim.rochester.edu/ece/sites/zduan/teaching/ece472/reading/Lipshitz_1992.pdf) —
  a referência sobre dither: porque é que o TPDF torna os dois primeiros momentos do erro independentes do sinal.
- R. Wannamaker, ["The Theory of Dithered Quantization" (tese, Univ. of Waterloo)](https://uwspace.uwaterloo.ca/handle/10012/202) — o tratamento completo.
- [juce::Random — documentação](https://docs.juce.com/master/classjuce_1_1Random.html) — o gerador usado.
- A fórmula 6.02·N + 1.76 dB é derivada passo a passo nos tutoriais MT-001 e MT-229 da Analog Devices
  (não consegui abrir o site da Analog Devices durante a verificação das fontes; procura "MT-001" em analog.com).
