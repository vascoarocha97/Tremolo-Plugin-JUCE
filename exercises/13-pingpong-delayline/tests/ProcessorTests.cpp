#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

namespace
{
    void configure (PingPongDelay& delay, float delayMs, float feedback, float mix, juce::uint32 channels = 2)
    {
        delay.setDelayMilliseconds (delayMs);
        delay.setFeedback (feedback);
        delay.setMix (mix);
        delay.prepare ({ 48000.0, 512, channels });
    }
}

TEST_CASE ("Os ecos alternam: esquerda em D, direita em 2D, esquerda em 3D...", "[13]")
{
    PingPongDelay delay;
    configure (delay, 10.0f, 0.5f, 1.0f); // D = 480 amostras, só wet

    auto buffer = makeImpulse (2, 5000, 0); // impulso nos dois canais => entrada mono = 1
    processInBlocks (delay, buffer, awkwardBlockSizes());

    // Esquerda: 1 em D, fb² em 3D
    REQUIRE_THAT (buffer.getSample (0, 480), WithinAbs (1.0, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (0, 960), WithinAbs (0.0, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (0, 1440), WithinAbs (0.25, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (0, 1920), WithinAbs (0.0, 1.0e-3));

    // Direita: fb em 2D, fb³ em 4D
    REQUIRE_THAT (buffer.getSample (1, 480), WithinAbs (0.0, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (1, 960), WithinAbs (0.5, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (1, 1440), WithinAbs (0.0, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (1, 1920), WithinAbs (0.125, 1.0e-3));
}

TEST_CASE ("A entrada é somada a mono: um impulso só na direita também começa à esquerda", "[13]")
{
    PingPongDelay delay;
    configure (delay, 10.0f, 0.5f, 1.0f);

    auto buffer = makeImpulse (2, 2000, 0);
    buffer.setSample (0, 0, 0.0f); // só a direita tem impulso => mono = 0.5

    processWhole (delay, buffer);

    REQUIRE_THAT (buffer.getSample (0, 480), WithinAbs (0.5, 1.0e-3));
    REQUIRE_THAT (buffer.getSample (1, 960), WithinAbs (0.25, 1.0e-3));
}

TEST_CASE ("Mix 0 dá exatamente o sinal original", "[13]")
{
    PingPongDelay delay;
    configure (delay, 120.0f, 0.9f, 0.0f);

    auto buffer = makeNoise (2, 8192);
    const auto input = copyOf (buffer);
    processInBlocks (delay, buffer, awkwardBlockSizes());

    REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
}

TEST_CASE ("Feedback limitado e saída estável", "[13]")
{
    PingPongDelay delay;
    configure (delay, 23.0f, 5.0f, 1.0f);

    auto buffer = makeNoise (2, 48000 * 5, 1.0f);
    processInBlocks (delay, buffer, { 480 });

    REQUIRE (allFinite (buffer));
    REQUIRE (peak (buffer, 0) < 25.0f);
    REQUIRE (peak (buffer, 1) < 25.0f);
}

TEST_CASE ("Buffers que não são estéreo ficam intactos", "[13]")
{
    PingPongDelay delay;
    configure (delay, 10.0f, 0.5f, 1.0f, 1);

    auto buffer = makeNoise (1, 1024);
    const auto input = copyOf (buffer);
    processWhole (delay, buffer);

    REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
}

TEST_CASE ("process() não aloca", "[13]")
{
    PingPongDelay delay;
    configure (delay, 300.0f, 0.6f, 0.5f);

    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (delay, buffer, sizes); }) == 0);
}
