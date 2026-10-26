// Wrapper do standalone: liga o teu SineOscillator a um juce::AudioProcessor real.
// Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class SineOscillatorExercise final : public learn::ExerciseAudioProcessor
{
public:
    SineOscillatorExercise()
    {
        addParameter (frequency = new juce::AudioParameterFloat (
                          juce::ParameterID { "frequency", 1 }, juce::String::fromUTF8 ("Frequência"),
                          juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 440.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));

        addParameter (amplitude = new juce::AudioParameterFloat (
                          juce::ParameterID { "amplitude", 1 }, "Amplitude",
                          juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));
    }

    const juce::String getName() const override { return juce::String::fromUTF8 ("Oscilador sinusoidal"); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        oscillator.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Os parâmetros são lidos uma vez por bloco (são std::atomic por dentro).
        oscillator.setFrequency (frequency->get());
        oscillator.setAmplitude (amplitude->get());
        oscillator.processBlock (buffer);
    }

private:
    SineOscillator oscillator;
    juce::AudioParameterFloat* frequency = nullptr;
    juce::AudioParameterFloat* amplitude = nullptr;
};

LEARN_EXERCISE_APP (SineOscillatorExercise)
