#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../18-lfo/Processor.h" // o TEU LFO do exercício 18

#include <array>

/** Exercício 21 — Flanger.

    Como o chorus, mas com atrasos muito curtos (0.1–10 ms) e feedback. Somar o sinal com uma cópia
    atrasada D cria um filtro pente: vales nas frequências em que a cópia chega em oposição de fase.
    Com o LFO a mexer em D, os vales varrem o espectro — o "jato" característico. */
class Flanger
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float maxDelayMs = 20.0f;
    static constexpr float maxFeedback = 0.95f;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setRate (float hz) noexcept;
    void setDelayRangeMs (float minMs, float maxMs) noexcept; // o LFO varre entre estes dois
    void setFeedback (float newFeedback) noexcept;            // [−0.95, 0.95]
    void setMix (float newMix) noexcept;                      // 0.5 = vales mais fundos

    float getLastDelaySamples (int channel) const noexcept { return lastDelay[(size_t) channel]; }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    std::array<Lfo, maxChannels> lfos;
    std::array<float, maxChannels> lastDelay {};
    juce::SmoothedValue<float> minDelayMs, maxDelayMsSmoothed, feedback, mix;
    double sampleRate = 44100.0;
    float minTarget = 1.0f, maxTarget = 5.0f, feedbackTarget = 0.5f, mixTarget = 0.5f;
};
