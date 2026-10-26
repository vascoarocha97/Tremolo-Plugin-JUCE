# 32 · Parâmetros com APVTS

> **Módulo 8 — Plugin real** · ⏱ ~40 min · Visualização: [Arquitetura de um plugin](http://localhost:5173/#/plugin-architecture)

## Objetivos

- Declarar os parâmetros de um plugin com `juce::AudioProcessorValueTreeState` (APVTS) e um `ParameterLayout`.
- Perceber **IDs estáveis**, **versões** (`ParameterID`), **gamas** e **skew** (`NormalisableRange`).
- Ler parâmetros no `processBlock` de forma segura entre threads (`std::atomic<float>*`).

## Teoria

### O módulo 8: um plugin de eco, peça a peça

| Exercício | Peça |
|---|---|
| **32** | parâmetros (este) |
| 33 | estado e presets (o que o DAW grava no projeto) |
| 34 | editor (GUI) ligado aos parâmetros |
| 35 | plugin VST3: layouts de canais, cauda, bypass do host |

O DSP (`EchoEngine`: eco com feedback, filtro no ciclo, ping-pong) já está feito — é o módulo 3. Aqui
interessa a **ponte** entre o mundo do host/GUI e o código de áudio.

Desta vez o processador do exercício **é** um `juce::AudioProcessor` completo: o standalone usa-o tal e qual.

### Parâmetros: o contrato com o host

Um parâmetro de plugin é visto pelo host como um número **normalizado entre 0 e 1**. É isso que o DAW
grava na automação. O plugin converte para o valor real (dB, ms, Hz) com uma `NormalisableRange`:

```
valor real = start + (end − start) · posição^(1/skew)   (com skew; sem skew, linear)
```

Um botão de tempo de 1 a 1000 ms linear teria metade do curso entre 500 e 1000 ms, onde o ouvido quase
não distingue nada. `setSkewForCentre (250)` faz com que o meio do curso seja 250 ms.

| ID | Nome | Tipo | Gama | Omissão | Notas |
|---|---|---|---|---|---|
| `gain` | Ganho | float | −24 … +12, passo 0.1 | 0 | label "dB" |
| `time` | Tempo | float | 1 … 1000, passo 0.1 | 350 | label "ms", centro 250 |
| `feedback` | Feedback | float | 0 … 0.95, passo 0.01 | 0.4 | |
| `tone` | Tom | float | 500 … 20000, passo 1 | 6000 | label "Hz", centro 3000 |
| `mix` | Mistura | float | 0 … 1, passo 0.01 | 0.3 | |
| `mode` | Modo | choice | "Mono", "Ping-pong" | 0 | |
| `bypass` | Bypass | bool | | false | |

### IDs e versões

O ID (`"time"`) fica gravado nos projetos dos utilizadores e na automação: **nunca o mudes** depois de
publicar o plugin. O `ParameterID { "time", 1 }` acrescenta uma *version hint*: quando, numa versão 2 do
plugin, acrescentares parâmetros, esses levam versão 2, depois 3... Segundo a documentação do JUCE, é
usada pelo formato **Audio Unit** (Logic, GarageBand) para ordenar os parâmetros e não estragar a
automação de projetos antigos. Não custa nada usá-la sempre — e o mesmo código pode um dia ser
compilado como AU.

### O APVTS

```cpp
juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMETERS", createParameterLayout() };
```

O APVTS cria os parâmetros (e regista-os no processador, como o `addParameter` dos exercícios anteriores),
guarda os valores numa `juce::ValueTree` (útil para o estado, exercício 33) e dá ferramentas para a GUI
(attachments, exercício 34).

### Ler parâmetros na thread de áudio

O host e a GUI mudam parâmetros noutras threads. `apvts.getRawParameterValue ("time")` devolve um
`std::atomic<float>*` com o valor **real** (não normalizado). Pede o ponteiro **uma vez** (construtor) e
no `processBlock` faz só `time->load()`: uma leitura atómica, sem locks e sem procurar strings.

Os valores lidos uma vez por bloco mudam "aos degraus": por isso o `EchoEngine` passa-os por
`SmoothedValue` (exercício 08).

## O exercício

Em `Processor.cpp`: **TODO(1)** `createParameterLayout`, **TODO(2)** os ponteiros no construtor,
**TODO(3)** `currentSettings` (ler os atomics), **TODO(4)** o bypass no `processBlock`.

## Armadilhas comuns

- **`getRawParameterValue` no `processBlock`**: procura por string a cada bloco. Guarda o ponteiro.
- **Confundir valor normalizado e real**: `param->getValue()` é 0..1; o atomic do APVTS é o valor real;
  `convertTo0to1` / `convertFrom0to1` convertem.
- **Mudar IDs ou a ordem/gama de parâmetros publicados**: projetos dos utilizadores deixam de abrir bem.
- **Ler `apvts.state` (a ValueTree) na thread de áudio**: não é thread-safe; usa os atomics.

## Como verificar

```powershell
./learn check 32
./learn run 32     # o host mostra os teus parâmetros num editor genérico
```

## Para saber mais

- [Tutorial JUCE: Saving and loading your plug-in state (APVTS)](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) — o APVTS passo a passo.
- [Tutorial JUCE: Adding plug-in parameters](https://juce.com/tutorials/tutorial_audio_parameter/) — parâmetros sem APVTS (o que fizeste nos wrappers dos exercícios anteriores).
- [juce::AudioProcessorValueTreeState — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html) — `ParameterLayout`, `getRawParameterValue`, `getParameter`.
- [juce::ParameterID — documentação](https://docs.juce.com/master/classjuce_1_1ParameterID.html) — o que é a *version hint*.
- [juce::NormalisableRange — documentação](https://docs.juce.com/master/classjuce_1_1NormalisableRange.html) — gamas, passo e `setSkewForCentre`.
- O exemplo oficial `examples/Plugins/DSPModulePluginDemo.h` (no repositório do JUCE) declara dezenas de parâmetros com este padrão.
