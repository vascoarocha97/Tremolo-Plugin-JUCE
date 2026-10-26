# 02 · Oscilador sinusoidal

> **Módulo 1 — Fundamentos** · ⏱ ~15 min · Visualização: [Onda contínua vs. amostrada](http://localhost:5173/#/sampling)

## Objetivos

- Perceber o **sample rate**: quantas amostras representam um segundo.
- Gerar um seno com um **acumulador de fase** que continua certo entre blocos.
- Perceber porque é que o `prepareToPlay()` existe: é lá que ficas a saber o sample rate.

## Teoria

### Sample rate

O sample rate *fs* é o número de amostras por segundo. Valores comuns: 44 100 Hz (CD), 48 000 Hz
(vídeo e a maioria das interfaces), 88 200 / 96 000 Hz. **Nunca assumas um valor**: o host diz-te
qual é no `prepareToPlay (sampleRate, ...)` e pode mudar entre sessões.

Uma amostra dura 1/*fs* segundos. A 48 kHz, um bloco de 480 amostras são 10 ms.

### Um seno amostrado

Um seno contínuo é `x(t) = A · sin(2π·f·t)`. Amostrar é avaliar em `t = n / fs`:

```
x[n] = A · sin(2π · f · n / fs)
```

Em vez de calcular `2π·f·n/fs` a partir de `n` (que cresce sem parar), guarda-se a **fase** e
soma-se um **incremento** constante em cada amostra:

```
incremento = 2π · f / fs          (radianos por amostra)
fase[n+1]  = fase[n] + incremento
```

Porquê um acumulador:

1. **Mudanças de frequência sem saltos.** Se a frequência muda entre dois blocos, a fase continua
   do mesmo sítio e só a velocidade muda. Com `sin(2π·f·n/fs)` a fase saltaria (um *click*).
2. **Continuidade entre blocos.** O host chama `processBlock` com blocos de tamanhos variáveis;
   a fase é **estado** do objeto e sobrevive entre chamadas.
3. **Precisão.** Mantém-se a fase em [0, 2π) subtraindo 2π quando passa. Um `float` tem 24 bits de
   mantissa: perto de 2π a distância entre `float`s consecutivos é ~5·10⁻⁷ rad, mas se a fase
   crescesse até 100 000 rad essa distância seria ~0.008 rad — já se ouve como ruído/desafinação.
   Por isso a fase aqui é `double` **e** é mantida no intervalo.

Subtrair 2π (em vez de `std::fmod` em cada amostra) chega porque o incremento é sempre menor do que
2π enquanto `f < fs`.

## O exercício

Em `Processor.cpp`:

1. **TODO(1)** `prepareToPlay()`: guarda o sample rate, recalcula o incremento e faz reset à fase.
2. **TODO(2)** `updateIncrement()`: o incremento em radianos por amostra (protege o caso `sampleRate == 0`).
3. **TODO(3)** `processBlock()`: para cada amostra, `amplitude · sin(fase)` em todos os canais; avança a fase e mantém-na em [0, 2π).

## Armadilhas comuns

- **44 100 escrito no código.** O teste usa 44.1, 48 e 96 kHz.
- **Fase local ao `processBlock`.** Recomeça a 0 em cada bloco → um *click* a cada bloco.
- **Avançar a fase uma vez por canal.** Num buffer estéreo a frequência dobra e os canais ficam diferentes.
- **Não fazer *wrap* da fase.** Funciona durante uns segundos; depois perde precisão.

## Como verificar

```powershell
./learn check 02
./learn run 02     # o standalone usa o teu oscilador como fonte (ignora a entrada)
```

No espectro do standalone deves ver uma única risca na frequência escolhida.

## A versão JUCE

`juce::dsp::Oscillator<float>` faz isto (com uma função qualquer, não só seno, e opcionalmente com
uma tabela de consulta), na convenção `prepare/process` do módulo `juce_dsp` que vais usar a partir
do exercício 09.

## Para saber mais

- Julius O. Smith III, [*Mathematics of the DFT* — "Sinusoids"](https://ccrma.stanford.edu/~jos/mdft/Sinusoids.html) e
  ["Sampled Sinusoids"](https://ccrma.stanford.edu/~jos/mdft/Sampled_Sinusoids.html) — amplitude, frequência, fase, e o que muda ao amostrar.
- [Tutorial JUCE: Build a sine wave synthesiser](https://juce.com/tutorials/tutorial_sine_synth/) — o mesmo acumulador de fase dentro de um sintetizador JUCE.
- [juce::dsp::Oscillator — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oscillator.html) — a versão do JUCE.
