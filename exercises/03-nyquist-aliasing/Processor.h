#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 03 — Nyquist e aliasing. */
namespace Nyquist
{
    /** Frequência que efetivamente se ouve (entre 0 e sampleRate/2) quando um seno de
        `frequency` Hz é amostrado a `sampleRate` Hz.

        Ex.: 30 kHz amostrado a 48 kHz soa a 18 kHz. */
    double aliasedFrequency (double frequency, double sampleRate);
}

/** Dente de serra "ingénuo": uma rampa de −1 a +1 que volta a −1 de repente, calculada
    diretamente amostra a amostra, sem limitar a banda. Os harmónicos acima de Nyquist
    não desaparecem: dobram para frequências que não fazem parte da nota. */
class NaiveSawOscillator
{
public:
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources() {}

    void setFrequency (float newFrequencyHz) noexcept;
    void reset() noexcept;

    /** Substitui o conteúdo do buffer pelo dente de serra (igual em todos os canais). */
    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    void updateIncrement() noexcept;

    double sampleRate = 0.0;
    float frequency = 220.0f;

    double phase = 0.0;     // em ciclos, sempre em [0, 1)
    double increment = 0.0; // ciclos por amostra
};
