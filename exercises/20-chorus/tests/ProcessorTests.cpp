#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>

using namespace learn::test;

namespace
{
    Chorus makeChorus (float rate, float depthMs, float centreMs, float mix)
    {
        Chorus chorus;
        chorus.setRate (rate);
        chorus.setDepthMs (depthMs);
        chorus.setCentreDelayMs (centreMs);
        chorus.setMix (mix);
        chorus.prepare ({ 48000.0, 512, 2 });
        return chorus;
    }
}

TEST_CASE ("Mix 0 deixa o sinal exatamente igual", "[20]")
{
    auto chorus = makeChorus (1.0f, 5.0f, 15.0f, 0.0f);
    auto buffer = makeNoise (2, 8192);
    const auto input = copyOf (buffer);
    processInBlocks (chorus, buffer, awkwardBlockSizes());
    REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
}

TEST_CASE ("Sem modulação (profundidade 0) é um delay fixo do atraso central", "[20]")
{
    auto chorus = makeChorus (1.0f, 0.0f, 10.0f, 1.0f);
    auto buffer = makeImpulse (2, 2000, 0);
    processWhole (chorus, buffer);

    for (int ch = 0; ch < 2; ++ch)
    {
        REQUIRE (firstSampleAbove (buffer, ch, 0.5f) == 480);
        REQUIRE_THAT (buffer.getSample (ch, 480), WithinAbs (1.0, 1.0e-3));
    }
}

TEST_CASE ("O atraso oscila entre centro − profundidade e centro + profundidade, à velocidade do LFO", "[20]")
{
    auto chorus = makeChorus (2.0f, 4.0f, 12.0f, 0.5f);
    juce::AudioBuffer<float> one (2, 1);
    std::vector<float> delays;

    for (int n = 0; n < 48000; ++n) // 1 s = 2 ciclos do LFO
    {
        one.clear();
        processWhole (chorus, one);
        delays.push_back (chorus.getLastDelaySamples (0));
    }

    const auto [minIt, maxIt] = std::minmax_element (delays.begin(), delays.end());
    REQUIRE_THAT (*minIt, WithinAbs ((12.0 - 4.0) * 48.0, 1.0));
    REQUIRE_THAT (*maxIt, WithinAbs ((12.0 + 4.0) * 48.0, 1.0));

    // Começa no centro (seno com fase 0) e volta ao centro a meio ciclo (0.25 s)
    REQUIRE_THAT (delays[0], WithinAbs (12.0 * 48.0, 1.0));
    REQUIRE_THAT (delays[12000], WithinAbs (12.0 * 48.0, 1.0));
    REQUIRE_THAT (delays[6000], WithinAbs (16.0 * 48.0, 1.0)); // 1/4 de ciclo: máximo
}

TEST_CASE ("L e R estão desfasados 90°", "[20]")
{
    auto chorus = makeChorus (1.0f, 5.0f, 15.0f, 0.5f);
    juce::AudioBuffer<float> one (2, 1);
    one.clear();
    processWhole (chorus, one);

    // Na primeira amostra: L no centro (seno a 0), R no máximo (cosseno a 1)
    REQUIRE_THAT (chorus.getLastDelaySamples (0), WithinAbs (15.0 * 48.0, 0.5));
    REQUIRE_THAT (chorus.getLastDelaySamples (1), WithinAbs (20.0 * 48.0, 0.5));
}

TEST_CASE ("A cópia é desafinada: um seno de 1 kHz ganha bandas laterais", "[20]")
{
    auto chorus = makeChorus (3.0f, 5.0f, 15.0f, 1.0f); // só a cópia
    auto buffer = makeSine (2, 96000, 1000.0, 48000.0);
    processInBlocks (chorus, buffer, { 512 });

    // A energia mantém-se (é só um atraso variável), mas já não está toda em 1 kHz
    REQUIRE_THAT (rms (buffer, 0, 48000), WithinAbs (0.7071, 0.02));
    REQUIRE (amplitudeAt (buffer, 0, 1000.0, 48000.0, 48000, 48000) < 0.7);
}

TEST_CASE ("process() não aloca e a saída fica limitada", "[20]")
{
    auto chorus = makeChorus (0.8f, 8.0f, 20.0f, 0.5f);
    auto buffer = makeNoise (2, 8192, 1.0f);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (chorus, buffer, sizes); }) == 0);
    REQUIRE (peak (buffer, 0) <= 1.0f + 1.0e-5f);
}
