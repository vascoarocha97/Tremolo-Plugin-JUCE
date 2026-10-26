#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 12 — Eco com feedback.

    A partir daqui usamos a delay line do JUCE (juce::dsp::DelayLine): já sabes o que ela faz
    por dentro (exercícios 10 e 11). */
class FeedbackEcho
{
public:
    static constexpr double maximumDelaySeconds = 2.0;
    static constexpr float maximumFeedback = 0.95f;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. Todos os parâmetros mudam com smoothing. */
    void setDelayMilliseconds (float newDelayMs) noexcept;
    /** Limitado a [0, maximumFeedback]. */
    void setFeedback (float newFeedback) noexcept;
    /** 0 = só o sinal original (dry), 1 = só os ecos (wet). */
    void setMix (float newMix) noexcept;

private:
    float delayMsToSamples (float ms) const noexcept;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float> delaySamples, feedback, mix;
    double sampleRate = 44100.0;
    float delayMs = 250.0f, feedbackTarget = 0.4f, mixTarget = 0.5f;
};
