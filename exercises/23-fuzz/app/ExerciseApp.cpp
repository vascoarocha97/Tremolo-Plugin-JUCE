// Wrapper do standalone. Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class FuzzExercise final : public learn::ExerciseAudioProcessor
{
public:
    FuzzExercise()
    {
        addParameter (gain = new juce::AudioParameterFloat (
                          juce::ParameterID { "fuzz", 1 }, "Fuzz",
                          juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 30.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
        addParameter (tone = new juce::AudioParameterFloat (juce::ParameterID { "tone", 1 }, "Tom",
                                                            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        addParameter (output = new juce::AudioParameterFloat (
                          juce::ParameterID { "output", 1 }, "Volume",
                          juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f), -12.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "Fuzz"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        fuzz.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        fuzz.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        fuzz.setGainDecibels (gain->get());
        fuzz.setTone (tone->get());
        fuzz.setOutputDecibels (output->get());
    }

    Fuzz fuzz;
    juce::AudioParameterFloat* gain = nullptr;
    juce::AudioParameterFloat* tone = nullptr;
    juce::AudioParameterFloat* output = nullptr;
};

LEARN_EXERCISE_APP (FuzzExercise)
