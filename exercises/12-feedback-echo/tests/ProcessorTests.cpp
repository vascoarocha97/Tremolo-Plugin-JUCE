#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    FeedbackEcho makeEcho (float delayMs, float feedback, float mix, int channels = 2)
    {
        FeedbackEcho echo;
        echo.setDelayMilliseconds (delayMs);
        echo.setFeedback (feedback);
        echo.setMix (mix);
        echo.prepare ({ 48000.0, 512, static_cast<juce::uint32> (channels) });
        return echo;
    }
}

TEST_CASE ("Ecos a cada 480 amostras (10 ms), cada um multiplicado pelo feedback", "[12]")
{
    auto echo = makeEcho (10.0f, 0.5f, 1.0f); // mix = 1: só ecos

    auto buffer = makeImpulse (2, 3000, 0);
    processInBlocks (echo, buffer, awkwardBlockSizes());

    for (int ch = 0; ch < 2; ++ch)
    {
        REQUIRE_THAT (buffer.getSample (ch, 0), WithinAbs (0.0, 1.0e-6)); // sem dry
        REQUIRE_THAT (buffer.getSample (ch, 480), WithinAbs (1.0, 1.0e-3));
        REQUIRE_THAT (buffer.getSample (ch, 960), WithinAbs (0.5, 1.0e-3));
        REQUIRE_THAT (buffer.getSample (ch, 1440), WithinAbs (0.25, 1.0e-3));
        REQUIRE_THAT (buffer.getSample (ch, 1920), WithinAbs (0.125, 1.0e-3));
        REQUIRE (peak (buffer, ch, 1, 478) < 1.0e-3f); // nada entre o impulso e o 1.º eco
    }
}

TEST_CASE ("Mix: 0 dá exatamente o sinal original, 0.5 dá metade de cada", "[12]")
{
    {
        auto echo = makeEcho (10.0f, 0.9f, 0.0f);
        auto buffer = makeNoise (2, 4096);
        const auto input = copyOf (buffer);
        processInBlocks (echo, buffer, awkwardBlockSizes());
        REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
    }

    {
        auto echo = makeEcho (10.0f, 0.0f, 0.5f);
        auto buffer = makeImpulse (1, 1000, 0);
        processWhole (echo, buffer);
        REQUIRE_THAT (buffer.getSample (0, 0), WithinAbs (0.5, 1.0e-6));
        REQUIRE_THAT (buffer.getSample (0, 480), WithinAbs (0.5, 1.0e-3));
    }
}

TEST_CASE ("O feedback é limitado a 0.95", "[12]")
{
    auto echo = makeEcho (10.0f, 1.5f, 1.0f);
    auto buffer = makeImpulse (1, 2000, 0);
    processWhole (echo, buffer);

    REQUIRE_THAT (buffer.getSample (0, 960) / buffer.getSample (0, 480), WithinAbs (0.95, 1.0e-3));
}

TEST_CASE ("Com feedback máximo e ruído forte, a saída fica limitada (estável)", "[12]")
{
    auto echo = makeEcho (37.0f, 1.0f, 1.0f, 1);
    auto buffer = makeNoise (1, 48000 * 10, 1.0f);
    processInBlocks (echo, buffer, { 512 });

    REQUIRE (allFinite (buffer));
    REQUIRE (peak (buffer, 0) < 25.0f); // ganho máximo de um ciclo com fb=0.95 é 1/(1-0.95) = 20
}

TEST_CASE ("reset() apaga os ecos pendentes", "[12]")
{
    auto echo = makeEcho (10.0f, 0.8f, 1.0f);
    auto buffer = makeImpulse (2, 600, 0);
    processWhole (echo, buffer);

    echo.reset();
    auto silence = makeConstant (2, 5000, 0.0f);
    processWhole (echo, silence);

    REQUIRE (peak (silence, 0) == 0.0f);
    REQUIRE (peak (silence, 1) == 0.0f);
}

TEST_CASE ("Canais independentes", "[12]")
{
    auto echo = makeEcho (10.0f, 0.5f, 1.0f);
    auto buffer = makeImpulse (2, 1000, 0);
    buffer.setSample (1, 0, 0.0f); // só o canal 0 tem impulso
    processWhole (echo, buffer);

    REQUIRE (peak (buffer, 0) > 0.9f);
    REQUIRE (peak (buffer, 1) == 0.0f);
}

TEST_CASE ("Mudar o tempo de delay não faz clicks", "[12]")
{
    auto echo = makeEcho (100.0f, 0.3f, 1.0f, 1);
    auto buffer = makeSine (1, 48000, 300.0, 48000.0, 0.5f);

    for (int block = 0; block < 100; ++block)
    {
        echo.setDelayMilliseconds (block % 20 < 10 ? 100.0f : 180.0f);
        processRange (echo, buffer, block * 480, 480);
    }

    REQUIRE (maxSampleJump (buffer, 0, 5000) < 0.1f);
}

TEST_CASE ("process() não aloca", "[12]")
{
    auto echo = makeEcho (250.0f, 0.5f, 0.5f);
    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (echo, buffer, sizes); }) == 0);
}
