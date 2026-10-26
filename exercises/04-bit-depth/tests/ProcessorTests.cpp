#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <set>

using namespace learn::test;

namespace
{
    /** SNR (dB) entre o sinal original e o erro (processado - original). */
    double measureSnrDb (const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed)
    {
        double signal = 0.0, error = 0.0;

        for (int n = 0; n < original.getNumSamples(); ++n)
        {
            const double x = original.getSample (0, n);
            const double e = processed.getSample (0, n) - x;
            signal += x * x;
            error += e * e;
        }

        return 10.0 * std::log10 (signal / std::max (error, 1.0e-30));
    }
}

TEST_CASE ("stepSize: 1 LSB para vários bit depths", "[04]")
{
    REQUIRE_THAT (Quantiser::stepSize (1), WithinAbs (1.0, 1.0e-12));
    REQUIRE_THAT (Quantiser::stepSize (8), WithinAbs (1.0 / 128.0, 1.0e-12));
    REQUIRE_THAT (Quantiser::stepSize (16), WithinAbs (1.0 / 32768.0, 1.0e-15));
    REQUIRE_THAT (Quantiser::stepSize (24), WithinAbs (1.0 / 8388608.0, 1.0e-18));
}

TEST_CASE ("quantise arredonda para o nível mais próximo (o zero é um nível)", "[04]")
{
    const auto lsb = static_cast<float> (Quantiser::stepSize (8));

    REQUIRE (Quantiser::quantise (0.0f, 8) == 0.0f);
    REQUIRE (Quantiser::quantise (0.49f * lsb, 8) == 0.0f);
    REQUIRE_THAT (Quantiser::quantise (0.51f * lsb, 8), WithinAbs (lsb, 1.0e-7));
    REQUIRE_THAT (Quantiser::quantise (-0.51f * lsb, 8), WithinAbs (-lsb, 1.0e-7));
    REQUIRE_THAT (Quantiser::quantise (10.2f * lsb, 8), WithinAbs (10.0f * lsb, 1.0e-6));
}

TEST_CASE ("quantise limita ao intervalo representável [-1, 1 - LSB]", "[04]")
{
    const auto lsb = static_cast<float> (Quantiser::stepSize (8));

    REQUIRE_THAT (Quantiser::quantise (1.0f, 8), WithinAbs (1.0f - lsb, 1.0e-7));
    REQUIRE_THAT (Quantiser::quantise (3.0f, 8), WithinAbs (1.0f - lsb, 1.0e-7));
    REQUIRE_THAT (Quantiser::quantise (-1.0f, 8), WithinAbs (-1.0f, 1.0e-7));
    REQUIRE_THAT (Quantiser::quantise (-3.0f, 8), WithinAbs (-1.0f, 1.0e-7));
}

TEST_CASE ("O erro de quantização nunca passa de meio LSB", "[04]")
{
    const auto bits = GENERATE (3, 8, 12, 16);
    const auto lsb = Quantiser::stepSize (bits);
    INFO ("bits: " << bits);

    juce::Random random (99);

    for (int i = 0; i < 20000; ++i)
    {
        const auto x = static_cast<float> ((2.0 - lsb) * random.nextDouble() - 1.0);
        const auto error = std::abs (static_cast<double> (Quantiser::quantise (x, bits)) - x);
        INFO ("x = " << x);
        REQUIRE (error <= 0.5 * lsb + 1.0e-7);
    }
}

TEST_CASE ("Com 3 bits há exatamente 8 níveis", "[04]")
{
    std::set<float> levels;

    for (int i = 0; i <= 10000; ++i)
        levels.insert (Quantiser::quantise (-1.0f + 2.0f * static_cast<float> (i) / 10000.0f, 3));

    REQUIRE (levels.size() == 8);
}

TEST_CASE ("SNR de um seno em escala completa ≈ 6.02·N + 1.76 dB", "[04]")
{
    const auto bits = GENERATE (8, 12, 16);
    INFO ("bits: " << bits);

    // Amplitude máxima sem tocar no limite (1 - LSB); a fórmula assume amplitude 1, daí a correção.
    const auto amplitude = 1.0 - Quantiser::stepSize (bits);
    const auto original = makeSine (1, 48000, 997.0, 48000.0, static_cast<float> (amplitude));
    auto processed = copyOf (original);

    BitDepthReducer reducer;
    reducer.prepareToPlay (48000.0, 48000);
    reducer.setBits (bits);
    reducer.setDitherEnabled (false);
    reducer.processBlock (processed);

    const auto expected = 6.02 * bits + 1.76 + 20.0 * std::log10 (amplitude);
    REQUIRE_THAT (measureSnrDb (original, processed), WithinAbs (expected, 1.0));
}

TEST_CASE ("Sem dither, um sinal abaixo de meio LSB desaparece", "[04]")
{
    const auto lsb = static_cast<float> (Quantiser::stepSize (8));
    auto buffer = makeSine (1, 48000, 440.0, 48000.0, 0.3f * lsb);

    BitDepthReducer reducer;
    reducer.prepareToPlay (48000.0, 48000);
    reducer.setBits (8);
    reducer.setDitherEnabled (false);
    reducer.processBlock (buffer);

    REQUIRE (peak (buffer, 0) == 0.0f);
}

TEST_CASE ("Com dither TPDF, o mesmo sinal sobrevive (enterrado em ruído)", "[04]")
{
    const auto lsb = Quantiser::stepSize (8);
    const auto amplitude = 0.3 * lsb;
    const auto original = makeSine (1, 96000, 440.0, 48000.0, static_cast<float> (amplitude));
    auto buffer = copyOf (original);

    BitDepthReducer reducer;
    reducer.prepareToPlay (48000.0, 96000);
    reducer.setBits (8);
    reducer.setDitherEnabled (true);
    reducer.processBlock (buffer);

    // O seno continua lá, com a mesma amplitude (em média)...
    REQUIRE_THAT (amplitudeAt (buffer, 0, 440.0, 48000.0), WithinRel (amplitude, 0.35));

    // ...e o erro total é ruído sem DC, com potência ≈ LSB²/4 (dither LSB²/6 + quantização LSB²/12).
    juce::AudioBuffer<float> error (1, buffer.getNumSamples());

    for (int n = 0; n < buffer.getNumSamples(); ++n)
        error.setSample (0, n, buffer.getSample (0, n) - original.getSample (0, n));

    REQUIRE (std::abs (mean (error, 0)) < 0.05 * lsb);
    REQUIRE_THAT (rms (error, 0) * rms (error, 0), WithinRel (lsb * lsb / 4.0, 0.2));
}

TEST_CASE ("BitDepthReducer trata todos os canais e não aloca", "[04]")
{
    BitDepthReducer reducer;
    reducer.prepareToPlay (48000.0, 512);
    reducer.setBits (4);
    reducer.setDitherEnabled (true);

    auto buffer = makeNoise (2, 2048, 0.9f);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (reducer, buffer, sizes); }) == 0);

    std::set<float> levels;

    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 2048; ++n)
            levels.insert (buffer.getSample (ch, n));

    REQUIRE (levels.size() <= 16); // 4 bits => no máximo 16 níveis
    REQUIRE (levels.size() >= 8);
}
