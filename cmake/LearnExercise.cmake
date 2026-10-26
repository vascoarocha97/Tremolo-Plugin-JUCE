# learn_add_exercise() — cria os três alvos de um exercício.
#
#   learn_add_exercise(
#       NUMBER        03
#       WEB_PAGE      nyquist-aliasing          (página da web app, opcional)
#       DSP_SOURCES   Processor.h Processor.cpp
#       TEST_SOURCES  tests/ProcessorTests.cpp
#       APP_SOURCES   app/ExerciseApp.cpp
#       [NO_APP])
#
# Alvos criados:
#   ex03_dsp    biblioteca estática com o código DSP (o que tu editas)
#   ex03_tests  executável Catch2; compila só o DSP + testes (rápido)
#   ex03_app    standalone para ouvir o resultado (fora do ALL: ./learn run 03)
#
# Todos ligam a learn_juce_modules, por isso o JUCE é compilado uma única vez.

function(learn_add_exercise)
    set(options NO_APP)
    set(one_value_args NUMBER WEB_PAGE)
    set(multi_value_args DSP_SOURCES TEST_SOURCES APP_SOURCES)
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
