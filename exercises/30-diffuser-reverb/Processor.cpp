#include "Processor.h"

#include <algorithm>
#include <cmath>

void Mixing::hadamard (Frame& v) noexcept
{
    // TODO(1): Hadamard rápida ("borboletas", ver README):
    //   - para half = 1, 2, 4 (enquanto half < N): em blocos de 2·half elementos, cada par
    //     (a, b) = (v[i], v[i + half]) passa a (a + b, a − b);
    //   - no fim multiplica tudo por 1/√N para a matriz ficar ortonormal.
    juce::ignoreUnused (v);
}

void DiffusionStep::prepare (double sampleRate, float maxDelayMs, juce::int64 seed)
{
    juce::Random random (seed);

    // TODO(2): divide o intervalo [0, maxDelayMs] em N segmentos iguais (em amostras). Para cada canal i:
    //   - atraso = um ponto aleatório dentro do segmento i (random.nextDouble() dá [0, 1)), no mínimo 1;
    //   - lines[i].assign (atraso, 0.0f)  — a linha com exatamente esse tamanho (aqui pode alocar);
    //   - flips[i] = random.nextBool().
    juce::ignoreUnused (sampleRate, maxDelayMs, random);

    reset();
}

void DiffusionStep::reset() noexcept
{
    for (auto& line : lines)
        std::fill (line.begin(), line.end(), 0.0f);

    indices.fill (0);
}

Frame DiffusionStep::process (const Frame& input) noexcept
{
    Frame out {};

    // TODO(3): para cada canal i:
    //   - atraso: lê de lines[i] na posição indices[i], escreve lá input[i], avança o índice
    //     (exatamente como no comb do exercício 27, mas sem feedback);
    //   - polaridade: se flips[i], inverte o sinal.
    // Depois mistura `out` com Mixing::hadamard.
    juce::ignoreUnused (input);

    return out;
}

void DiffuserReverb::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (size_t s = 0; s < steps.size(); ++s)
        steps[s].prepare (spec.sampleRate, stepLengthsMs[s], static_cast<juce::int64> (1000 + 17 * s));

    fdn.prepare (spec.sampleRate);
}

void DiffuserReverb::reset() noexcept
{
    for (auto& s : steps)
        s.reset();

    fdn.reset();
}

Frame DiffuserReverb::diffuseOnly (const Frame& input) noexcept
{
    auto frame = input;

    for (auto& s : steps)
        frame = s.process (frame);

    return frame;
}

void DiffuserReverb::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();

    if (block.getNumChannels() == 0)
        return;

    auto* left = block.getChannelPointer (0);
    auto* right = block.getNumChannels() > 1 ? block.getChannelPointer (1) : nullptr;
    const auto scale = 1.0f / std::sqrt (static_cast<float> (FeedbackDelayNetwork::size));

    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        const auto dryL = left[n];
        const auto dryR = right != nullptr ? right[n] : dryL;

        // Upmix: canais pares recebem L, ímpares R
        Frame frame;
        for (size_t i = 0; i < frame.size(); ++i)
            frame[i] = (i % 2 == 0 ? dryL : dryR) * scale;

        frame = diffuseOnly (frame);
        const auto out = fdn.processFrame (frame);

        // Downmix: duas combinações diferentes das 8 saídas
        float wetL = 0.0f, wetR = 0.0f;
        for (size_t i = 0; i < out.size(); ++i)
        {
            wetL += (i % 2 == 0 ? 1.0f : -1.0f) * out[i];
            wetR += ((i / 2) % 2 == 0 ? 1.0f : -1.0f) * out[i];
        }

        left[n] = (1.0f - mix) * dryL + mix * wetL * scale;

        if (right != nullptr)
            right[n] = (1.0f - mix) * dryR + mix * wetR * scale;
    }
}
