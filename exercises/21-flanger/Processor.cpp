#include "Processor.h"

void Flanger::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    delayLine.setMaximumDelayInSamples (static_cast<int> (maxDelayMs * 0.001 * sampleRate) + 2);
    delayLine.prepare ({ spec.sampleRate, spec.maximumBlockSize, static_cast<juce::uint32> (maxChannels) });

    for (auto& lfo : lfos)
    {
        lfo.prepare (sampleRate);
        lfo.setShape (Lfo::Shape::triangle); // varrimento linear, o mais comum em flangers
    }

    lfos[1].setPhaseOffset (0.25f);

    minDelayMs.reset (sampleRate, 0.05);
    maxDelayMsSmoothed.reset (sampleRate, 0.05);
    feedback.reset (sampleRate, 0.02);
    mix.reset (sampleRate, 0.02);
    reset();
}

void Flanger::reset() noexcept
{
    delayLine.reset();

    for (auto& lfo : lfos)
        lfo.reset();

    minDelayMs.setCurrentAndTargetValue (minTarget);
    maxDelayMsSmoothed.setCurrentAndTargetValue (maxTarget);
    feedback.setCurrentAndTargetValue (feedbackTarget);
    mix.setCurrentAndTargetValue (mixTarget);
}

void Flanger::setRate (float hz) noexcept
{
    for (auto& lfo : lfos)
        lfo.setFrequency (hz);
}

void Flanger::setDelayRangeMs (float minMs, float maxMs) noexcept
{
    minTarget = juce::jlimit (0.05f, maxDelayMs - 1.0f, minMs);
    maxTarget = juce::jlimit (minTarget, maxDelayMs - 1.0f, maxMs);
    minDelayMs.setTargetValue (minTarget);
    maxDelayMsSmoothed.setTargetValue (maxTarget);
}

void Flanger::setFeedback (float newFeedback) noexcept
{
    feedbackTarget = juce::jlimit (-maxFeedback, maxFeedback, newFeedback);
    feedback.setTargetValue (feedbackTarget);
}

void Flanger::setMix (float newMix) noexcept
{
    mixTarget = juce::jlimit (0.0f, 1.0f, newMix);
    mix.setTargetValue (mixTarget);
}

void Flanger::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO: como no chorus (exercício 20), mas:
    //   - o atraso varre entre o mínimo e o máximo: mínimo + (máximo − mínimo) · unipolar (LFO);
    //   - o que entra na delay line é a entrada MAIS feedback · cópia (como no eco do exercício 12).
    // Não te esqueças de lastDelay[canal].
    juce::ignoreUnused (context);
}
