#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 04 — Bit depth e quantização. */
namespace Quantiser
{
    /** Distância entre dois níveis consecutivos (1 LSB) para `bits` bits, na escala [-1, 1).
        Como num inteiro com sinal: 16 bits => 65536 níveis de -32768 a 32767 => LSB = 1/32768. */
    double stepSize (int bits) noexcept;

    /** Arredonda x para o nível mais próximo (quantização "mid-tread": o zero é um nível) e limita
        o resultado ao intervalo representável [-1, 1 - LSB]. */
    float quantise (float x, int bits) noexcept;
}

/** Reduz a resolução do sinal para N bits, com dither TPDF opcional. */
class BitDepthReducer
{
public:
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources() {}

    void setBits (int newBits) noexcept           { bits = juce::jlimit (1, 24, newBits); }
    void setDitherEnabled (bool shouldDither) noexcept { ditherEnabled = shouldDither; }

    /** Se o dither estiver ligado, soma ruído TPDF (±1 LSB, triangular) antes de quantizar. */
    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    int bits = 8;
    bool ditherEnabled = false;
    juce::Random random;
};
