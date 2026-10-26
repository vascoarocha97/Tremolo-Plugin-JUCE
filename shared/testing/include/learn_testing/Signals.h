#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <numbers>

namespace learn::test
{

/** Seno de amplitude e frequência dadas, igual em todos os canais. */
inline juce::AudioBuffer<float> makeSine (int numChannels, int numSamples, double frequencyHz,
                                          double sampleRate, float amplitude = 1.0f,
                                          double startPhaseRadians = 0.0)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    const auto increment = 2.0 * std::numbers::pi * frequencyHz / sampleRate;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int n = 0; n < numSamples; ++n)
            data[n] = amplitude * static_cast<float> (std::sin (startPhaseRadians + increment * n));
    }

    return buffer;
}

/** Zeros em todo o lado e um único impulso na posição dada. */
inline juce::AudioBuffer<float> makeImpulse (int numChannels, int numSamples, int position = 0,
                                             float amplitude = 1.0f)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    buffer.clear();

    for (int ch = 0; ch < numChannels; ++ch)
        buffer.setSample (ch, position, amplitude);

    return buffer;
}

/** Valor constante em todas as amostras. */
inline juce::AudioBuffer<float> makeConstant (int numChannels, int numSamples, float value)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        juce::FloatVectorOperations::fill (buffer.getWritePointer (ch), value, numSamples);

    return buffer;
}

/** 0 até stepPosition, depois `value` (um degrau). */
inline juce::AudioBuffer<float> makeStep (int numChannels, int numSamples, int stepPosition,
                                          float value = 1.0f)
{
    auto buffer = makeConstant (numChannels, numSamples, 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
        for (int n = stepPosition; n < numSamples; ++n)
            buffer.setSample (ch, n, value);

    return buffer;
}

/** Ruído branco uniforme em [-amplitude, amplitude], determinístico (seed fixa).
    Cada canal recebe ruído diferente. */
inline juce::AudioBuffer<float> makeNoise (int numChannels, int numSamples, float amplitude = 1.0f,
                                           juce::int64 seed = 1234)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    juce::Random random (seed);

    for (int ch = 0; ch < numChannels; ++ch)
        for (int n = 0; n < numSamples; ++n)
            buffer.setSample (ch, n, amplitude * (2.0f * random.nextFloat() - 1.0f));

    return buffer;
}

/** Cópia profunda (útil para comparar entrada e saída). */
inline juce::AudioBuffer<float> copyOf (const juce::AudioBuffer<float>& source)
{
    juce::AudioBuffer<float> copy;
    copy.makeCopyOf (source);
    return copy;
}

} // namespace learn::test
