#include "Processor.h"

#include <algorithm>

void CombFilter::prepare (int delaySamples)
{
    buffer.assign (static_cast<size_t> (juce::jmax (1, delaySamples)), 0.0f);
    reset();
}

void CombFilter::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    index = 0;
    filterState = 0.0f;
}

float CombFilter::processSample (float x) noexcept
{
    // TODO(1): o pente com feedback e amortecimento (ver as equações no Processor.h e no README).
    //   - A linha tem exatamente D posições: a posição `index` guarda o que foi escrito há D passos.
    //     Lê-a ANTES de a reescrever.
    //   - Passa a saída pelo passa-baixo de um polo (filterState) e escreve na linha a entrada mais o
    //     feedback do valor filtrado.
    //   - Avança `index` com wrap-around e devolve a saída lida.
    juce::ignoreUnused (x);
    return 0.0f;
}

void SchroederAllpass::prepare (int delaySamples)
{
    buffer.assign (static_cast<size_t> (juce::jmax (1, delaySamples)), 0.0f);
    reset();
}

void SchroederAllpass::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    index = 0;
}

float SchroederAllpass::processSample (float x) noexcept
{
    // TODO(2): o allpass de Schroeder com UMA só delay line, que guarda o sinal intermédio w:
    //     w[n] = x[n] + g·w[n−D]
    //     y[n] = w[n−D] − g·w[n]
    //   Mesma mecânica de leitura/escrita/índice do comb.
    juce::ignoreUnused (x);
    return 0.0f;
}

void CombAllpassDemo::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    combs.resize (spec.numChannels);
    allpasses.resize (spec.numChannels);

    const auto delay = juce::jmax (1, juce::roundToInt (delayMs * 0.001 * sampleRate));

    for (auto& c : combs)
        c.prepare (delay);

    for (auto& a : allpasses)
        a.prepare (delay);
}

void CombAllpassDemo::reset() noexcept
{
    for (auto& c : combs)
        c.reset();

    for (auto& a : allpasses)
        a.reset();
}

void CombAllpassDemo::setParameters (Mode newMode, float newDelayMs, float gain, float damping) noexcept
{
    mode = newMode;
    delayMs = newDelayMs;

    for (auto& c : combs)
    {
        c.setFeedback (gain);
        c.setDamping (damping);
    }

    for (auto& a : allpasses)
        a.setGain (gain);
}

void CombAllpassDemo::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = juce::jmin (block.getNumChannels(), combs.size());

    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* samples = block.getChannelPointer (channel);

        for (size_t n = 0; n < block.getNumSamples(); ++n)
            samples[n] = mode == Mode::comb ? combs[channel].processSample (samples[n])
                                            : allpasses[channel].processSample (samples[n]);
    }
}
