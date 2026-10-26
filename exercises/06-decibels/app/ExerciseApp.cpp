// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Repara no click quando mexes depressa no ganho: a solução é o exercício 08.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class DecibelsExercise final : public learn::ExerciseAudioProcessor
{
public:
    DecibelsExercise()
    {
        addParameter (gainDb = new juce::AudioParameterFloat (
                          juce::ParameterID { "gain", 1 }, "Ganho",
                          juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return juce::String::fromUTF8 ("Decibéis"); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        processor.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        processor.setGainDecibels (gainDb->get());
        processor.processBlock (buffer);
    }

private:
    DecibelGainProcessor processor;
    juce::AudioParameterFloat* gainDb = nullptr;
};

LEARN_EXERCISE_APP (DecibelsExercise)
