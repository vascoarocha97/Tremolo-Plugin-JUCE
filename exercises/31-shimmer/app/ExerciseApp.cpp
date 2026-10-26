// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta com "Loop de demonstração" e um decaimento longo: sobe o "Shimmer" devagar.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class ShimmerExercise final : public learn::ExerciseAudioProcessor
{
public:
    ShimmerExercise()
    {
        addParameter (decay = new juce::AudioParameterFloat (
                          juce::ParameterID { "decay", 1 }, "Decaimento (RT60)",
                          juce::NormalisableRange<float> (0.5f, 20.0f, 0.0f, 0.4f), 6.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("s")));
        addParameter (shimmer = new juce::AudioParameterFloat (juce::ParameterID { "shimmer", 1 }, "Shimmer",
                                                               juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        addParameter (semitones = new juce::AudioParameterInt (juce::ParameterID { "semitones", 1 },
                                                               "Intervalo (semitons)", -12, 12, 12));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));
    }

    const juce::String getName() const override { return "Shimmer"; }

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
        reverb.setShimmer (shimmer->get());
        reverb.setSemitones (static_cast<float> (semitones->get()));
        reverb.setMix (mix->get());
    }

    ShimmerReverb reverb;
    juce::AudioParameterFloat* decay = nullptr;
    juce::AudioParameterFloat* shimmer = nullptr;
    juce::AudioParameterInt* semitones = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (ShimmerExercise)
