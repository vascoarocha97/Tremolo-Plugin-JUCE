# O pedal é escrito do zero: podes criar os ficheiros .h/.cpp que quiseres NESTA pasta (fora de
# tests/) — entram automaticamente no build (glob). Depois de criares um ficheiro novo, o
# ./learn check 40 reconfigura sozinho.
file(GLOB pedal_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER pedal_sources EXCLUDE REGEX "PluginEntry\.cpp$")

# Testes (caixa preta: só usam a interface juce::AudioProcessor e os IDs dos parâmetros).
learn_add_exercise(
    NUMBER       40
    WEB_PAGE     pedal
    DSP_SOURCES  ${pedal_sources}
    TEST_SOURCES tests/ProcessorTests.cpp
    NO_APP)

# O plugin: VST3 + Standalone (./learn run 40).
learn_add_plugin(
    NUMBER       40
    PRODUCT_NAME "Learn JUCE Pedal"
    PLUGIN_CODE  Lj40
    SOURCES      ${pedal_sources} PluginEntry.cpp)
