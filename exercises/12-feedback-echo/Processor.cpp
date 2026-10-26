#include "Processor.h"

float FeedbackEcho::delayMsToSamples (float ms) const noexcept
{
    // Mínimo de 1 amostra: lemos (popSample) ANTES de escrever (pushSample), por isso um atraso
    // de 0 leria uma posição ainda por escrever.
    const auto samples = static_cast<float> (ms * 0.001 * sampleRate);
    return juce::jlimit (1.0f, static_cast<float> (maximumDelaySeconds * sampleRate), samples);
}

void FeedbackEcho::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // TODO(1): prepara a juce::dsp::DelayLine:
    //   - setMaximumDelayInSamples (...) com espaço para maximumDelaySeconds (+1 de margem);
    //   - prepare (spec).
    //   Lê a documentação/header: setMaximumDelayInSamples aloca (por isso só aqui).

    delaySamples.reset (sampleRate, 0.1);
    feedback.reset (sampleRate, 0.02);
    mix.reset (sampleRate, 0.02);

    reset();
}

void FeedbackEcho::reset() noexcept
{
    delayLine.reset();
    delaySamples.setCurrentAndTargetValue (delayMsToSamples (delayMs));
    feedback.setCurrentAndTargetValue (feedbackTarget);
    mix.setCurrentAndTargetValue (mixTarget);
}

void FeedbackEcho::setDelayMilliseconds (float newDelayMs) noexcept
{
    delayMs = newDelayMs;
    delaySamples.setTargetValue (delayMsToSamples (newDelayMs));
}

void FeedbackEcho::setFeedback (float newFeedback) noexcept
{
    // TODO(2): limita a [0, maximumFeedback] antes de usar (porquê < 1? ver README).
    feedbackTarget = newFeedback;
    feedback.setTargetValue (feedbackTarget);
}

void FeedbackEcho::setMix (float newMix) noexcept
{
    mixTarget = juce::jlimit (0.0f, 1.0f, newMix);
    mix.setTargetValue (mixTarget);
}

void FeedbackEcho::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(3): para cada amostra (avança os três SmoothedValue uma vez) e cada canal:
    //   1. lê o eco:     delayLine.popSample (canal, atraso)
    //   2. escreve:      delayLine.pushSample (canal, entrada + feedback · eco)
    //   3. mistura:      saída = (1 - mix) · entrada + mix · eco
    //
    // A ordem pop -> push importa (ver README).
    juce::ignoreUnused (context);
}
