#include "Processor.h"

#include <cmath>
#include <numbers>

void OnePoleFilter::setCutoff (float cutoffHz, double sampleRate) noexcept
{
    // TODO(1): limita o corte a [1 Hz, 0.49·fs] e calcula a = 1 − e^(−2π·fc/fs) (ver README:
    //          porque é esta a fórmula e não simplesmente 2π·fc/fs).
    juce::ignoreUnused (cutoffHz, sampleRate);
}

float OnePoleFilter::processSample (float x) noexcept
{
    // TODO(2): o passa-baixo aproxima `state` da entrada uma fração `a`; o passa-alto é o que sobra.
    return x;
}

void OnePoleProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filters.resize (spec.numChannels);

    for (auto& f : filters)
    {
        f.setCutoff (cutoff, sampleRate);
        f.setType (type);
    }

    reset();
}

void OnePoleProcessor::reset() noexcept
{
    for (auto& f : filters)
        f.reset();
}

void OnePoleProcessor::setCutoff (float cutoffHz) noexcept
{
    cutoff = cutoffHz;

    for (auto& f : filters)
        f.setCutoff (cutoff, sampleRate);
}

void OnePoleProcessor::setType (OnePoleFilter::Type newType) noexcept
{
    type = newType;

    for (auto& f : filters)
        f.setType (type);
}

void OnePoleProcessor::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = juce::jmin (block.getNumChannels(), filters.size());

    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* samples = block.getChannelPointer (channel);
        auto& filter = filters[channel];

        for (size_t n = 0; n < block.getNumSamples(); ++n)
            samples[n] = filter.processSample (samples[n]);
    }
}
