// Wrapper do standalone. Não precisas de editar este ficheiro.
// Compara com o exercício 28: com "Impulsos", a FDN tem menos "flutter" metálico na cauda.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class FdnExercise final : public learn::ExerciseAudioProcessor
{
public:
    FdnExercise()
    {
        addParameter (decay = new juce::AudioParameterFloat (
                          juce::ParameterID { "decay", 1 }, "Decaimento (RT60)",
                          juce::NormalisableRange<float> (0.2f, 20.0f, 0.0f, 0.4f), 2.5f,
                          juce::AudioParameterFloatAttributes().withLabel ("s")));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    }

    const juce::String getName() const override { return "Feedback Delay Network"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        reverb.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        reverb.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        reverb.setDecaySeconds (decay->get());
        reverb.setMix (mix->get());
    }

    FdnReverb reverb;
    juce::AudioParameterFloat* decay = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (FdnExercise)
