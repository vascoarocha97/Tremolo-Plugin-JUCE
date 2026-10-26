// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta: fonte "Seno" a 440 Hz, Mistura = 1 (só o sinal atrasado), e arrasta o "Atraso":
// ouves a altura a mudar enquanto o atraso desliza (efeito Doppler) — é a base do chorus e do flanger.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class FractionalDelayExercise final : public learn::ExerciseAudioProcessor
{
public:
    FractionalDelayExercise()
    {
        addParameter (delayMs = new juce::AudioParameterFloat (
                          juce::ParameterID { "delay", 1 }, "Atraso",
                          juce::NormalisableRange<float> (0.0f, 1000.0f, 0.0f, 0.4f), 250.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (mix = new juce::AudioParameterFloat (juce::ParameterID { "mix", 1 }, "Mistura",
                                                           juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

    const juce::String getName() const override { return juce::String::fromUTF8 ("Delay fracionário"); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        delay.setDelayMilliseconds (delayMs->get());
        delay.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
        dry.setSize (2, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        const auto numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < 2; ++ch)
            dry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        delay.setDelayMilliseconds (delayMs->get());
        juce::dsp::AudioBlock<float> block (buffer);
        delay.process (juce::dsp::ProcessContextReplacing<float> (block));

        const auto m = mix->get();

        for (int ch = 0; ch < 2; ++ch)
        {
            buffer.applyGain (ch, 0, numSamples, m);
            buffer.addFrom (ch, 0, dry, ch, 0, numSamples, 1.0f - m);
        }
    }

private:
    FractionalDelay delay;
    juce::AudioBuffer<float> dry;
    juce::AudioParameterFloat* delayMs = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
};

LEARN_EXERCISE_APP (FractionalDelayExercise)
