# Host standalone partilhado: compilado uma vez, ligado ao standalone de cada exercício.
# O main() (START_JUCE_APPLICATION) fica no app/ExerciseApp.cpp de cada exercício, através da
# macro LEARN_EXERCISE_APP.

add_library(learn_host STATIC
    include/learn_host/ExerciseAudioProcessor.h
    include/learn_host/HostApplication.h
    source/AnalyserComponents.cpp
    source/AnalyserComponents.h
    source/DemoLoop.cpp
    source/DemoLoop.h
    source/HostApplication.cpp
    source/HostAudioEngine.cpp
    source/HostAudioEngine.h
    source/HostComponent.cpp
    source/HostComponent.h
    source/TestSignalSource.cpp
    source/TestSignalSource.h)

target_include_directories(learn_host PUBLIC include)
target_link_libraries(learn_host PUBLIC learn_juce_modules)
set_target_properties(learn_host PROPERTIES FOLDER "shared")
