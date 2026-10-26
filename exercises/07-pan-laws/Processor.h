#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 07 — Leis de pan. */
enum class PanLaw
{
    linear,          // L + R = 1          (centro: −6 dB em cada lado)
    constantPower,   // L² + R² = 1        (centro: −3 dB em cada lado)
    compromise4p5dB  // entre as duas      (centro: −4.5 dB em cada lado)
};

struct StereoGains
{
    float left = 0.0f;
    float right = 0.0f;
};

namespace Pan
{
    /** Ganhos de amplitude para cada lado. pan ∈ [-1, 1]: -1 = só esquerda, 0 = centro, 1 = só direita.
        Valores fora do intervalo são limitados. */
    StereoGains computeGains (float pan, PanLaw law) noexcept;
}

/** Coloca uma fonte mono (a média dos canais de entrada) na imagem estéreo. */
class StereoPanner
{
public:
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources() {}

    void setPan (float newPan) noexcept  { pan = newPan; }
    void setLaw (PanLaw newLaw) noexcept { law = newLaw; }

    /** Buffers com menos de 2 canais ficam como estão. Canais além do 2.º ficam como estão. */
    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    float pan = 0.0f;
    PanLaw law = PanLaw::constantPower;
};
