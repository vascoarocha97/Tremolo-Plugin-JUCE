// Wrapper do standalone. Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class ChorusExercise final : public learn::ExerciseAudioProcessor
{
public:
    ChorusExercise()
    {
        addParameter (rate = new juce::AudioParameterFloat (
                          juce::ParameterID { "rate", 1 }, "Velocidade",
                          juce::NormalisableRange<float> (0.05f, 5.0f, 0.0f, 0.5f), 0.8f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (depth = new juce::AudioParameterFloat (
                          juce::ParameterID { "depth", 1 }, "Profundidade",
                          juce::NormalisableRange<float> (0.0f, 10.0f), 3.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (centre = new juce::AudioParameterFloat (
                          juce::ParameterID { "centre", 1 }, "Atraso central",
                          juce::NormalisableRange<float> (5.0f, 29.0f), 15.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

    const juce::String getName() const override { return "Chorus"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        chorus.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        chorus.setRate (rate->get());
        chorus.setDepthMs (depth->get());
        chorus.setCentreDelayMs (centre->get());
        chorus.setMix (mix->get());
    }

    Chorus chorus;
    juce::AudioParameterFloat* rate = nullptr;
    juce::AudioParameterFloat* depth = nullptr;
    juce::AudioParameterFloat* centre = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (ChorusExercise)
