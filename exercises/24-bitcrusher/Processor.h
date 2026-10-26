#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../04-bit-depth/Processor.h" // o TEU Quantiser do exercício 04

#include <vector>

/** Exercício 24 — Bitcrusher.

    Dois "estragos" digitais de propósito:
      - redução de bits: quantizar para N bits (exercício 04);
      - redução de sample rate: segurar cada valor durante `factor` amostras (sample & hold),
        sem filtrar — o aliasing que daí resulta faz parte do som. */
class Bitcrusher
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setBits (int newBits) noexcept { bits = juce::jlimit (1, 24, newBits); }
    /** >= 1. Pode ser fracionário (ex.: 2.5 segura 2 e 3 amostras, alternadamente). */
    void setDownsampleFactor (float newFactor) noexcept { factor = juce::jmax (1.0f, newFactor); }
    void setMix (float newMix) noexcept { mix = juce::jlimit (0.0f, 1.0f, newMix); }

private:
    struct ChannelState
    {
        float held = 0.0f;   // o valor que está a ser segurado
        double phase = 1.0;  // quando chega a 1, apanha uma amostra nova
    };

    std::vector<ChannelState> channels;
    int bits = 8;
    float factor = 1.0f;
    float mix = 1.0f;
};
