// Wrapper do standalone. Não precisas de editar este ficheiro.
// Compara com os exercícios 28 e 29 usando "Impulsos": aqui a cauda já começa densa.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class DiffuserExercise final : public learn::ExerciseAudioProcessor
{
public:
    DiffuserExercise()
    {
        addParameter (decay = new juce::AudioParameterFloat (
                          juce::ParameterID { "decay", 1 }, "Decaimento (RT60)",
                          juce::NormalisableRange<float> (0.2f, 20.0f, 0.0f, 0.4f), 3.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("s")));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    }

    const juce::String getName() const override { return "Difusor + FDN"; }

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

    DiffuserReverb reverb;
    juce::AudioParameterFloat* decay = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (DiffuserExercise)
