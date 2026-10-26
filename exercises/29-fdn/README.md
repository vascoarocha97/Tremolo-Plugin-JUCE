# 29 · Feedback Delay Network (FDN)

> **Módulo 7 — Reverb** · ⏱ ~60 min · Visualização: [FDN](http://localhost:5173/#/fdn)

## Objetivos

- Implementar uma FDN de 8 linhas com uma matriz de feedback **ortogonal** (Householder).
- Perceber porque é que "ortogonal" quer dizer "não ganha nem perde energia".
- Controlar o decaimento por linha com a fórmula do exercício 28.

## Teoria

### De 8 combs separados para uma rede

Na reverb de Schroeder, os 8 combs são independentes: o eco que entra no comb 3 fica no comb 3 para
sempre. Numa **FDN** (Gerzon 1971, Stautner & Puckette 1982, Jot & Chaigne 1991), as saídas das N linhas são
**misturadas** antes de voltarem às entradas:

```
         ┌────────────────────────────────────────────┐
         │                                            │
entrada ─┴─► [ linhas 1..N, atrasos D₁..D_N ] ─► saídas ─┬─► (mistura L/R) ─► saída
         ▲                                              │
         └── ganhos g_i ◄── matriz A (N×N) ◄────────────┘
```

Cada eco "visita" várias linhas: um eco que saiu da linha 3 entra em todas, e na volta seguinte cada um
desses ecos volta a espalhar-se. O número de ecos cresce muito mais depressa do que em combs paralelos.

### Matriz ortogonal = sem ganho de energia

Se A for **ortogonal** (Aᵀ·A = I), então ‖A·v‖ = ‖v‖: a mistura não acrescenta nem tira energia.
Com todos os g_i = 1 a rede é **sem perdas** — a energia guardada nas linhas fica constante para sempre
(é um dos testes). Com g_i < 1, decai de forma controlada. Uma matriz qualquer podia amplificar certas
combinações e a rede explodia mesmo com g < 1.

### Householder

A matriz de Householder N×N (para o vetor [1, 1, ..., 1]) é:

```
A = I − (2/N) · 1·1ᵀ        →        A·v = v − (2/N)·(Σ v)·[1, 1, ..., 1]
```

É ortogonal, simétrica e a sua própria inversa (A·A = I). E custa **O(N)**: uma soma e N subtrações, em
vez de N² multiplicações. Mistura "um pouco" (cada saída é a própria entrada mais uma fração da soma);
o artigo da Signalsmith explica porque é que, *dentro* do ciclo de feedback, misturar de menos é melhor
do que misturar demais.

### Decaimento por linha

Cada linha i tem o seu ganho g_i = 0.001^(D_i / (T60 · fs)) — exatamente o do exercício 28. Assim, ao fim
de T60 segundos, **qualquer** caminho pela rede perdeu 60 dB, independentemente das linhas por onde passou.

### Entrada e saída

- **Entrada**: o mesmo sinal mono em todas as linhas (×1/√N).
- **Saída**: L e R são duas combinações diferentes das N saídas (sinais `+ − + − ...` e `+ + − − ...` —
  duas linhas de uma matriz de Hadamard), ×1/√N. Combinações ortogonais ⇒ L e R descorrelacionados.

### Atrasos

100–200 ms, todos diferentes e sem razões simples entre si (como no exemplo da Signalsmith). Atrasos
longos dão uma reverb "grande" — e um início pouco denso: nos primeiros 100 ms não há nada, e depois só
ecos isolados. O teste de densidade mostra isso; o **exercício 30** resolve-o com um difusor.

## O exercício

Em `Processor.cpp`: **TODO(1)** `householder`, **TODO(2)** os ganhos em `setDecaySeconds`,
**TODO(3)** `processFrame`. O `processSample` (entrada mono / saída estéreo) e o `FdnReverb` já estão feitos.

## Armadilhas comuns

- **Misturar no sítio enquanto ainda lês**: lê as N saídas todas primeiro; só depois mistura e escreve.
- **Aplicar a matriz à entrada em vez de ao feedback**.
- **Esquecer o fator 2/N** na Householder: deixa de ser ortogonal (o teste de energia apanha).
- **Ganhos calculados antes de as linhas existirem**: o `prepare` chama `setDecaySeconds` depois de
  criar as linhas por isso mesmo.

## Como verificar

```powershell
./learn check 29
./learn run 29     # compara com o 28 em "Impulsos": menos "flutter" metálico, início mais esparso
```

## Para saber mais

- Julius O. Smith III, ["Feedback Delay Networks (FDN)" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Feedback_Delay_Networks_FDN.html) — a estrutura em forma matricial, e ["History of FDNs for Artificial Reverberation"](https://ccrma.stanford.edu/~jos/pasp/History_FDNs_Artificial_Reverberation.html) — Gerzon (1971), Stautner & Puckette (1982), Jot.
- Julius O. Smith III, ["Householder Feedback Matrix" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Householder_Feedback_Matrix.html) — a matriz deste exercício e porque é eficiente.
- Julius O. Smith III, ["Achieving Desired Reverberation Times" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Achieving_Desired_Reverberation_Times.html) — o ganho por linha a partir do RT60.
- Geraint Luff (Signalsmith Audio), ["Let's Write A Reverb"](https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/) — FDN multicanal com Householder, explicada passo a passo (base dos exercícios 29 e 30).
- J.-M. Jot, A. Chaigne, "Digital Delay Networks for Designing Artificial Reverberators", 90.ª Convenção AES, 1991 — o artigo que tornou a FDN o padrão.
