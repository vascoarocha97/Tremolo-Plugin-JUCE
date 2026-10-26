#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>

using namespace learn::test;

namespace
{
    Flanger makeFlanger (float minMs, float maxMs, float feedback, float mix, float rate = 0.5f)
    {
        Flanger flanger;
        flanger.setRate (rate);
        flanger.setDelayRangeMs (minMs, maxMs);
        flanger.setFeedback (feedback);
        flanger.setMix (mix);
        flanger.prepare ({ 48000.0, 512, 2 });
        return flanger;
    }

    double gainAt (Flanger& flanger, double frequency)
    {
        flanger.reset();
        auto input = makeSine (2, 48000, frequency, 48000.0);
        auto output = copyOf (input);
        processInBlocks (flanger, output, { 480 });
        return gainDb (input, output, 0, 24000);
    }
}

TEST_CASE ("Atraso fixo de 1 ms com mix 0.5: vales em 500 Hz, 1500 Hz... e picos em 1 kHz, 2 kHz...", "[21]")
{
    auto flanger = makeFlanger (1.0f, 1.0f, 0.0f, 0.5f);

    // y = 0.5·x + 0.5·x[n − D]: as duas metades cancelam-se quando D é meio período
    REQUIRE (gainAt (flanger, 500.0) < -30.0);
    REQUIRE (gainAt (flanger, 1500.0) < -30.0);
    REQUIRE_THAT (gainAt (flanger, 1000.0), WithinAbs (0.0, 0.3));
    REQUIRE_THAT (gainAt (flanger, 2000.0), WithinAbs (0.0, 0.3));
}

TEST_CASE ("Feedback positivo realça os picos; negativo troca picos e vales", "[21]")
{
    auto positive = makeFlanger (1.0f, 1.0f, 0.7f, 0.5f);
    auto negative = makeFlanger (1.0f, 1.0f, -0.7f, 0.5f);

    // H = (1 − m) + m·z⁻ᴰ / (1 − fb·z⁻ᴰ). Com fb = 0.7: +6.7 dB em 1 kHz, −13.7 dB em 500 Hz.
    REQUIRE (gainAt (positive, 1000.0) > 6.0);
    REQUIRE (gainAt (positive, 500.0) < -12.0);

    // Com fb = −0.7 a frequência de 500 Hz passa a ser mais forte do que a de 1 kHz
    REQUIRE (gainAt (negative, 500.0) > gainAt (negative, 1000.0) + 2.0);
}

TEST_CASE ("O LFO varre o atraso entre o mínimo e o máximo", "[21]")
{
    auto flanger = makeFlanger (0.5f, 6.0f, 0.3f, 0.5f, 1.0f);
    juce::AudioBuffer<float> one (2, 1);
    std::vector<float> delays;

    for (int n = 0; n < 48000; ++n)
    {
        one.clear();
        processWhole (flanger, one);
        delays.push_back (flanger.getLastDelaySamples (0));
    }

    const auto [lo, hi] = std::minmax_element (delays.begin(), delays.end());
    REQUIRE_THAT (*lo, WithinAbs (0.5 * 48.0, 1.0));
    REQUIRE_THAT (*hi, WithinAbs (6.0 * 48.0, 1.0));
}

TEST_CASE ("Feedback limitado a ±0.95 e saída estável", "[21]")
{
    auto flanger = makeFlanger (0.2f, 3.0f, 5.0f, 0.5f, 0.3f);
    auto buffer = makeNoise (2, 48000 * 4, 1.0f);
    processInBlocks (flanger, buffer, { 512 });

    REQUIRE (allFinite (buffer));
    REQUIRE (peak (buffer, 0) < 25.0f);
}

TEST_CASE ("Mix 0 deixa o sinal igual; process() não aloca", "[21]")
{
    auto flanger = makeFlanger (1.0f, 5.0f, 0.5f, 0.0f);
    auto buffer = makeNoise (2, 8192);
    const auto input = copyOf (buffer);
    const auto sizes = awkwardBlockSizes();

    REQUIRE (countAllocations ([&] { processInBlocks (flanger, buffer, sizes); }) == 0);
    REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
}
