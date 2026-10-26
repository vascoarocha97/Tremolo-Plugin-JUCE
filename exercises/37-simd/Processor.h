#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 37 — SIMD: uma instrução, vários dados.

    Um registo SSE tem 128 bits = 4 floats. Uma instrução "soma estes 4 com aqueles 4" custa o mesmo
    que somar 1. O JUCE dá duas ferramentas:
      - juce::FloatVectorOperations: operações prontas sobre arrays (somar, multiplicar, min/max...);
      - juce::dsp::SIMDRegister<float>: um "float com N faixas" para escreveres os teus algoritmos.

    Corre em Release (curriculum.json). */
namespace Simd
{
    //==============================================================================
    // Referências escalares — NÃO EDITES (os testes comparam com elas).

    /** dest[i] += gain · src[i] */
    void mixScalar (float* dest, const float* src, float gain, int numSamples) noexcept;

    /** Σ data[i]² */
    float sumOfSquaresScalar (const float* data, int numSamples) noexcept;

    //==============================================================================
    // As tuas versões.

    /** dest[i] += gain · src[i], com juce::FloatVectorOperations. */
    void mix (float* dest, const float* src, float gain, int numSamples) noexcept;

    /** max |data[i]|, com juce::FloatVectorOperations. 0 para numSamples == 0. */
    float peak (const float* data, int numSamples) noexcept;

    /** Σ data[i]², com juce::dsp::SIMDRegister<float>. Funciona com QUALQUER ponteiro (alinhado ou
        não) e qualquer número de amostras. */
    float sumOfSquares (const float* data, int numSamples) noexcept;

    /** RMS em dB (usa o teu sumOfSquares). −100 dB para silêncio. Já feito. */
    float rmsDecibels (const float* data, int numSamples) noexcept;
}
