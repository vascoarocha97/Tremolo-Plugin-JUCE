# Uma única biblioteca estática com todos os módulos JUCE usados no curso.
#
# Porquê: cada alvo que liga diretamente a juce::juce_xxx compila de novo o código dos módulos.
# Com 40 exercícios (testes + standalone) isso seria compilar o JUCE dezenas de vezes.
# Este é o padrão descrito por um developer do JUCE no fórum
# (https://forum.juce.com/t/using-cmake-flow-to-build-libraries-android-apps/40492/2):
#   - os módulos são ligados PRIVATE a uma biblioteca estática;
#   - as definições e include dirs dessa biblioteca são reexportadas como INTERFACE;
#   - TODOS os alvos ligam a esta biblioteca e NUNCA diretamente a módulos JUCE
#     (senão o mesmo módulo entra duas vezes no binário, possivelmente com flags diferentes).
#
# Exceção: os alvos de plugin (juce_add_plugin, exercícios 35 e 40) — ver DECISIONS.md, ponto 5.

add_library(learn_juce_modules STATIC)

target_link_libraries(learn_juce_modules
    PRIVATE
        juce::juce_audio_utils   # puxa audio_processors, audio_devices, audio_formats, gui_*, ...
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(learn_juce_modules
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
    INTERFACE
        $<TARGET_PROPERTY:learn_juce_modules,COMPILE_DEFINITIONS>)

# Os ficheiros fonte (e os textos em português) estão em UTF-8.
target_compile_options(learn_juce_modules PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_include_directories(learn_juce_modules
    INTERFACE
        $<TARGET_PROPERTY:learn_juce_modules,INCLUDE_DIRECTORIES>)

set_target_properties(learn_juce_modules PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    FOLDER "deps")
