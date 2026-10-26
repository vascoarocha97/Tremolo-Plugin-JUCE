# Dependências externas, com versões fixas (ver PLAN.md, secção 1).
#
# O FetchContent descarrega cada dependência uma única vez para build/_deps.
# Para reutilizar um clone local (ex.: sem internet), configura com
#   -DFETCHCONTENT_SOURCE_DIR_JUCE=C:/caminho/para/JUCE
# (tem de estar exatamente na mesma tag).

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# JUCE 9.0.4 — commit 3fb5cbc0340758596dac39f7c8de47d562b4896e (2026-10-12)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.4
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)

# Catch2 v3.16.1 (2026-10-08)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)

# Não precisamos dos exemplos nem dos extras do JUCE (Projucer, AudioPluginHost...)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(JUCE Catch2)

# catch_discover_tests()
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)

set_target_properties(Catch2 Catch2WithMain PROPERTIES FOLDER "deps")
