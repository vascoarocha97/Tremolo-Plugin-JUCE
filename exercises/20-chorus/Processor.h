#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../18-lfo/Processor.h" // o TEU LFO do exercício 18

#include <array>

/** Exercício 20 — Chorus.

    Uma cópia do sinal atrasada 5–30 ms, com o atraso a oscilar devagar (LFO). A variação do
    atraso desafina ligeiramente a cópia (efeito Doppler); misturada com o original soa a
    "várias vozes". Em estéreo, os LFOs de L e R estão desfasados 90°. */
class Chorus
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float maxDelayMs = 50.0f;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setRate (float hz) noexcept;                 // velocidade do LFO
    void setDepthMs (float ms) noexcept;              // quanto o atraso oscila (± ms)
    void setCentreDelayMs (float ms) noexcept;        // atraso médio
    void setMix (float newMix) noexcept;              // 0 = só original, 1 = só a cópia

    /** O atraso (em amostras) usado na última amostra processada de cada canal (para os testes). */
    float getLastDelaySamples (int channel) const noexcept { return lastDelay[(size_t) channel]; }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    std::array<Lfo, maxChannels> lfos;
    std::array<float, maxChannels> lastDelay {};
    juce::SmoothedValue<float> depthMs, centreMs, mix;
    double sampleRate = 44100.0;
    float depthTarget = 3.0f, centreTarget = 15.0f, mixTarget = 0.5f;
};
