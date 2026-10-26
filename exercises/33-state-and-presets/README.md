# 33 · Estado e presets

> **Módulo 8 — Plugin real** · ⏱ ~45 min · Visualização: [Arquitetura de um plugin](http://localhost:5173/#/plugin-architecture)

## Objetivos

- Guardar e repor o estado do plugin (`getStateInformation` / `setStateInformation`).
- Validar dados que vêm de fora (projetos antigos, corrompidos, de outro plugin).
- Aplicar presets como o host faria (notificando host e GUI).

## Teoria

### O que o DAW grava

Quando gravas um projeto, o DAW pede a cada plugin um bloco de bytes (`getStateInformation`) e guarda-o
no ficheiro do projeto. Ao abrir, devolve-os (`setStateInformation`). O formato é **decisão nossa** — o
host não olha para dentro. Também é isto que o DAW usa para os seus próprios presets ("guardar preset do
plugin"), para copiar plugins entre pistas, e para o *undo* em alguns hosts.

Com o APVTS, o estado natural é a `ValueTree` dos parâmetros, guardada como XML:

```
apvts.copyState()           ValueTree (cópia, thread-safe)
    .createXml()            XmlElement  <PARAMETERS><PARAM id="time" value="350"/>...</PARAMETERS>
copyXmlToBinary (xml, mb)   bytes (o JUCE junta um cabeçalho e o tamanho)
```

e o caminho inverso: `getXmlFromBinary` → `ValueTree::fromXml` → `apvts.replaceState`.

XML tem vantagens: é legível (dá para depurar abrindo o ficheiro), e o formato é tolerante a parâmetros
a mais ou a menos.

### Dados de fora não são de confiança

`setStateInformation` pode receber:

- bytes que não são XML (projeto corrompido) → `getXmlFromBinary` devolve `nullptr`;
- XML de **outro** formato (outro plugin, ou um estado teu antigo noutro formato) → verifica a tag raiz;
- um estado de uma **versão antiga** do teu plugin, sem os parâmetros novos.

Este último é subtil. O `replaceState` do APVTS liga cada parâmetro ao filho correspondente da
`ValueTree`; um parâmetro sem filho **fica com o valor que tinha** (confirmado no código do JUCE 9.0.4,
`AudioProcessorValueTreeState::updateParameterConnectionsToChildTrees`). Num DAW que reutiliza a
instância do plugin, isso seria o valor do projeto anterior! A solução deste exercício: repor todos os
parâmetros no valor por omissão antes do `replaceState`.

### Presets

Um preset aqui é um nome mais **valores reais** de alguns parâmetros. Aplicar um preset = todos os
parâmetros no valor por omissão, depois os do preset. Usa-se `setValueNotifyingHost` (com o valor
convertido para 0..1) para que o host (automação, "modificado") e a GUI saibam da mudança.

O `StatefulEchoProcessor` (já feito) liga isto ao `juce::AudioProcessor`: estado → `EchoState`, e os
**programas** (a lista de presets que alguns hosts mostram) → `factoryPresets()`. O standalone mostra
uma caixa "Preset..." quando o processador tem programas, e grava o estado ao fechar.

## O exercício

Em `Processor.cpp`: **TODO(1)** `EchoState::save`, **TODO(2)** `EchoState::load`, **TODO(3)** `applyPreset`.

## Armadilhas comuns

- **Ler `apvts.state` diretamente** em vez de `copyState()`: não é thread-safe (o áudio pode estar a mexer).
- **Confiar nos bytes**: um `setStateInformation` com lixo não pode crashar nem deixar o plugin num estado meio carregado.
- **Esquecer os parâmetros em falta** num estado antigo (ver acima).
- **`setValue` em vez de `setValueNotifyingHost`**: o host e a GUI não ficam a saber.

## Como verificar

```powershell
./learn check 33
./learn run 33     # escolhe um preset, fecha, volta a abrir: o estado foi reposto
```

## Para saber mais

- [Tutorial JUCE: Saving and loading your plug-in state](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) — `copyState`/`replaceState` com XML, como aqui.
- [juce::AudioProcessor — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html) — `getStateInformation`, `setStateInformation`, `copyXmlToBinary`, `getXmlFromBinary`, programas.
- [juce::AudioProcessorValueTreeState — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html) — `copyState`, `replaceState`, `state`.
- [pluginval (Tracktion)](https://github.com/Tracktion/pluginval) — um dos testes que faz é precisamente gravar/repor estado com dados aleatórios.
