# 23 · Fuzz

> **Módulo 6 — Distorção** · ⏱ ~40 min · Visualização: [Waveshaping e harmónicos](http://localhost:5173/#/waveshaping)

## Objetivos

- Montar uma cadeia de distorção completa, como num pedal: filtrar → distorcer → limpar → tom → volume.
- Escrever um **DC blocker** e perceber porque é obrigatório depois de um clip assimétrico.
- Perceber porque é que a **ordem** dos blocos importa.

## Teoria

### A cadeia

```
entrada -> passa-alto 100 Hz -> ganho (até +50 dB) -> clip assimétrico -> DC blocker -> passa-baixo (tom) -> volume
```

1. **Passa-alto antes de distorcer.** Graves fortes, quando distorcidos, criam intermodulação (somas e
   diferenças de frequências) que soa a "lama". Tirar os graves antes deixa a distorção mais definida.
   Muitos pedais têm condensadores de entrada pequenos que fazem exatamente isto.
2. **Ganho enorme + clip.** Com +40 dB, quase todo o sinal está na zona de saturação: o fuzz transforma
   tudo numa onda quase quadrada. A curva assimétrica do exercício 22 dá harmónicos pares.
3. **DC blocker.** O clip assimétrico desloca a média (exercício 22). O DC blocker clássico:

   ```
   y[n] = x[n] − x[n−1] + R·y[n−1]          R = 1 − 2π·fc/fs   (fc ≈ 10 Hz -> R ≈ 0.9987 a 48 kHz)
   ```

   É um diferenciador (`x − x₁`, que anula o DC) seguido de um "integrador com fugas" (`R·y₁`) que repõe
   tudo o que está acima de fc. Um zero em z = 1 e um polo logo ao lado, em z = R.
4. **Tom.** A distorção cria muitos harmónicos agudos ("fizz"); um passa-baixo de 1.ª ordem com corte
   entre 500 Hz e 10 kHz doma-os. A frequência varia **exponencialmente** com o botão (o ouvido percebe
   frequências em escala logarítmica).
5. **Volume.** Com tanto ganho, a saída precisa de ser baixada.

Para os filtros 1 e 4 usamos o `juce::dsp::FirstOrderTPTFilter` (um filtro de 1.ª ordem TPT, a mesma
família do SVF do exercício 16).

### Porque não tem oversampling?

Devia ter: um fuzz com +40 dB é o pior caso de aliasing. Este exercício foca a cadeia; o exercício 26
mostra a solução, e o projeto final (40) junta as duas coisas.

## O exercício

Em `Processor.cpp`: **TODO(1)–(2)** o `DcBlocker`, **TODO(3)** a cadeia em `Fuzz::process`. Usas a tua
`Shapers::asymmetric` do exercício 22.

## Armadilhas comuns

- **DC blocker antes do clip**: não adianta — o DC é criado pelo clip.
- **Tom antes do clip**: muda o carácter (é outra forma de pedal), mas não doma os harmónicos criados pelo clip.
- **`R` calculado com `float`** muito perto de 1: aqui chega, mas em filtros com cortes muito baixos a
  precisão de `float` começa a contar.

## Como verificar

```powershell
./learn check 23
./learn run 23     # loop ou "Dente de serra"; o tom ouve-se bem com o fuzz alto
```

## Para saber mais

- Julius O. Smith III, [*Introduction to Digital Filters* — "DC Blocker"](https://ccrma.stanford.edu/~jos/filters/DC_Blocker.html) —
  a fórmula, a resposta em frequência e a escolha de R.
- [juce::dsp::FirstOrderTPTFilter — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1FirstOrderTPTFilter.html).
- (não confirmado) A descrição dos condensadores de entrada como "passa-alto antes da distorção" é a
  explicação comum em análises de circuitos de pedais clássicos (Fuzz Face, Big Muff); não consegui abrir
  a análise da ElectroSmash durante a verificação das fontes.
