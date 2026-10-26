# 34 · Editor e attachments

> **Módulo 8 — Plugin real** · ⏱ ~45 min · Visualização: [Arquitetura de um plugin](http://localhost:5173/#/plugin-architecture)

## Objetivos

- Escrever um `juce::AudioProcessorEditor` com botões rotativos, uma ComboBox e um botão.
- Ligar cada componente a um parâmetro com os **attachments** do APVTS.
- Fazer o layout no `resized()` com `juce::Rectangle`.

## Teoria

### Duas threads, uma ponte

```
 message thread (GUI)                          audio thread
 ┌──────────────┐   attachment   ┌───────────┐   atomic   ┌──────────────┐
 │ Slider       │ ─────────────► │ parâmetro │ ─────────► │ processBlock │
 │              │ ◄───────────── │  (APVTS)  │            │              │
 └──────────────┘   (host muda)  └───────────┘            └──────────────┘
```

O editor **nunca** chama o DSP diretamente e o DSP nunca toca na GUI. A GUI muda parâmetros; o áudio
lê-os. Um attachment faz a ligação nos dois sentidos:

- arrastas o slider → o attachment chama `beginChangeGesture` / `setValueNotifyingHost` /
  `endChangeGesture` (o host grava automação e sabe quando começaste e acabaste);
- o host muda o parâmetro (automação, preset) → o attachment atualiza o slider (na message thread).

Também copia para o slider a gama, o skew, o passo, o texto do valor e o valor por omissão (duplo clique).

### Attachments do APVTS

```cpp
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
attachment = std::make_unique<SliderAttachment> (apvts, "time", timeSlider);
```

Há `SliderAttachment`, `ComboBoxAttachment` e `ButtonAttachment`. Duas regras de ordem:

1. **Destruição**: o attachment tem de morrer **antes** do componente. Os membros de uma classe são
   destruídos pela ordem inversa da declaração: declara o attachment **depois** do componente (o `Knob`
   do `Processor.h` já está assim).
2. **ComboBox**: o attachment traduz o valor do parâmetro para um índice de item — os itens têm de lá
   estar **antes** de criares o attachment.

### Layout

```
┌──────────────────────────────────────┐
│ Eco                                  │  título
│ Ganho  Tempo  Feedback  Tom  Mistura │  5 colunas: etiqueta
│  (o)    (o)     (o)     (o)    (o)   │             + slider rotativo
│ [Mono      v]            [x] Bypass  │  linha de baixo
└──────────────────────────────────────┘
```

No `resized()`, parte-se `getLocalBounds()` com `removeFromTop`, `removeFromBottom`, `removeFromLeft`,
`reduced`. Cada chamada devolve o pedaço retirado e encolhe o retângulo original — é a forma idiomática
de fazer layouts no JUCE (também há `juce::FlexBox` e `juce::Grid`). `setSize` no fim do construtor
chama `resized()`.

O host mostra o teu editor porque `EditorEchoProcessor::hasEditor()` devolve `true`. No JUCE 9, o
`createEditor()` é privado: hosts chamam `createEditorAndMakeActive()`, e antes de apagarem o editor
chamam `editorBeingDeleted()`.

## O exercício

Em `Processor.cpp`: **TODO(1)** os `SliderAttachment`, **TODO(2)** a ComboBox (itens + attachment) e o
`ButtonAttachment`, **TODO(3)** o `resized()`.

## Armadilhas comuns

- **Attachment declarado antes do componente**: na destruição, o attachment tenta falar com um
  componente que já não existe (crash ao fechar a janela).
- **ComboBox sem itens quando o attachment é criado**: a caixa fica vazia ou no item errado.
- **Configurar a gama do slider à mão**: o attachment sobrepõe-na; e se não sobrepusesse, ficavam duas
  fontes de verdade.
- **Chamar o DSP a partir da GUI** (ex.: `onValueChange = [&] { engine.setTime(...) }`): corrida de dados
  com a thread de áudio.

## Como verificar

```powershell
./learn check 34
./learn run 34     # o teu editor aparece no lugar do editor genérico
```

## Para saber mais

- [Tutorial JUCE: Saving and loading your plug-in state (APVTS)](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) — inclui um editor com `SliderAttachment` e `ButtonAttachment`.
- [AudioProcessorValueTreeState::SliderAttachment — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1SliderAttachment.html) — e as classes irmãs para ComboBox e Button.
- [juce::AudioProcessorEditor — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorEditor.html) — tamanho, redimensionamento, ciclo de vida.
- [juce::Rectangle — documentação](https://docs.juce.com/master/classjuce_1_1Rectangle.html) — `removeFromTop` e companhia.
- [Tutorial JUCE: Code your first audio plug-in](https://juce.com/tutorials/tutorial_code_basic_plugin/) — o editor mais simples possível, sem APVTS.
