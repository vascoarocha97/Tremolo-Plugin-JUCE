// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta com "Ruído branco": o varrimento dos vales vê-se no espectro e ouve-se como um "jato".

#include <learn_host/HostApplication.h>

#include "Processor.h"

class FlangerExercise final : public learn::ExerciseAudioProcessor
{
public:
    FlangerExercise()
    {
        addParameter (rate = new juce::AudioParameterFloat (
                          juce::ParameterID { "rate", 1 }, "Velocidade",
                          juce::NormalisableRange<float> (0.02f, 5.0f, 0.0f, 0.4f), 0.25f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (minDelay = new juce::AudioParameterFloat (
                          juce::ParameterID { "min", 1 }, juce::String::fromUTF8 ("Atraso mínimo"),
                          juce::NormalisableRange<float> (0.1f, 5.0f), 0.3f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (maxDelay = new juce::AudioParameterFloat (
                          juce::ParameterID { "max", 1 }, juce::String::fromUTF8 ("Atraso máximo"),
                          juce::NormalisableRange<float> (0.5f, 15.0f), 5.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (feedback = new juce::AudioParameterFloat (juce::ParameterID { "feedback", 1 }, "Feedback",
                                                                juce::NormalisableRange<float> (-0.95f, 0.95f), 0.6f));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

    const juce::String getName() const override { return "Flanger"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        flanger.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        flanger.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        flanger.setRate (rate->get());
        flanger.setDelayRangeMs (minDelay->get(), maxDelay->get());
        flanger.setFeedback (feedback->get());
        flanger.setMix (mix->get());
    }

    Flanger flanger;
    juce::AudioParameterFloat* rate = nullptr;
    juce::AudioParameterFloat* minDelay = nullptr;
    juce::AudioParameterFloat* maxDelay = nullptr;
    juce::AudioParameterFloat* feedback = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (FlangerExercise)
