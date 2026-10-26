#include "Processor.h"

#include <algorithm>
#include <cmath>

void EfficientLowpass::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    state.assign (spec.numChannels, { 0.0f, 0.0f });
    coefficientsNeedUpdate = true;
    updateCoefficientsIfNeeded();
    reset();
}

void EfficientLowpass::reset() noexcept
{
    for (auto& s : state)
        s = { 0.0f, 0.0f };
}

void EfficientLowpass::setCutoff (float newCutoffHz) noexcept
{
    // TODO(1): só marca os coeficientes para recalcular se o valor MUDOU (o host chama isto em
    //   todos os blocos). O mesmo no setResonance.
    cutoff = newCutoffHz;
    coefficientsNeedUpdate = true;
}

void EfficientLowpass::setResonance (float newQ) noexcept
{
    q = newQ;
    coefficientsNeedUpdate = true;
}

void EfficientLowpass::updateCoefficientsIfNeeded() noexcept
{
    if (! coefficientsNeedUpdate)
        return;

    const auto safeCutoff = juce::jlimit (10.0f, 0.49f * static_cast<float> (sampleRate), cutoff);
    coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, safeCutoff, juce::jmax (0.1f, q));
    coefficientsNeedUpdate = false;
    ++coefficientUpdates;
}

bool EfficientLowpass::canSkip (const juce::dsp::AudioBlock<float>& block) const noexcept
{
    // TODO(3): pode-se saltar o bloco se:
    //   - o estado de TODOS os canais for exatamente zero, e
    //   - a entrada de todos os canais estiver abaixo de silenceThreshold (em valor absoluto)
    //     (o pico de um canal: FloatVectorOperations::findMinAndMax, como no exercício 37).
    juce::ignoreUnused (block);
    return false;
}

void EfficientLowpass::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(2): desliga os denormais durante este process (uma linha; ver README).

    auto& block = context.getOutputBlock();

    if (canSkip (block))
    {
        block.clear(); // silêncio → silêncio: nada para calcular
        ++skippedBlocks;
        return;
    }

    updateCoefficientsIfNeeded();

    const auto b0 = coefficients[0], b1 = coefficients[1], b2 = coefficients[2];
    const auto a1 = coefficients[4], a2 = coefficients[5];
    const auto numChannels = std::min (block.getNumChannels(), state.size());

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = block.getChannelPointer (ch);
        auto s1 = state[ch][0], s2 = state[ch][1];

        // Forma direta II transposta
        for (size_t n = 0; n < block.getNumSamples(); ++n)
        {
            const auto x = samples[n];
            const auto y = b0 * x + s1;
            s1 = b1 * x - a1 * y + s2;
            s2 = b2 * x - a2 * y;
            samples[n] = y;
        }

        state[ch] = { s1, s2 };
    }
}
