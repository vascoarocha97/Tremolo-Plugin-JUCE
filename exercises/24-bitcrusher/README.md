# 24 · Bitcrusher

> **Módulo 6 — Distorção** · ⏱ ~30 min · Visualização: [Bitcrusher](http://localhost:5173/#/bitcrush)

## Objetivos

- Usar a **quantização** (exercício 04) e a **redução de sample rate** como efeito.
- Implementar um **sample & hold** com um acumulador de fase (fatores fracionários).
- Ligar o resultado ao **aliasing** (exercício 03): aqui é desejado.

## Teoria

### Redução de bits

Quantizar para poucos bits (2–8) é o som "8-bit" de consolas antigas: o erro de quantização deixa de ser
ruído e passa a ser distorção (exercício 04). Usas o teu `Quantiser::quantise` — o exercício 24 liga ao
código do exercício 04.

### Redução de sample rate: sample & hold

Para simular um sample rate `fs / fator` sem mudar o sample rate real, segura-se cada valor durante
`fator` amostras:

```
fase += 1/fator por amostra;   quando fase >= 1: fase −= 1 e captura uma amostra nova
```

Com fator 4: captura, repete, repete, repete, captura... Com fator 2.5 a fase faz patamares de 2 e 3
amostras alternados (em média 2.5) — é o mesmo truque do acumulador de fase dos osciladores.

### Aliasing de propósito

Um conversor real filtra antes de baixar o sample rate. Aqui **não**: tudo o que está acima do novo
Nyquist (`fs / (2·fator)`) dobra. Com fator 8 a 48 kHz o novo sample rate é 6 kHz; um seno de 5 kHz
aparece a **1 kHz** (6 − 5). O teste mede exatamente isto. Há ainda as "imagens" criadas pelos
patamares (o espectro do sinal segurado repete-se em torno de múltiplos de 6 kHz) — é esse brilho
metálico que se ouve.

## O exercício

Em `Processor.cpp`: **TODO** `Bitcrusher::process`.

> Este exercício usa o teu `Quantiser` do exercício 04: se o 04 não estiver feito, os testes de níveis
> vão falhar aqui também.

## Armadilhas comuns

- **Contar com um inteiro** em vez da fase: não deixa usar fatores fracionários (e o controlo fica aos saltos).
- **Fase partilhada entre canais**: os canais saem desalinhados se tiverem blocos diferentes.
- **Esquecer que a primeira amostra também é capturada** (a fase começa em 1).

## Como verificar

```powershell
./learn check 24
./learn run 24     # 4–6 bits e fator 4–12 com o loop
```

## Para saber mais

- Julius O. Smith III, ["Digital Audio Resampling Home Page"](https://ccrma.stanford.edu/~jos/resample/) — como se muda o sample rate *bem*
  (com filtros) — o contrário do que o bitcrusher faz de propósito.
- Os exercícios 03 (aliasing) e 04 (quantização) deste curso, e as respetivas fontes.
