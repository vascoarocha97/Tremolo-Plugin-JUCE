// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// O wrapper junta o sinal atrasado ao original ("Mistura") para se ouvir um eco simples.
// Com a fonte "Impulsos" vês no osciloscópio/espectro o atraso exato.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class CircularBufferExercise final : public learn::ExerciseAudioProcessor
{
public:
    CircularBufferExercise()
    {
        addParameter (delayMs = new juce::AudioParameterFloat (
                          juce::ParameterID { "delay", 1 }, "Atraso",
                          juce::NormalisableRange<float> (0.0f, 1000.0f, 1.0f), 250.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

    const juce::String getName() const override { return "Buffer circular"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        currentSampleRate = sampleRate;
        delay.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
        dry.setSize (2, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        const auto numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < 2; ++ch)
            dry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        delay.setDelaySamples (juce::roundToInt (delayMs->get() * 0.001 * currentSampleRate));

        juce::dsp::AudioBlock<float> block (buffer);
        delay.process (juce::dsp::ProcessContextReplacing<float> (block));

        // Mistura: (1 - m)·seco + m·atrasado (no exercício 12 vais escrever isto tu)
        const auto m = mix->get();

        for (int ch = 0; ch < 2; ++ch)
        {
            buffer.applyGain (ch, 0, numSamples, m);
            buffer.addFrom (ch, 0, dry, ch, 0, numSamples, 1.0f - m);
        }
    }

private:
    IntegerDelay delay;
    juce::AudioBuffer<float> dry;
    double currentSampleRate = 44100.0;
    juce::AudioParameterFloat* delayMs = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (CircularBufferExercise)
