#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>

/** Exercício 09 — Ganho suave e thread-safe, na convenção juce::dsp.

    A partir daqui os processadores seguem a convenção dos blocos do módulo juce_dsp:
      prepare (const ProcessSpec&)                 -> sample rate, bloco máximo, nº de canais
      process (const ProcessContextReplacing&)     -> processa o bloco "no sítio"
      reset()                                      -> limpa o estado (sem alocar)
    É isto que permite encaixá-los num juce::dsp::ProcessorChain. */
class SmoothedGain
{
public:
    static constexpr double rampLengthSeconds = 0.02;

    /** Ganhos iguais ou abaixo disto são silêncio (ganho 0). */
    static constexpr float silenceDb = -60.0f;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Pode ser chamado de QUALQUER thread (GUI, automação do host) a qualquer momento. */
    void setGainDecibels (float newGainDb) noexcept;
    float getGainDecibels() const noexcept;

private:
    static float toLinear (float gainDb) noexcept;

    std::atomic<float> targetGainDb { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gain { 1.0f };
};
