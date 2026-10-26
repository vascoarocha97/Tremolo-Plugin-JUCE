# 03 · Nyquist e aliasing

> **Módulo 1 — Fundamentos** · ⏱ ~20 min · Visualização: [Nyquist e aliasing](http://localhost:5173/#/nyquist-aliasing)

## Objetivos

- Saber qual é a frequência máxima que um sample rate representa (**Nyquist = fs/2**).
- Calcular para onde "dobra" uma frequência acima de Nyquist (**aliasing**).
- Ouvir e medir o aliasing de um oscilador ingénuo — o mesmo problema que vai voltar na distorção
  (módulo 6).

## Teoria

### O teorema da amostragem

Um sinal amostrado a *fs* só consegue representar sem ambiguidade frequências **abaixo de fs/2**
(a frequência de Nyquist). A 48 kHz, isso é 24 kHz — acima do que ouvimos, e é por isso que estes
sample rates foram escolhidos.

### O que acontece acima de Nyquist

As amostras de um seno a *f* e de um seno a *fs − f* são as mesmas (a menos do sinal/fase). O mesmo
para *f + k·fs* para qualquer inteiro *k*. Ou seja, depois de amostrar, **não há forma de distinguir
estas frequências**: tudo o que estava acima de Nyquist aparece "espelhado" abaixo. Isto é o
*aliasing* (um "pseudónimo": uma frequência disfarçada de outra).

Para calcular a frequência ouvida: reduz-se *f* a um período [0, fs) e, se ficar acima de fs/2,
espelha-se em torno de fs/2. Exemplos a 48 kHz: 30 kHz → 18 kHz; 47 kHz → 1 kHz; 50 kHz → 2 kHz.

O teste "o resultado bate com o que um seno amostrado realmente faz" verifica isto diretamente:
calcula as amostras de um seno acima de Nyquist e compara com as do seno à frequência que a tua
função devolve.

### Porque é que isto importa num plugin

Num conversor analógico-digital há um filtro analógico antes da amostragem que remove o que está
acima de Nyquist. Mas **quando geras ou distorces som dentro do domínio digital, ninguém filtra por
ti**. Uma forma de onda com uma descontinuidade — como a rampa de um dente de serra que volta de
+1 a −1 numa amostra — tem harmónicos até ao infinito (amplitude ≈ 2/(π·k) para o harmónico *k*).
Os que passam de Nyquist dobram para frequências que **não são múltiplos da fundamental**: soam
desafinados, metálicos, e quando sobes a nota ouves tons a descer ("birdies").

As soluções reais são osciladores com banda limitada (BLIT, PolyBLEP, wavetables com mipmaps) e,
para não-linearidades, **oversampling** (exercício 26).

## O exercício

Em `Processor.cpp`:

1. **TODO(1)** `Nyquist::aliasedFrequency()`.
2. **TODO(2)** `NaiveSawOscillator::updateIncrement()` — a fase deste oscilador vai de 0 a 1 (ciclos), não de 0 a 2π.
3. **TODO(3)** `NaiveSawOscillator::processBlock()` — a rampa de −1 a +1.

O teste mais interessante gera um dente de serra de 2.5 kHz a 48 kHz e verifica que há energia
**exatamente** onde a tua fórmula prevê que os harmónicos 10–19 vão parar (23 kHz, 20.5 kHz, ...,
500 Hz) — frequências que um dente de serra ideal nunca teria.

## Armadilhas comuns

- **Frequências negativas**: um seno a −f é o mesmo que a +f com a fase invertida; usa o valor absoluto.
- **Esquecer o *wrap* de vários períodos**: 100 kHz a 48 kHz dá 4 kHz (dobra mais do que uma vez).
- **Confundir ciclos com radianos** no incremento do dente de serra.

## Como verificar

```powershell
./learn check 03
./learn run 03
```

No standalone: sobe a frequência devagar a partir de 2 kHz. No espectro aparecem riscas que descem
enquanto a nota sobe. Compara com a fonte "Seno" do host (sem harmónicos, sem aliasing).

## Para saber mais

- Julius O. Smith III, [*Mathematics of the DFT* — "Sampling Theorem"](https://ccrma.stanford.edu/~jos/mdft/Sampling_Theorem.html) —
  o enunciado e a prova do teorema da amostragem.
- Julius O. Smith III, ["Sampled Sinusoids"](https://ccrma.stanford.edu/~jos/mdft/Sampled_Sinusoids.html) —
  porque é que frequências acima de Nyquist são indistinguíveis de outras abaixo.
- T. Stilson e J. Smith, ["Alias-Free Digital Synthesis of Classic Analog Waveforms" (ICMC 1996)](https://ccrma.stanford.edu/~stilti/papers/blit.pdf) —
  o artigo clássico sobre o problema do dente de serra ingénuo e a solução BLIT.
- Martin Finke, ["PolyBLEP Oscillator"](https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/) —
  uma técnica simples e muito usada para reduzir o aliasing de osciladores (blog de plugins em C++).
