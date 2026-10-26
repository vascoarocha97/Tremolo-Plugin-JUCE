#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <numbers>

using namespace learn::test;

TEST_CASE ("DcBlocker: R = 1 − 2π·fc/fs", "[23]")
{
    DcBlocker dc;
    dc.prepare (48000.0, 10.0f);
    REQUIRE_THAT (dc.getR(), WithinAbs (1.0 - 2.0 * std::numbers::pi * 10.0 / 48000.0, 1.0e-6));
}

TEST_CASE ("DcBlocker: um valor constante desaparece; 1 kHz passa", "[23]")
{
    DcBlocker dc;
    dc.prepare (48000.0, 10.0f);

    float y = 0.0f;

    for (int n = 0; n < 48000; ++n)
        y = dc.processSample (0.5f);

    REQUIRE (std::abs (y) < 1.0e-3f);

    dc.reset();
    auto input = makeSine (1, 48000, 1000.0, 48000.0);
    auto output = copyOf (input);

    for (int n = 0; n < 48000; ++n)
        output.setSample (0, n, dc.processSample (output.getSample (0, n)));

    REQUIRE_THAT (gainDb (input, output, 0, 24000), WithinAbs (0.0, 0.05));
}

TEST_CASE ("DcBlocker: um seno com DC fica centrado em zero", "[23]")
{
    DcBlocker dc;
    dc.prepare (48000.0, 10.0f);

    juce::AudioBuffer<float> buffer (1, 96000);

    for (int n = 0; n < 96000; ++n)
        buffer.setSample (0, n, dc.processSample (0.3f + 0.5f * static_cast<float> (std::sin (2.0 * std::numbers::pi * 200.0 * n / 48000.0))));

    REQUIRE (std::abs (mean (buffer, 0, 48000)) < 1.0e-3);
}

TEST_CASE ("Fuzz: o clip assimétrico cria DC, e o DC blocker tira-o", "[23]")
{
    Fuzz fuzz;
    fuzz.setGainDecibels (40.0f);
    fuzz.setTone (1.0f);
    fuzz.setOutputDecibels (0.0f);
    fuzz.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeSine (2, 96000, 220.0, 48000.0, 0.5f);
    processInBlocks (fuzz, buffer, { 512 });

    // Sem DC blocker a média seria ~ +0.25 (a parte positiva satura em +1, a negativa só em −0.5)
    REQUIRE (std::abs (mean (buffer, 0, 48000)) < 0.01);
    REQUIRE (rms (buffer, 0, 48000) > 0.3); // e o sinal continua lá, bem saturado
}

TEST_CASE ("Fuzz: o tom escurece (menos harmónicos agudos)", "[23]")
{
    auto harmonicAt5k = [] (float tone)
    {
        Fuzz fuzz;
        fuzz.setGainDecibels (30.0f);
        fuzz.setTone (tone);
        fuzz.setOutputDecibels (0.0f);
        fuzz.prepare ({ 48000.0, 512, 1 });

        auto buffer = makeSine (1, 48000, 250.0, 48000.0, 0.5f);
        processInBlocks (fuzz, buffer, { 512 });
        return toDb (amplitudeAt (buffer, 0, 5000.0, 48000.0, 24000, 24000));
    };

    REQUIRE (harmonicAt5k (0.0f) < harmonicAt5k (1.0f) - 12.0);
    REQUIRE_THAT (Fuzz::toneToFrequency (0.0f), WithinAbs (500.0, 0.01));
    REQUIRE_THAT (Fuzz::toneToFrequency (1.0f), WithinAbs (10000.0, 0.5));
}

TEST_CASE ("Fuzz: saída limitada pelo volume e process() não aloca", "[23]")
{
    Fuzz fuzz;
    fuzz.setGainDecibels (50.0f);
    fuzz.setTone (1.0f);
    fuzz.setOutputDecibels (-6.0f);
    fuzz.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeNoise (2, 48000, 1.0f);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (fuzz, buffer, sizes); }) == 0);
    REQUIRE (allFinite (buffer));
    REQUIRE (peak (buffer, 0) < 1.5f * 0.5f);
}
