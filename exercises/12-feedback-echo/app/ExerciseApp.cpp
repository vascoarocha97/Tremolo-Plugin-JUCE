// Wrapper do standalone. Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class FeedbackEchoExercise final : public learn::ExerciseAudioProcessor
{
public:
    FeedbackEchoExercise()
    {
        addParameter (delayMs = new juce::AudioParameterFloat (
                          juce::ParameterID { "delay", 1 }, "Atraso",
                          juce::NormalisableRange<float> (1.0f, 2000.0f, 0.0f, 0.4f), 350.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (feedback = new juce::AudioParameterFloat (juce::ParameterID { "feedback", 1 }, "Feedback",
                                                                juce::NormalisableRange<float> (0.0f, 0.95f), 0.45f));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));
    }

    const juce::String getName() const override { return "Eco com feedback"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        updateParameters();
        echo.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        updateParameters();
        juce::dsp::AudioBlock<float> block (buffer);
        echo.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void updateParameters()
    {
        echo.setDelayMilliseconds (delayMs->get());
        echo.setFeedback (feedback->get());
        echo.setMix (mix->get());
    }

    FeedbackEcho echo;
    juce::AudioParameterFloat* delayMs = nullptr;
    juce::AudioParameterFloat* feedback = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (FeedbackEchoExercise)
