#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 13 — Ping-pong com juce::dsp::DelayLine.

    Os ecos alternam entre a esquerda e a direita: a entrada (somada a mono) entra na linha da
    esquerda; o que sai da esquerda alimenta a direita; o que sai da direita volta à esquerda. */
class PingPongDelay
{
public:
    static constexpr double maximumDelaySeconds = 2.0;
    static constexpr float maximumFeedback = 0.95f;

    /** Precisa de 2 canais. Com outro número de canais, process() não mexe no áudio. */
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Atraso de CADA salto (esquerda -> direita -> esquerda ...). Chamar na thread de áudio. */
    void setDelayMilliseconds (float newDelayMs) noexcept;
    void setFeedback (float newFeedback) noexcept;  // limitado a [0, maximumFeedback]
    void setMix (float newMix) noexcept;            // 0 = dry, 1 = wet

private:
    float delayMsToSamples (float ms) const noexcept;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float> delaySamples, feedback, mix;
    double sampleRate = 44100.0;
    int numChannels = 0;
    float delayMs = 300.0f, feedbackTarget = 0.5f, mixTarget = 0.5f;
};
