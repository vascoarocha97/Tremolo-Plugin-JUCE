# Learn JUCE — projeto único com todos os exercícios.
#
# Estrutura (ver PLAN.md):
#   cmake/LearnDependencies.cmake  -> JUCE 9.0.4 + Catch2 v3.16.1 via FetchContent
#   cmake/LearnJuceModules.cmake   -> biblioteca estática única com os módulos JUCE
#   cmake/LearnExercise.cmake      -> learn_add_exercise(): exNN_dsp, exNN_tests, exNN_app
#   shared/                        -> helpers de teste e host standalone partilhado
#   exercises/NN-*/                -> um exercício por pasta

cmake_minimum_required(VERSION 3.22)

project(LearnJuce
    VERSION 0.1.0
    DESCRIPTION "Plataforma para aprender JUCE e DSP de audio"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Pastas no Visual Studio / VS Code
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# compile_commands.json para o IntelliSense (só tem efeito com Ninja/Makefiles)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(LearnDependencies)
include(LearnJuceModules)
include(LearnExercise)

enable_testing()

add_subdirectory(shared)
add_subdirectory(exercises)
