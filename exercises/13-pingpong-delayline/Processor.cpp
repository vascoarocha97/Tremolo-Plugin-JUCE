#include "Processor.h"

float PingPongDelay::delayMsToSamples (float ms) const noexcept
{
    const auto samples = static_cast<float> (ms * 0.001 * sampleRate);
    return juce::jlimit (1.0f, static_cast<float> (maximumDelaySeconds * sampleRate), samples);
}

void PingPongDelay::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    numChannels = static_cast<int> (spec.numChannels);

    delayLine.setMaximumDelayInSamples (static_cast<int> (maximumDelaySeconds * sampleRate) + 1);
    delayLine.prepare ({ spec.sampleRate, spec.maximumBlockSize, 2 }); // sempre 2 linhas: L e R

    delaySamples.reset (sampleRate, 0.1);
    feedback.reset (sampleRate, 0.02);
    mix.reset (sampleRate, 0.02);

    reset();
}

void PingPongDelay::reset() noexcept
{
    delayLine.reset();
    delaySamples.setCurrentAndTargetValue (delayMsToSamples (delayMs));
    feedback.setCurrentAndTargetValue (feedbackTarget);
    mix.setCurrentAndTargetValue (mixTarget);
}

void PingPongDelay::setDelayMilliseconds (float newDelayMs) noexcept
{
    delayMs = newDelayMs;
    delaySamples.setTargetValue (delayMsToSamples (newDelayMs));
}

void PingPongDelay::setFeedback (float newFeedback) noexcept
{
    feedbackTarget = juce::jlimit (0.0f, maximumFeedback, newFeedback);
    feedback.setTargetValue (feedbackTarget);
}

void PingPongDelay::setMix (float newMix) noexcept
{
    mixTarget = juce::jlimit (0.0f, 1.0f, newMix);
    mix.setTargetValue (mixTarget);
}

void PingPongDelay::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO: o ping-pong. Se o bloco não tiver exatamente 2 canais, não mexe no áudio.
    //
    // A delay line tem 2 canais (0 = linha da esquerda, 1 = linha da direita). Para cada amostra:
    //   - a entrada é somada a mono;
    //   - lê o que sai de cada linha;
    //   - a linha da ESQUERDA recebe a entrada mono + feedback · (o que saiu da DIREITA);
    //   - a linha da DIREITA recebe feedback · (o que saiu da ESQUERDA) — sem entrada direta;
    //   - saída de cada lado = mistura entre o original desse lado e o que saiu da linha desse lado.
    //
    // Desenha num papel o percurso de um impulso antes de escrever código.
    juce::ignoreUnused (context);
}
