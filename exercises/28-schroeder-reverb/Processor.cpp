#include "Processor.h"

#include <cmath>

float SchroederReverb::feedbackForRt60 (int delaySamples, double sampleRate, float rt60Seconds) noexcept
{
    // TODO(1): cada volta no comb (D amostras) multiplica o eco por g. Quantas voltas cabem em
    //   rt60Seconds? Que g faz o produto de todas essas voltas valer 0.001 (−60 dB)?
    //   Protege-te de rt60Seconds <= 0 (usa um mínimo, p. ex. 0.01 s).
    juce::ignoreUnused (delaySamples, sampleRate, rt60Seconds);
    return 0.0f;
}

void SchroederReverb::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    const auto scale = sampleRate / 44100.0;

    for (int ch = 0; ch < 2; ++ch)
    {
        const auto spread = ch == 0 ? 0 : stereoSpread;

        for (int i = 0; i < numCombs; ++i)
            combs[(size_t) ch][(size_t) i].prepare (static_cast<int> ((combTunings[(size_t) i] + spread) * scale));

        for (int i = 0; i < numAllpasses; ++i)
        {
            auto& ap = allpasses[(size_t) ch][(size_t) i];
            ap.prepare (static_cast<int> ((allpassTunings[(size_t) i] + spread) * scale));
            ap.setGain (0.5f);
        }
    }

    updateFeedback();
}

void SchroederReverb::reset() noexcept
{
    for (auto& channel : combs)
        for (auto& c : channel)
            c.reset();

    for (auto& channel : allpasses)
        for (auto& a : channel)
            a.reset();
}

void SchroederReverb::setDecaySeconds (float rt60) noexcept
{
    decay = rt60;
    updateFeedback();
}

void SchroederReverb::setDamping (float newDamping) noexcept
{
    damping = juce::jlimit (0.0f, 0.95f, newDamping);
    updateFeedback();
}

void SchroederReverb::updateFeedback() noexcept
{
    // Cada comb tem o seu próprio g: combs mais compridos dão menos voltas por segundo, por isso
    // cada volta tem de perder mais (g menor) para todos decaírem ao mesmo ritmo.
    for (auto& channel : combs)
    {
        for (auto& c : channel)
        {
            c.setFeedback (feedbackForRt60 (c.getDelaySamples(), sampleRate, decay));
            c.setDamping (damping);
        }
    }
}

void SchroederReverb::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = block.getNumChannels();

    if (numChannels == 0)
        return;

    auto* left = block.getChannelPointer (0);
    auto* right = numChannels > 1 ? block.getChannelPointer (1) : nullptr;

    // Escala de entrada: 8 combs somados ficam muito altos (o Freeverb usa um ganho fixo de 0.015
    // com a sua escala de "room size"; aqui, 1/numCombs mantém o nível perto do original).
    constexpr float inputGain = 1.0f / numCombs;

    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        const auto dryL = left[n];
        const auto dryR = right != nullptr ? right[n] : dryL;
        const auto input = 0.5f * (dryL + dryR) * inputGain;

        float wet[2] {};

        // TODO(2): para cada canal (0 = L, 1 = R), calcula wet[ch]:
        //   - os 8 combs de combs[ch] em PARALELO: todos recebem `input`, as saídas somam-se;
        //   - essa soma passa pelos 4 allpasses de allpasses[ch] em SÉRIE (um a seguir ao outro).
        juce::ignoreUnused (input);

        left[n] = (1.0f - mix) * dryL + mix * wet[0];

        if (right != nullptr)
            right[n] = (1.0f - mix) * dryR + mix * wet[1];
    }
}
