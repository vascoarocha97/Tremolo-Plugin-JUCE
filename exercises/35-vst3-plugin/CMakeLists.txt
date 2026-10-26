# Testes: como nos outros exercícios (com a biblioteca JUCE partilhada e os teus 32–34).
learn_add_exercise(
    NUMBER       35
    WEB_PAGE     plugin-architecture
    DSP_SOURCES  Processor.h Processor.cpp
    TEST_SOURCES tests/ProcessorTests.cpp
    DEPENDS      32 33 34
    NO_APP)

# O plugin: VST3 + Standalone (./learn run 35). As fontes dos exercícios 32–34 entram diretamente.
learn_add_plugin(
    NUMBER       35
    PRODUCT_NAME "Learn JUCE Eco"
    PLUGIN_CODE  Lj35
    SOURCES      Processor.cpp
                 PluginEntry.cpp
                 ../32-apvts-parameters/Processor.cpp
                 ../33-state-and-presets/Processor.cpp
                 ../34-editor-attachments/Processor.cpp)
