#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 06 — Decibéis. */
namespace Gain
{
    /** Abaixo deste valor consideramos "−∞ dB" (silêncio). O juce::Decibels usa o mesmo valor por omissão. */
    constexpr float minusInfinityDb = -100.0f;

    /** dB -> ganho linear de amplitude. decibels <= minusInfinityDb => 0. */
    float decibelsToGain (float decibels) noexcept;

    /** Ganho linear de amplitude -> dB. gain <= 0 => minusInfinityDb; nunca devolve menos do que isso. */
    float gainToDecibels (float gain) noexcept;

    /** Razão de POTÊNCIAS -> dB (repara: não é a mesma fórmula que para amplitudes). */
    float powerRatioToDecibels (float powerRatio) noexcept;
}

/** Aplica um ganho definido em dB. */
class DecibelGainProcessor
{
public:
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources() {}

    void setGainDecibels (float newGainDb) noexcept;
    float getGainLinear() const noexcept { return gainLinear; }

    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    float gainLinear = 1.0f;
};
