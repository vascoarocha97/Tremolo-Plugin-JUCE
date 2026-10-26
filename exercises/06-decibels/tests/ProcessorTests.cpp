#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

TEST_CASE ("decibelsToGain: valores de referência", "[06]")
{
    REQUIRE_THAT (Gain::decibelsToGain (0.0f), WithinAbs (1.0, 1.0e-6));
    REQUIRE_THAT (Gain::decibelsToGain (-6.0206f), WithinAbs (0.5, 1.0e-4));
    REQUIRE_THAT (Gain::decibelsToGain (6.0206f), WithinAbs (2.0, 1.0e-3));
    REQUIRE_THAT (Gain::decibelsToGain (-20.0f), WithinAbs (0.1, 1.0e-6));
    REQUIRE_THAT (Gain::decibelsToGain (20.0f), WithinAbs (10.0, 1.0e-4));
    REQUIRE_THAT (Gain::decibelsToGain (-60.0f), WithinAbs (0.001, 1.0e-7));
}

TEST_CASE ("decibelsToGain: −∞ dB é silêncio", "[06]")
{
    REQUIRE (Gain::decibelsToGain (-100.0f) == 0.0f);
    REQUIRE (Gain::decibelsToGain (-250.0f) == 0.0f);
    REQUIRE (Gain::decibelsToGain (-99.0f) > 0.0f);
}

TEST_CASE ("gainToDecibels: valores de referência e piso", "[06]")
{
    REQUIRE_THAT (Gain::gainToDecibels (1.0f), WithinAbs (0.0, 1.0e-5));
    REQUIRE_THAT (Gain::gainToDecibels (0.5f), WithinAbs (-6.0206, 1.0e-3));
    REQUIRE_THAT (Gain::gainToDecibels (0.1f), WithinAbs (-20.0, 1.0e-4));
    REQUIRE_THAT (Gain::gainToDecibels (2.0f), WithinAbs (6.0206, 1.0e-3));
    REQUIRE (Gain::gainToDecibels (0.0f) == Gain::minusInfinityDb);
    REQUIRE (Gain::gainToDecibels (-0.5f) == Gain::minusInfinityDb);
    REQUIRE (Gain::gainToDecibels (1.0e-9f) == Gain::minusInfinityDb);
}

TEST_CASE ("As tuas funções dão o mesmo que juce::Decibels", "[06]")
{
    for (float db = -99.0f; db <= 24.0f; db += 0.37f)
    {
        INFO ("dB = " << db);
        REQUIRE_THAT (Gain::decibelsToGain (db), WithinRel (juce::Decibels::decibelsToGain (db), 1.0e-5f));
    }

    for (float gain = 0.00002f; gain <= 16.0f; gain *= 1.37f)
    {
        INFO ("ganho = " << gain);
        REQUIRE_THAT (Gain::gainToDecibels (gain), WithinAbs (juce::Decibels::gainToDecibels (gain), 1.0e-4));
    }
}

TEST_CASE ("Somar dB é multiplicar ganhos", "[06]")
{
    const auto a = Gain::decibelsToGain (-3.0f);
    const auto b = Gain::decibelsToGain (-9.0f);
    REQUIRE_THAT (a * b, WithinRel (Gain::decibelsToGain (-12.0f), 1.0e-5f));
}

TEST_CASE ("Potência: duplicar a potência são +3 dB, duplicar a amplitude são +6 dB", "[06]")
{
    REQUIRE_THAT (Gain::powerRatioToDecibels (2.0f), WithinAbs (3.0103, 1.0e-3));
    REQUIRE_THAT (Gain::powerRatioToDecibels (100.0f), WithinAbs (20.0, 1.0e-4));
    REQUIRE_THAT (Gain::gainToDecibels (2.0f), WithinAbs (2.0 * Gain::powerRatioToDecibels (2.0f), 1.0e-3));
    REQUIRE (Gain::powerRatioToDecibels (0.0f) == Gain::minusInfinityDb);
}

TEST_CASE ("DecibelGainProcessor aplica o ganho em todos os canais", "[06]")
{
    DecibelGainProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    processor.setGainDecibels (-12.0f);

    auto buffer = makeConstant (2, 256, 1.0f);
    processInBlocks (processor, buffer, awkwardBlockSizes());

    const auto expected = std::pow (10.0, -12.0 / 20.0);

    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 256; n += 17)
            REQUIRE_THAT (buffer.getSample (ch, n), WithinAbs (expected, 1.0e-6));
}

TEST_CASE ("DecibelGainProcessor: −∞ dB silencia e 0 dB não muda nada", "[06]")
{
    DecibelGainProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto buffer = makeNoise (2, 512);
    const auto input = copyOf (buffer);

    processor.setGainDecibels (0.0f);
    processor.processBlock (buffer);
    REQUIRE (maxAbsDifference (buffer, input) < 1.0e-6f);

    processor.setGainDecibels (-120.0f);
    processor.processBlock (buffer);
    REQUIRE (peak (buffer, 0) == 0.0f);
    REQUIRE (peak (buffer, 1) == 0.0f);
}
