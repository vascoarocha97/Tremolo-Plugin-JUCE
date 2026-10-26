#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../29-fdn/Processor.h" // a TUA FDN do exercício 29

#include <array>
#include <vector>

/** Exercício 30 — Difusor + FDN (ao estilo do artigo "Let's Write A Reverb", Signalsmith).

    Antes da FDN, o som passa por vários "passos de difusão". Cada passo: atrasa cada um dos 8
    canais um tempo diferente, inverte a polaridade de alguns, e mistura todos com uma matriz de
    Hadamard. Cada passo multiplica o número de ecos por 8: em 4 passos, um impulso vira milhares
    de ecos — a "nuvem" densa que uma sala real produz logo no início. */
using Frame = FeedbackDelayNetwork::Frame;

namespace Mixing
{
    /** Hadamard normalizada (1/√N), no sítio, pelo algoritmo "borboleta" em O(N·log N).
        Ortogonal e simétrica: H·H = I. Mistura TUDO com tudo, com sinais diferentes. */
    void hadamard (Frame& v) noexcept;
}

class DiffusionStep
{
public:
    /** O canal i recebe um atraso aleatório dentro do seu segmento [i, i+1)·maxDelayMs/N, e uma
        polaridade aleatória. A seed torna a escolha reprodutível (e diferente para cada passo). */
    void prepare (double sampleRate, float maxDelayMs, juce::int64 seed);
    void reset() noexcept;

    /** atrasar -> inverter polaridades -> Hadamard */
    Frame process (const Frame& input) noexcept;

    int getDelaySamples (int channel) const noexcept { return static_cast<int> (lines[(size_t) channel].size()); }
    bool isFlipped (int channel) const noexcept { return flips[(size_t) channel]; }

private:
    std::array<std::vector<float>, FeedbackDelayNetwork::size> lines;
    std::array<int, FeedbackDelayNetwork::size> indices {};
    std::array<bool, FeedbackDelayNetwork::size> flips {};
};

class DiffuserReverb
{
public:
    static constexpr int numSteps = 4;
    /** Cada passo com o dobro do anterior: 20, 40, 80, 160 ms (como no artigo da Signalsmith). */
    static constexpr std::array<float, numSteps> stepLengthsMs { 20.0f, 40.0f, 80.0f, 160.0f };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    void setDecaySeconds (float rt60) noexcept { fdn.setDecaySeconds (rt60); }
    void setMix (float newMix) noexcept { mix = juce::jlimit (0.0f, 1.0f, newMix); }

    /** Só o difusor (sem FDN), de um canal de entrada para a saída 0 — para os testes. */
    Frame diffuseOnly (const Frame& input) noexcept;

private:
    std::array<DiffusionStep, numSteps> steps;
    FeedbackDelayNetwork fdn;
    float mix = 0.3f;
};
