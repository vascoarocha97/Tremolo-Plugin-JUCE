#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace learn::test;

TEST_CASE ("valueAt: as quatro formas em pontos conhecidos do ciclo", "[18]")
{
    using S = Lfo::Shape;

    REQUIRE_THAT (Lfo::valueAt (S::sine, 0.0), WithinAbs (0.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::sine, 0.25), WithinAbs (1.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::sine, 0.75), WithinAbs (-1.0, 1.0e-6));

    REQUIRE_THAT (Lfo::valueAt (S::triangle, 0.0), WithinAbs (0.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::triangle, 0.125), WithinAbs (0.5, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::triangle, 0.25), WithinAbs (1.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::triangle, 0.5), WithinAbs (0.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::triangle, 0.75), WithinAbs (-1.0, 1.0e-6));

    REQUIRE_THAT (Lfo::valueAt (S::saw, 0.0), WithinAbs (0.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::saw, 0.25), WithinAbs (0.5, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::saw, 0.5), WithinAbs (-1.0, 1.0e-6));
    REQUIRE_THAT (Lfo::valueAt (S::saw, 0.75), WithinAbs (-0.5, 1.0e-6));

    REQUIRE (Lfo::valueAt (S::square, 0.1) == 1.0f);
    REQUIRE (Lfo::valueAt (S::square, 0.6) == -1.0f);
}

TEST_CASE ("valueAt aceita fases fora de [0, 1) (periódica)", "[18]")
{
    for (auto shape : { Lfo::Shape::sine, Lfo::Shape::triangle, Lfo::Shape::saw, Lfo::Shape::square })
    {
        INFO ("forma " << static_cast<int> (shape));
        REQUIRE (std::abs (Lfo::valueAt (shape, 0.3)) > 0.1f);
        REQUIRE_THAT (Lfo::valueAt (shape, 2.3), WithinAbs (Lfo::valueAt (shape, 0.3), 1.0e-5));
        REQUIRE_THAT (Lfo::valueAt (shape, -0.7), WithinAbs (Lfo::valueAt (shape, 0.3), 1.0e-5));
    }
}

TEST_CASE ("getNextSample: período exato e valores sempre em [−1, 1]", "[18]")
{
    for (auto shape : { Lfo::Shape::sine, Lfo::Shape::triangle, Lfo::Shape::saw, Lfo::Shape::square })
    {
        Lfo lfo;
        lfo.prepare (48000.0);
        lfo.setFrequency (2.0f); // período = 24000 amostras
        lfo.setShape (shape);

        std::vector<float> values;

        for (int n = 0; n < 48000; ++n)
            values.push_back (lfo.getNextSample());

        INFO ("forma " << static_cast<int> (shape));

        for (int n = 0; n < 24000; n += 97)
            REQUIRE_THAT (values[(size_t) n], WithinAbs (values[(size_t) n + 24000], 1.0e-4));

        REQUIRE (std::all_of (values.begin(), values.end(), [] (float v) { return v >= -1.0f && v <= 1.0f; }));

        // A amostra n tem fase n·f/fs
        REQUIRE_THAT (values[6000], WithinAbs (Lfo::valueAt (shape, 0.25), 1.0e-4));
    }
}

TEST_CASE ("Desvio de fase: um seno com 0.25 ciclos de avanço é um cosseno", "[18]")
{
    Lfo lfo;
    lfo.prepare (1000.0);
    lfo.setFrequency (5.0f);
    lfo.setPhaseOffset (0.25f);

    for (int n = 0; n < 400; ++n)
    {
        const auto expected = std::cos (2.0 * std::numbers::pi * 5.0 * n / 1000.0);
        INFO ("amostra " << n);
        REQUIRE_THAT (lfo.getNextSample(), WithinAbs (expected, 1.0e-4));
    }
}

TEST_CASE ("Mudar a frequência não faz saltar a fase (seno contínuo)", "[18]")
{
    Lfo lfo;
    lfo.prepare (48000.0);
    float previous = lfo.getNextSample();
    float maxJump = 0.0f, maxValue = 0.0f;

    for (int block = 0; block < 100; ++block)
    {
        lfo.setFrequency (block % 2 == 0 ? 0.5f : 15.0f);

        for (int n = 0; n < 480; ++n)
        {
            const auto v = lfo.getNextSample();
            maxJump = juce::jmax (maxJump, std::abs (v - previous));
            maxValue = juce::jmax (maxValue, v);
            previous = v;
        }
    }

    // A 15 Hz o maior passo de um seno é 2π·15/48000 ≈ 0.002
    REQUIRE (maxValue > 0.9f); // o LFO mexeu-se mesmo
    REQUIRE (maxJump < 0.0025f);
}

TEST_CASE ("A fase fica em [0, 1) e o unipolar em [0, 1]", "[18]")
{
    Lfo lfo;
    lfo.prepare (44100.0);
    lfo.setFrequency (19.7f);

    bool phaseInRange = true, unipolarInRange = true;
    double maxPhase = 0.0;

    for (int n = 0; n < 200000; ++n)
    {
        const auto v = lfo.getNextSample();
        phaseInRange = phaseInRange && lfo.getPhase() >= 0.0 && lfo.getPhase() < 1.0;
        maxPhase = juce::jmax (maxPhase, lfo.getPhase());
        unipolarInRange = unipolarInRange && Lfo::toUnipolar (v) >= 0.0f && Lfo::toUnipolar (v) <= 1.0f;
    }

    REQUIRE (maxPhase > 0.99); // a fase percorreu o ciclo todo
    REQUIRE (phaseInRange);
    REQUIRE (unipolarInRange);

    REQUIRE (Lfo::toUnipolar (-1.0f) == 0.0f);
    REQUIRE (Lfo::toUnipolar (1.0f) == 1.0f);
}

TEST_CASE ("reset() volta ao início do ciclo", "[18]")
{
    Lfo lfo;
    lfo.prepare (48000.0);
    lfo.setFrequency (3.0f);

    for (int n = 0; n < 1000; ++n)
        lfo.getNextSample();

    lfo.reset();
    REQUIRE_THAT (lfo.getNextSample(), WithinAbs (0.0, 1.0e-6));
    lfo.reset (0.25);
    REQUIRE_THAT (lfo.getNextSample(), WithinAbs (1.0, 1.0e-6));
}
