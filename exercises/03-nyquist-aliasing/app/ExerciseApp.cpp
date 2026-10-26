// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta: sobe a frequência devagar acima de ~2 kHz e ouve (e vê no espectro) os tons que
// descem enquanto a nota sobe — são harmónicos acima de Nyquist a dobrar para baixo.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class NyquistExercise final : public learn::ExerciseAudioProcessor
{
public:
    NyquistExercise()
    {
        addParameter (frequency = new juce::AudioParameterFloat (
                          juce::ParameterID { "frequency", 1 }, juce::String::fromUTF8 ("Frequência"),
                          juce::NormalisableRange<float> (20.0f, 12000.0f, 0.0f, 0.3f), 220.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));

        addParameter (level = new juce::AudioParameterFloat (
                          juce::ParameterID { "level", 1 }, juce::String::fromUTF8 ("Nível"),
                          juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));
    }

    const juce::String getName() const override { return "Nyquist e aliasing"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        saw.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        saw.setFrequency (frequency->get());
        saw.processBlock (buffer);
        buffer.applyGain (level->get());
    }

private:
    NaiveSawOscillator saw;
    juce::AudioParameterFloat* frequency = nullptr;
    juce::AudioParameterFloat* level = nullptr;
};

LEARN_EXERCISE_APP (NyquistExercise)
