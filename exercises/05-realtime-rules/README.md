# 05 · Regras de tempo-real

> **Módulo 1 — Fundamentos** · ⏱ ~25 min · Visualização: [Tempo-real e deadlines](http://localhost:5173/#/realtime)

## Objetivos

- Perceber porque é que a thread de áudio tem regras diferentes do resto do programa.
- Reconhecer as três violações mais comuns: **alocar memória, bloquear (locks) e I/O**.
- Passar dados entre a thread de áudio e a GUI sem locks: `std::atomic` e uma FIFO lock-free.

## Teoria

### Um deadline em cada bloco

A placa de som consome um bloco enquanto tu preparas o próximo. Com 64 amostras a 48 kHz tens
**1.33 ms** para devolver o bloco — sempre, não "em média". Se uma única chamada a `processBlock`
demorar mais, a placa fica sem dados: ouve-se um *click* ou um corte (*dropout*). Em jogos, uma frame
atrasada é um soluço visual; em áudio é um defeito audível.

Por isso, na thread de áudio não interessa o tempo médio, interessa o **pior caso**. Qualquer
operação cujo tempo não é limitado é proibida.

### As regras (Ross Bencina, "Time waits for nothing")

No caminho do áudio, **não**:

1. **Alocar ou libertar memória** (`new`/`delete`, `malloc`, `std::vector::push_back`/`resize` quando
   cresce, criar `std::string`/`juce::String`, `std::function` com capturas grandes...). O allocator
   pode usar locks internos, pedir páginas ao sistema operativo ou fazer trabalho de arrumação.
2. **Bloquear em locks** (`std::mutex`, `juce::CriticalSection`, `ScopedLock`). Se a GUI tiver o lock,
   a thread de áudio — com prioridade máxima — fica à espera de uma thread de prioridade normal, que
   pode ser interrompida por outras (*priority inversion*).
3. **Fazer I/O**: ficheiros, consola, rede, `DBG`/`juce::Logger` (que também formatam texto, ou seja,
   alocam).
4. **Chamar código de tempo desconhecido** (bibliotecas que não conheces, chamadas ao sistema) ou
   **esperar por outras threads**.

### Então como se fala com a GUI?

- **Um valor de cada vez** (um nível, um parâmetro): `std::atomic<float>`. Para `float` é lock-free
  em todas as plataformas que interessam (`std::atomic<float>::is_always_lock_free`). A GUI lê o
  valor mais recente; não precisa de os ver todos.
- **Uma sequência de valores** (histórico, amostras para um osciloscópio): uma **FIFO lock-free** de
  um produtor e um consumidor, com memória fixa reservada antes. No JUCE: `juce::AbstractFifo`
  gere os índices de leitura/escrita de um buffer que tu crias; o `write(n)`/`read(n)` devolvem até
  dois blocos contíguos (por causa do *wrap* circular). Se a FIFO encher, **descarta-se** o valor:
  a thread de áudio nunca espera pela GUI.
- **Toda a memória** é reservada no `prepareToPlay` (ou no construtor), nunca no `processBlock`.

O próprio standalone deste curso segue estas regras (vê `shared/host/source/HostAudioEngine.cpp`:
os medidores usam `std::atomic` e o osciloscópio usa uma `AbstractFifo`).

## O exercício

`PeakMeterProcessor` mede o pico de cada canal em cada bloco. A versão em `Processor.h/.cpp`
**funciona** — mas usa um `std::mutex` partilhado com a GUI, faz `resize`/`push_back` num
`std::vector` e escreve no log com `DBG` em cada bloco.

Reescreve a parte `private` e as implementações (a interface pública mantém-se) de forma a que:

- `processBlock` não aloque, não bloqueie e não faça I/O;
- `getPeak` leia um `std::atomic<float>` por canal (`std::array<std::atomic<float>, maxChannels>`);
- o histórico use `juce::AbstractFifo` + `std::array<float, historyCapacity>`.

Os TODO em `Processor.cpp` dizem onde mexer.

> **Limitação honesta dos testes:** os testes detetam alocações (o heap de debug do MSVC avisa-os de
> cada `malloc`/`new`), mas **não conseguem provar que não há locks**. Um `std::mutex` sem disputa
> não aloca nem demora. Revê tu o código (ou pede ao `/tutor 05`).

## Armadilhas comuns

- **`reserve()` no `prepareToPlay` e manter o `push_back`**: deixa de alocar, mas continua com o lock
  e o `std::vector` cresce sem limite se a GUI não ler.
- **Um `std::atomic` para o histórico inteiro**: atómico é para valores pequenos (`float`, `int`,
  ponteiros); um array grande dentro de `std::atomic` usa um lock escondido.
- **`AbstractFifo` com dois produtores ou dois consumidores**: só é segura com um de cada.
- **Esquecer os canais que deixaram de existir**: um bloco mono depois de um estéreo.

## Como verificar

```powershell
./learn check 05
./learn run 05     # o editor mostra getPeak() e o gráfico de readHistory() lidos na thread da GUI
```

## Para saber mais

- Ross Bencina, ["Real-time audio programming 101: time waits for nothing"](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) —
  o texto de referência sobre estas regras e porquê.
- Dave Rowland & Fabian Renn-Giles, ["Real time 101" (Meeting C++ 2019)](https://www.youtube.com/watch?v=ndeN983j_GQ) —
  a mesma talk da ADC 2019 (partes I e II): custos de chamadas ao sistema, alocações, *priority inversion*, atomics e FIFOs.
- [JUCE: "Real-time programming in audio development"](https://juce.com/posts/real-time-programming-in-audio-development/) —
  o artigo do blog do JUCE sobre o mesmo tema.
- Timur Doumler, ["Using locks in real-time audio processing, safely"](https://timur.audio/using-locks-in-real-time-audio-processing-safely) —
  quando é que (com muito cuidado) um lock pode ser aceitável, e porque é que um `std::mutex` não é.
- [juce::AbstractFifo — documentação](https://docs.juce.com/master/classjuce_1_1AbstractFifo.html) — a FIFO lock-free do JUCE.
