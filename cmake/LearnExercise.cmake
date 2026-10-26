# learn_add_exercise() — cria os três alvos de um exercício.
#
#   learn_add_exercise(
#       NUMBER        03
#       WEB_PAGE      nyquist-aliasing          (página da web app, opcional)
#       DSP_SOURCES   Processor.h Processor.cpp
#       TEST_SOURCES  tests/ProcessorTests.cpp
#       APP_SOURCES   app/ExerciseApp.cpp
#       [DEPENDS      18 22]                    (exercícios anteriores cujo código este usa)
#       [NO_APP])
#
# Alvos criados:
#   ex03_dsp    biblioteca estática com o código DSP (o que tu editas)
#   ex03_tests  executável Catch2; compila só o DSP + testes (rápido)
#   ex03_app    standalone para ouvir o resultado (fora do ALL: ./learn run 03)
#
# Todos ligam a learn_juce_modules, por isso o JUCE é compilado uma única vez.
#
# DEPENDS: o código DSP deste exercício usa o TEU código de um exercício anterior (ex.: o tremolo
# usa o LFO do 18). O header inclui-se com caminho relativo (#include "../18-lfo/Processor.h") e a
# biblioteca liga-se com $<LINK_ONLY:...> para que os include dirs dos dois exercícios (ambos com um
# Processor.h) não se misturem.

function(learn_add_exercise)
    set(options NO_APP)
    set(one_value_args NUMBER WEB_PAGE)
    set(multi_value_args DSP_SOURCES TEST_SOURCES APP_SOURCES DEPENDS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ARG_NUMBER)
        message(FATAL_ERROR "learn_add_exercise: falta NUMBER")
    endif()

    set(prefix "ex${ARG_NUMBER}")
    get_filename_component(folder_name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

    # ---- DSP -----------------------------------------------------------------------------------
    add_library(${prefix}_dsp STATIC ${ARG_DSP_SOURCES})
    target_include_directories(${prefix}_dsp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${prefix}_dsp PUBLIC learn_juce_modules)

    foreach(dependency IN LISTS ARG_DEPENDS)
        target_link_libraries(${prefix}_dsp PUBLIC "$<LINK_ONLY:ex${dependency}_dsp>")
    endforeach()
    set_target_properties(${prefix}_dsp PROPERTIES FOLDER "exercises/${folder_name}")

    # ---- Testes --------------------------------------------------------------------------------
    add_executable(${prefix}_tests ${ARG_TEST_SOURCES})
    target_link_libraries(${prefix}_tests PRIVATE ${prefix}_dsp learn_testing Catch2::Catch2WithMain)
    set_target_properties(${prefix}_tests PROPERTIES FOLDER "exercises/${folder_name}")

    catch_discover_tests(${prefix}_tests
        TEST_PREFIX "${prefix} | "
        DISCOVERY_MODE PRE_TEST
        PROPERTIES LABELS "${prefix}")

    # ---- Standalone ----------------------------------------------------------------------------
    if(NOT ARG_NO_APP)
        juce_add_gui_app(${prefix}_app
            PRODUCT_NAME "Learn JUCE ${ARG_NUMBER}"
            COMPANY_NAME "Learn JUCE"
            BUNDLE_ID "com.learnjuce.${prefix}"
            VERSION "${PROJECT_VERSION}")

        target_sources(${prefix}_app PRIVATE ${ARG_APP_SOURCES})
        target_compile_definitions(${prefix}_app PRIVATE
            LEARN_EXERCISE_NUMBER="${ARG_NUMBER}"
            LEARN_EXERCISE_WEB_PAGE="${ARG_WEB_PAGE}")
        target_link_libraries(${prefix}_app PRIVATE ${prefix}_dsp learn_host)
        set_target_properties(${prefix}_app PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            FOLDER "exercises/${folder_name}")
    endif()
endfunction()

# learn_add_plugin() — um plugin a sério (VST3 + Standalone do JUCE), para os exercícios 35 e 40.
#
#   learn_add_plugin(
#       NUMBER        35
#       PRODUCT_NAME  "Learn JUCE Eco"
#       PLUGIN_CODE   Lj35                       (4 caracteres, exatamente uma maiúscula)
#       SOURCES       Processor.cpp PluginEntry.cpp ../32-apvts-parameters/Processor.cpp ...)
#
# Cria exNN_plugin (código partilhado), exNN_plugin_VST3 e exNN_plugin_Standalone, todos fora do ALL
# (só compilam com ./learn run NN). O wrapper de plugin precisa de compilar os módulos JUCE com as
# definições JucePlugin_* deste plugin, por isso aqui NÃO se usa learn_juce_modules: os módulos são
# compilados outra vez para este alvo (ver DECISIONS.md, ponto 5). Por isso as fontes dos exercícios
# anteriores entram como SOURCES (e não como bibliotecas).
function(learn_add_plugin)
    set(one_value_args NUMBER PRODUCT_NAME PLUGIN_CODE)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(target "ex${ARG_NUMBER}_plugin")
    get_filename_component(folder_name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

    juce_add_plugin(${target}
        PRODUCT_NAME "${ARG_PRODUCT_NAME}"
        COMPANY_NAME "Learn JUCE"
        BUNDLE_ID "com.learnjuce.${target}"
        VERSION "${PROJECT_VERSION}"
        PLUGIN_MANUFACTURER_CODE Lrnj
        PLUGIN_CODE ${ARG_PLUGIN_CODE}
        FORMATS VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx)

    target_sources(${target} PRIVATE ${ARG_SOURCES})

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_STRICT_REFCOUNTEDPOINTER=1)

    target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    foreach(t ${target} ${target}_VST3 ${target}_Standalone)
        if(TARGET ${t})
            set_target_properties(${t} PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "exercises/${folder_name}")
        endif()
    endforeach()

    if(TARGET ${target}_All)
        set_target_properties(${target}_All PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "exercises/${folder_name}")
    endif()
endfunction()
