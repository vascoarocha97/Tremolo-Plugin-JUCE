# SOURCES — fontes usadas na plataforma

Todas as lições explicam por palavras próprias e ligam às fontes. Não há excertos longos copiados.
Os links foram verificados (HTTP 200) na data indicada em cada secção.

## Referências do JUCE (versão 9.0.4)

| Fonte | O que cobre |
|---|---|
| [Repositório JUCE, tag 9.0.4](https://github.com/juce-framework/JUCE/tree/9.0.4) | Código-fonte e headers (a documentação mais exata desta versão). Lido em `build/_deps/juce-src/modules`. |
| [BREAKING_CHANGES.md](https://github.com/juce-framework/JUCE/blob/9.0.4/BREAKING_CHANGES.md) | O que mudou entre versões (ex.: `createEditor()` privado desde 8.0.13). |
| [docs/CMake API.md](https://github.com/juce-framework/JUCE/blob/9.0.4/docs/CMake%20API.md) | `juce_add_gui_app`, `juce_add_plugin`, opções de CMake. |
| [Documentação da API](https://docs.juce.com/master/) | Páginas por classe (gerada a partir dos headers do branch `master`). |
| [Tutoriais oficiais](https://juce.com/learn/tutorials/) | Tutoriais por tema (plugins, DSP, GUI). |
| [Fórum do JUCE](https://forum.juce.com/) | Armadilhas práticas conhecidas. |
| [Thread: biblioteca estática com os módulos JUCE](https://forum.juce.com/t/using-cmake-flow-to-build-libraries-android-apps/40492/2) | Padrão usado em `cmake/LearnJuceModules.cmake`. |

## Módulo 1 — Fundamentos (verificado a 2026-10-26)

| Fonte | O que cobre | Usado em |
|---|---|---|
| [juce::AudioBuffer](https://docs.juce.com/master/classjuce_1_1AudioBuffer.html) | Layout canais × amostras, flag "isClear" | 01 |
| [juce::AudioProcessor](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html) | `prepareToPlay`/`processBlock`, blocos de tamanho variável | 01, 05 |
| [Tutorial: Create a basic Audio/MIDI plugin, Part 1](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/) | Onde vivem `prepareToPlay`/`processBlock` | 01 |
| Julius O. Smith III, [*Mathematics of the DFT*, "Sampling Theory"](https://ccrma.stanford.edu/~jos/mdft/Sampling_Theory.html) | Amostragem e teorema de Nyquist | 01, 02, 03 |

## Bibliotecas de terceiros

| Biblioteca | Licença | Uso |
|---|---|---|
| [JUCE 9.0.4](https://github.com/juce-framework/JUCE) | AGPLv3 / licença comercial JUCE 9 | Framework (descarregado pelo CMake, não incluído no repositório) |
| [Catch2 v3.16.1](https://github.com/catchorg/Catch2) | Boost Software License 1.0 | Testes (descarregado pelo CMake) |

Nenhum código de terceiros foi copiado para o repositório até agora.
