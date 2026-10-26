# Helpers de teste partilhados por todos os exercícios.
#
# É uma OBJECT library (e não STATIC) de propósito: AllocationTracker.cpp substitui o operator new
# global; numa biblioteca estática o linker só incluiria esse .obj se algum símbolo fosse
# referenciado, e a substituição podia desaparecer em silêncio.

add_library(learn_testing OBJECT
    source/AllocationTracker.cpp
    include/learn_testing/RealtimeChecks.h
    include/learn_testing/Signals.h
    include/learn_testing/Measurements.h
    include/learn_testing/BlockProcessing.h
    include/learn_testing/LearnTesting.h)

target_include_directories(learn_testing PUBLIC include)
target_link_libraries(learn_testing PUBLIC learn_juce_modules Catch2::Catch2)
set_target_properties(learn_testing PROPERTIES FOLDER "shared")

# Testes dos próprios helpers
add_executable(learn_testing_selftest tests/SelfTests.cpp)
target_link_libraries(learn_testing_selftest PRIVATE learn_testing Catch2::Catch2WithMain)
set_target_properties(learn_testing_selftest PROPERTIES FOLDER "shared")
catch_discover_tests(learn_testing_selftest TEST_PREFIX "selftest | " DISCOVERY_MODE PRE_TEST)
