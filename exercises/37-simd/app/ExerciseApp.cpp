// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Um medidor de nível feito com as tuas funções: "Nível RMS" usa o teu sumOfSquares (SIMD) e
// "Pico" o teu peak. "Mistura" soma ao bloco o bloco anterior com o teu mix (um eco curtíssimo,
// com feedback: ouve-se como um filtro pente).
// Usa ./learn run 37: compila em Release.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class SimdExercise final : public learn::ExerciseAudioProcessor
{
public:
    SimdExercise()
    {
        addParameter (amount = new juce::AudioParameterFloat (juce::ParameterID { "amount", 1 }, "Mistura",
                                                              juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        addParameter (rmsDb = new juce::AudioParameterFloat (juce::ParameterID { "rms", 1 }, juce::String::fromUTF8 ("Nível RMS"),
                                                             juce::NormalisableRange<float> (-100.0f, 6.0f, 0.1f), -100.0f,
                                                             juce::AudioParameterFloatAttributes().withLabel ("dB")));
        addParameter (peakDb = new juce::AudioParameterFloat (juce::ParameterID { "peak", 1 }, "Pico",
                                                              juce::NormalisableRange<float> (-100.0f, 6.0f, 0.1f), -100.0f,
                                                              juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "SIMD"; }

    void prepareToPlay (double, int samplesPerBlock) override
    {
        previous.setSize (2, samplesPerBlock);
        previous.clear();
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        const auto numSamples = juce::jmin (buffer.getNumSamples(), previous.getNumSamples());

        for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            Simd::mix (data, previous.getReadPointer (ch), amount->get(), numSamples);
            previous.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        }

        const auto* left = buffer.getReadPointer (0);
        rmsDb->setValueNotifyingHost (rmsDb->convertTo0to1 (juce::jlimit (-100.0f, 6.0f, Simd::rmsDecibels (left, buffer.getNumSamples()))));
        peakDb->setValueNotifyingHost (peakDb->convertTo0to1 (
            juce::jlimit (-100.0f, 6.0f, juce::Decibels::gainToDecibels (Simd::peak (left, buffer.getNumSamples()), -100.0f))));
    }

private:
    juce::AudioBuffer<float> previous;
    juce::AudioParameterFloat* amount = nullptr;
    juce::AudioParameterFloat* rmsDb = nullptr;
    juce::AudioParameterFloat* peakDb = nullptr;
};

LEARN_EXERCISE_APP (SimdExercise)
