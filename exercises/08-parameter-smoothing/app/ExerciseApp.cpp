// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta com a fonte "Seno" a 200 Hz: alterna o "Ganho" entre os extremos com e sem smoothing.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class SmoothingExercise final : public learn::ExerciseAudioProcessor
{
public:
    SmoothingExercise()
    {
        addParameter (gain = new juce::AudioParameterFloat (juce::ParameterID { "gain", 1 }, "Ganho (linear)",
                                                            juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));
        addParameter (smoothing = new juce::AudioParameterBool (juce::ParameterID { "smoothing", 1 },
                                                                "Smoothing", true));
    }

    const juce::String getName() const override { return "Parameter smoothing"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        smoothed.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (smoothing->get())
        {
            smoothed.setGain (gain->get());
            smoothed.processBlock (buffer);
        }
        else
        {
            // Sem smoothing: o ganho muda de uma vez no início do bloco => salto => click
            smoothed.setGain (gain->get());
            buffer.applyGain (gain->get());
        }
    }

private:
    SmoothedGainProcessor smoothed;
    juce::AudioParameterFloat* gain = nullptr;
    juce::AudioParameterBool* smoothing = nullptr;
};

LEARN_EXERCISE_APP (SmoothingExercise)
