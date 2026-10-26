# 35 · Plugin VST3

> **Módulo 8 — Plugin real** · ⏱ ~60 min · Visualização: [Arquitetura de um plugin](http://localhost:5173/#/plugin-architecture)

## Objetivos

- Compilar o eco dos exercícios 32–34 como **VST3** (e Standalone) com `juce_add_plugin`.
- Responder ao host: layouts de canais, cauda, parâmetro de bypass.
- Implementar um bypass **sem cliques**.
- Passar uma mini-validação ao estilo do [pluginval](https://github.com/Tracktion/pluginval).

## Teoria

### Do processador ao plugin

Até aqui o processador vivia no nosso standalone. Um plugin é o mesmo `juce::AudioProcessor` embrulhado
pelo **wrapper** do formato (VST3, AU, AAX...), que o JUCE compila a partir de `juce_audio_plugin_client`.
No `CMakeLists.txt` deste exercício:

```cmake
learn_add_plugin(NUMBER 35 PRODUCT_NAME "Learn JUCE Eco" PLUGIN_CODE Lj35 SOURCES ...)
```

`learn_add_plugin` (em `cmake/LearnExercise.cmake`) chama `juce_add_plugin` com `FORMATS VST3 Standalone`,
um código de fabricante e um código de plugin (4 caracteres cada — identificam o plugin nos projetos; não
os mudes depois de publicar). O wrapper chama `createPluginFilter()` (em `PluginEntry.cpp`) para criar o
processador. Depois de compilar o VST3, o JUCE **carrega-o** para gerar o `moduleinfo.json` — se o plugin
não arrancar, a compilação falha (uma primeira validação grátis).

### Layouts de canais

O host pergunta "aceitas este layout?" com vários layouts (`isBusesLayoutSupported`) e usa um dos que
aceitares. Um eco faz sentido em mono → mono e estéreo → estéreo. Recusar o resto (5.1, mono → estéreo,
bus desligado) é melhor do que aceitar e fazer algo estranho.

### Cauda

Quando a entrada para, um eco continua a soar. `getTailLengthSeconds()` diz ao host quanto tempo — usado,
por exemplo, para não cortar o fim num *bounce*/exportação, ou para decidir quando pode parar de chamar
o plugin. Com feedback `fb`, o eco k tem amplitude `fb^k` e fica abaixo de −60 dB quando
`k = ln(0.001) / ln(fb)`.

### Bypass do host

Os DAWs têm um botão de bypass próprio. `getBypassParameter()` diz ao host "o meu bypass é este
parâmetro": o botão do DAW e o do plugin ficam o mesmo, e o plugin decide **como** fazer bypass.

### Bypass sem cliques

Um bypass instantâneo salta do sinal com eco para o sinal seco de uma amostra para a outra: clique. Faz-se
um crossfade de ~20 ms (`SmoothedValue`, exercício 08):

```
saída = (1 − a) · eco + a · seco        a: 0 → 1 em 20 ms
```

Para isso guarda-se uma cópia do seco (`dryBuffer`, alocado no `prepareToPlay`). O eco continua a correr
durante o bypass: assim, ao desligar o bypass, a delay line tem o som recente e não ecos "velhos".
(Também se pode parar de processar em bypass para poupar CPU — mas então é preciso limpar o estado.)

### Blocos maiores do que o prometido

`prepareToPlay (sr, maxBlock)` promete um tamanho máximo, mas alguns hosts mandam blocos maiores. O
`processBlock` (já feito) processa aos pedaços de no máximo `dryBuffer.getNumSamples()`, com "vistas"
`AudioBuffer` sobre os mesmos dados (não copiam nem alocam). A mini-validação testa isto.

### Latência

Este eco não tem latência. Um plugin com oversampling (exercício 26) ou *lookahead* tem: chama
`setLatencySamples (n)` e o DAW atrasa as outras pistas o mesmo (*plugin delay compensation*).

## O exercício

Em `Processor.cpp`: **TODO(1)** `isBusesLayoutSupported`, **TODO(2)** `getTailLengthSeconds`, **TODO(3)**
`getBypassParameter`, **TODO(4)** o crossfade do bypass em `processChunk`, **TODO(5)** o editor
(`hasEditor`/`createEditor`). O estado e os programas vêm do teu exercício 33 (`StatefulEchoProcessor`).

## Experimentar num DAW

```powershell
./learn run 35     # compila VST3 + Standalone e abre o Standalone
```

O VST3 fica em `build\exercises\35-vst3-plugin\ex35_plugin_artefacts\Debug\VST3\Learn JUCE Eco.vst3`.
Para um DAW o encontrar, copia a pasta `.vst3` para `%LOCALAPPDATA%\Programs\Common\VST3` (pasta do
utilizador, não precisa de administrador) ou `C:\Program Files\Common Files\VST3`, ou acrescenta a pasta
de build às pastas de plugins do teu DAW. Em `Debug`, o plugin é mais lento e o JUCE avisa com
`jassert`s se algo estiver mal — é o que queres enquanto desenvolves.

## Armadilhas comuns

- **Aceitar todos os layouts** (`return true`): o host pode dar-te 6 canais e o teu código só trata 2.
- **Alocar o `dryBuffer` no `processBlock`**: aloca na thread de áudio.
- **Crossfade sem smoothing**: troca abrupta = clique.
- **`getTailLengthSeconds` com fb = 0**: `ln(0)` é −∞.

## Como verificar

```powershell
./learn check 35
./learn run 35
```

## Para saber mais

- [JUCE — CMake API (`juce_add_plugin`)](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md) — todas as opções (formatos, códigos, categorias, `COPY_PLUGIN_AFTER_BUILD`, `VST3_AUTO_MANIFEST`).
- [juce::AudioProcessor — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html) — `isBusesLayoutSupported`, `getTailLengthSeconds`, `getBypassParameter`, `setLatencySamples`.
- [juce::AudioChannelSet — documentação](https://docs.juce.com/master/classjuce_1_1AudioChannelSet.html) — mono, estéreo, surround.
- [pluginval (Tracktion)](https://github.com/Tracktion/pluginval) — o validador a sério; os testes deste exercício imitam parte do que ele faz. Vale a pena correr no teu VST3.
- [Steinberg — localização dos plugins VST3](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html) — onde os hosts procuram plugins em cada sistema.
- [Tutorial JUCE: Create a basic audio plug-in](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/) — a mesma ideia com o Projucer (aqui usamos CMake).
