#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "ExerciseAudioProcessor.h"

namespace learn
{

class MainWindow;

/** A aplicação standalone partilhada por todos os exercícios.

    Abre o dispositivo de áudio, cria o processador do exercício e mostra:
    fonte de teste, parâmetros, osciloscópio, espectro, medidores e um limitador de proteção. */
class HostApplication : public juce::JUCEApplication
{
public:
    HostApplication();
    ~HostApplication() override;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;

protected:
    virtual std::unique_ptr<juce::AudioProcessor> createProcessor() = 0;
    virtual juce::String getExerciseNumber() const = 0;
    virtual juce::String getWebPage() const = 0;

private:
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace learn

#ifndef LEARN_EXERCISE_NUMBER
 #define LEARN_EXERCISE_NUMBER "00"
#endif

#ifndef LEARN_EXERCISE_WEB_PAGE
 #define LEARN_EXERCISE_WEB_PAGE ""
#endif

/** Usa no fim de app/ExerciseApp.cpp: LEARN_EXERCISE_APP (MyExerciseProcessor) */
#define LEARN_EXERCISE_APP(ProcessorType)                                                    \
    class LearnExerciseApplication final : public learn::HostApplication                    \
    {                                                                                        \
    protected:                                                                               \
        std::unique_ptr<juce::AudioProcessor> createProcessor() override                     \
        {                                                                                    \
            return std::make_unique<ProcessorType>();                                        \
        }                                                                                    \
        juce::String getExerciseNumber() const override { return LEARN_EXERCISE_NUMBER; }    \
        juce::String getWebPage() const override { return LEARN_EXERCISE_WEB_PAGE; }         \
    };                                                                                       \
    START_JUCE_APPLICATION (LearnExerciseApplication)
