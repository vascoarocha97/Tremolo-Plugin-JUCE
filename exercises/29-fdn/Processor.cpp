#include "Processor.h"

#include <algorithm>
#include <cmath>

void FeedbackDelayNetwork::householder (Frame& v) noexcept
{
    // TODO(1): v ← v − (2/N)·(Σ v)·[1, 1, ..., 1]  — uma soma e uma subtração por elemento, O(N).
    juce::ignoreUnused (v);
}

void FeedbackDelayNetwork::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (size_t i = 0; i < size; ++i)
        lines[i].assign (static_cast<size_t> (delayTimesMs[i] * 0.001 * sampleRate), 0.0f);

    setDecaySeconds (decay);
    reset();
}

void FeedbackDelayNetwork::reset() noexcept
{
    for (auto& line : lines)
        std::fill (line.begin(), line.end(), 0.0f);

    indices.fill (0);
}

void FeedbackDelayNetwork::setDecaySeconds (float rt60) noexcept
{
    decay = juce::jmax (0.01f, rt60);

    // TODO(2): o ganho de cada linha, com a mesma fórmula do exercício 28 (o atraso da linha i é
    //   lines[i].size() amostras). Guarda em gains[i].
}

FeedbackDelayNetwork::Frame FeedbackDelayNetwork::processFrame (const Frame& input) noexcept
{
    Frame delayed {};

    // TODO(3): um passo da rede (ver o diagrama no README):
    //   1. lê a saída de cada linha para `delayed` (como no comb: a posição indices[i] guarda o
    //      que foi escrito há D_i passos);
    //   2. copia `delayed` e mistura a cópia com householder();
    //   3. em cada linha escreve input[i] + gains[i] · misturado[i], e avança indices[i] com wrap-around.
    //   Devolve `delayed`.
    juce::ignoreUnused (input);

    return delayed;
}

std::pair<float, float> FeedbackDelayNetwork::processSample (float monoInput) noexcept
{
    Frame input;
    input.fill (monoInput / std::sqrt (static_cast<float> (size)));

    const auto out = processFrame (input);

    // Duas combinações diferentes (linhas de uma matriz de Hadamard) => L e R descorrelacionados
    float left = 0.0f, right = 0.0f;

    for (size_t i = 0; i < size; ++i)
    {
        left += (i % 2 == 0 ? 1.0f : -1.0f) * out[i];
        right += ((i / 2) % 2 == 0 ? 1.0f : -1.0f) * out[i];
    }

    const auto scale = 1.0f / std::sqrt (static_cast<float> (size));
    return { left * scale, right * scale };
}

double FeedbackDelayNetwork::getStoredEnergy() const noexcept
{
    double energy = 0.0;

    for (const auto& line : lines)
        for (const auto v : line)
            energy += static_cast<double> (v) * v;

    return energy;
}

void FdnReverb::prepare (const juce::dsp::ProcessSpec& spec)
{
    fdn.prepare (spec.sampleRate);
}

void FdnReverb::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();

    if (block.getNumChannels() == 0)
        return;

    auto* left = block.getChannelPointer (0);
    auto* right = block.getNumChannels() > 1 ? block.getChannelPointer (1) : nullptr;

    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        const auto dryL = left[n];
        const auto dryR = right != nullptr ? right[n] : dryL;
        const auto [wetL, wetR] = fdn.processSample (0.5f * (dryL + dryR));

        left[n] = (1.0f - mix) * dryL + mix * wetL;

        if (right != nullptr)
            right[n] = (1.0f - mix) * dryR + mix * wetR;
    }
}
