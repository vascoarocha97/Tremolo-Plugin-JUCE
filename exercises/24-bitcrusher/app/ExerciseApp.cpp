// Wrapper do standalone. Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class BitcrusherExercise final : public learn::ExerciseAudioProcessor
{
public:
    BitcrusherExercise()
    {
        addParameter (bits = new juce::AudioParameterInt (juce::ParameterID { "bits", 1 }, "Bits", 1, 16, 6));
        addParameter (factor = new juce::AudioParameterFloat (
                          juce::ParameterID { "factor", 1 }, juce::String::fromUTF8 ("Redução de sample rate"),
                          juce::NormalisableRange<float> (1.0f, 40.0f, 0.0f, 0.5f), 4.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("x")));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    }

    const juce::String getName() const override { return "Bitcrusher"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        crusher.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        crusher.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        crusher.setBits (bits->get());
        crusher.setDownsampleFactor (factor->get());
        crusher.setMix (mix->get());
    }

    Bitcrusher crusher;
    juce::AudioParameterInt* bits = nullptr;
    juce::AudioParameterFloat* factor = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (BitcrusherExercise)
