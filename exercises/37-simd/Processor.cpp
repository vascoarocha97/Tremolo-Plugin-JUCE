#include "Processor.h"

#include <cmath>

using Register = juce::dsp::SIMDRegister<float>;

//==============================================================================
void Simd::mixScalar (float* dest, const float* src, float gain, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
        dest[i] += gain * src[i];
}

float Simd::sumOfSquaresScalar (const float* data, int numSamples) noexcept
{
    float sum = 0.0f;

    for (int i = 0; i < numSamples; ++i)
        sum += data[i] * data[i];

    return sum;
}

//==============================================================================
void Simd::mix (float* dest, const float* src, float gain, int numSamples) noexcept
{
    // TODO(1): uma só chamada a juce::FloatVectorOperations (procura a operação "soma com multiplicação").
    juce::ignoreUnused (dest, src, gain, numSamples);
}

float Simd::peak (const float* data, int numSamples) noexcept
{
    // TODO(2): FloatVectorOperations::findMinAndMax devolve um juce::Range<float> (mínimo, máximo).
    //   O pico é o maior dos dois em valor absoluto. Atenção a numSamples == 0.
    juce::ignoreUnused (data, numSamples);
    return 0.0f;
}

float Simd::sumOfSquares (const float* data, int numSamples) noexcept
{
    // TODO(3): com Register (= juce::dsp::SIMDRegister<float>), em três partes (ver README):
    //   1. cabeça: amostras uma a uma até Register::isSIMDAligned (data + i);
    //   2. corpo: Register::SIMDNumElements de cada vez, Register::fromRawArray, num acumulador
    //      Register (começa em Register::expand (0.0f)); no fim, acumulador.sum();
    //   3. cauda: as amostras que sobram, uma a uma.
    juce::ignoreUnused (data, numSamples);
    return 0.0f;
}

float Simd::rmsDecibels (const float* data, int numSamples) noexcept
{
    if (numSamples <= 0)
        return -100.0f;

    const auto rms = std::sqrt (sumOfSquares (data, numSamples) / static_cast<float> (numSamples));
    return juce::Decibels::gainToDecibels (rms, -100.0f);
}
