// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta com "Impulsos" (ouve a cauda e o "flutter" metálico típico do Schroeder) e com o loop.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class SchroederExercise final : public learn::ExerciseAudioProcessor
{
public:
    SchroederExercise()
    {
        addParameter (decay = new juce::AudioParameterFloat (
                          juce::ParameterID { "decay", 1 }, "Decaimento (RT60)",
                          juce::NormalisableRange<float> (0.2f, 10.0f, 0.0f, 0.5f), 2.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("s")));
        addParameter (damping = new juce::AudioParameterFloat (juce::ParameterID { "damping", 1 }, "Damping",
                                                               juce::NormalisableRange<float> (0.0f, 0.95f), 0.3f));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    }

    const juce::String getName() const override { return "Reverb de Schroeder"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        reverb.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
        reverb.reset();
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
        reverb.setDamping (damping->get());
        reverb.setMix (mix->get());
    }

    SchroederReverb reverb;
    juce::AudioParameterFloat* decay = nullptr;
    juce::AudioParameterFloat* damping = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (SchroederExercise)
