#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    /** Ganhos (dB) das três saídas a uma frequência, medidos com um seno. */
    std::array<double, 3> measure (float cutoff, float q, double f)
    {
        StateVariableFilter svf;
        svf.setParameters (cutoff, q, fs);

        auto input = makeSine (1, 48000, f, fs);
        juce::AudioBuffer<float> lp (1, 48000), bp (1, 48000), hp (1, 48000);

        for (int n = 0; n < 48000; ++n)
        {
            const auto out = svf.processSample (input.getSample (0, n));
            lp.setSample (0, n, out.lowPass);
            bp.setSample (0, n, out.bandPass);
            hp.setSample (0, n, out.highPass);
        }

        return { gainDb (input, lp, 0, 24000), gainDb (input, bp, 0, 24000), gainDb (input, hp, 0, 24000) };
    }
}

TEST_CASE ("Coeficientes: g = tan(π·fc/fs), R2 = 1/Q, com limites", "[16]")
{
    StateVariableFilter svf;
    svf.setParameters (1000.0f, 2.0f, fs);
    REQUIRE_THAT (svf.getR2(), WithinAbs (0.5, 1.0e-6));

    svf.setParameters (1000.0f, 0.0f, fs); // Q abaixo do mínimo
    REQUIRE_THAT (svf.getR2(), WithinAbs (10.0, 1.0e-4));
}

TEST_CASE ("Passa-baixo: 0 dB em DC e −3 dB em fc com Q = 0.707", "[16]")
{
    REQUIRE_THAT (measure (1000.0f, 0.7071f, 30.0)[0], WithinAbs (0.0, 0.05));
    REQUIRE_THAT (measure (1000.0f, 0.7071f, 1000.0)[0], WithinAbs (-3.01, 0.1));
    REQUIRE (measure (1000.0f, 0.7071f, 10000.0)[0] < -38.0); // 2.ª ordem: −12 dB/oitava
}

TEST_CASE ("Passa-alto e passa-banda", "[16]")
{
    const auto atCutoff = measure (2000.0f, 0.7071f, 2000.0);
    REQUIRE_THAT (atCutoff[2], WithinAbs (-3.01, 0.1));                 // HP em fc
    REQUIRE_THAT (atCutoff[1], WithinAbs (toDb (0.7071), 0.1));         // BP em fc: ganho Q

    const auto resonant = measure (2000.0f, 5.0f, 2000.0);
    REQUIRE_THAT (resonant[1], WithinAbs (toDb (5.0), 0.15));           // BP em fc: ganho Q
    REQUIRE (measure (2000.0f, 0.7071f, 50.0)[2] < -55.0);              // HP corta as baixas
}

TEST_CASE ("As saídas somam o original: x = HP + R2·BP + LP", "[16]")
{
    StateVariableFilter svf;
    svf.setParameters (1500.0f, 3.0f, fs);
    const auto input = makeNoise (1, 2000);

    for (int n = 0; n < 2000; ++n)
    {
        const auto x = input.getSample (0, n);
        const auto o = svf.processSample (x);
        REQUIRE_THAT (o.highPass + svf.getR2() * o.bandPass + o.lowPass, WithinAbs (x, 1.0e-5));
    }
}

TEST_CASE ("Igual ao juce::dsp::StateVariableTPTFilter, amostra a amostra", "[16]")
{
    using JuceType = juce::dsp::StateVariableTPTFilterType;

    for (const auto [type, index] : { std::pair { JuceType::lowpass, 0 }, std::pair { JuceType::bandpass, 1 },
                                      std::pair { JuceType::highpass, 2 } })
    {
        juce::dsp::StateVariableTPTFilter<float> theirs;
        theirs.setType (type);
        theirs.setCutoffFrequency (850.0f);
        theirs.setResonance (2.5f);
        theirs.prepare ({ fs, 512, 1 });

        StateVariableFilter mine;
        mine.setParameters (850.0f, 2.5f, fs);

        const auto input = makeNoise (1, 3000);

        for (int n = 0; n < 3000; ++n)
        {
            const auto x = input.getSample (0, n);
            const auto o = mine.processSample (x);
            const float mineOut[] = { o.lowPass, o.bandPass, o.highPass };
            INFO ("saída " << index << ", amostra " << n);
            REQUIRE_THAT (mineOut[index], WithinAbs (theirs.processSample (0, x), 1.0e-4));
        }
    }
}

TEST_CASE ("Estável com o corte a mudar em cada amostra (20 Hz – 20 kHz, Q = 20)", "[16]")
{
    StateVariableFilter svf;
    juce::Random random (11);
    const auto input = makeNoise (1, 96000, 1.0f, 5);
    float maxOut = 0.0f;
    bool finite = true;

    for (int n = 0; n < 96000; ++n)
    {
        const auto cutoff = 20.0f * std::pow (1000.0f, random.nextFloat()); // 20 Hz a 20 kHz
        svf.setParameters (cutoff, 20.0f, fs);
        const auto o = svf.processSample (input.getSample (0, n));
        finite = finite && std::isfinite (o.lowPass) && std::isfinite (o.bandPass) && std::isfinite (o.highPass);
        maxOut = juce::jmax (maxOut, std::abs (o.lowPass), std::abs (o.highPass));
    }

    REQUIRE (finite);
    REQUIRE (maxOut < 200.0f);
}

TEST_CASE ("SvfProcessor: modulação por amostra, blocos de qualquer tamanho, sem alocar", "[16]")
{
    SvfProcessor reference;
    reference.setParameters (SvfProcessor::Mode::lowPass, 800.0f, 4.0f, 2.0f, 3.0f);
    reference.prepare ({ fs, 8192, 2 });
    auto expected = makeNoise (2, 8192);
    auto buffer = copyOf (expected);
    processWhole (reference, expected);

    SvfProcessor processor;
    processor.setParameters (SvfProcessor::Mode::lowPass, 800.0f, 4.0f, 2.0f, 3.0f);
    processor.prepare ({ fs, 512, 2 });

    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (processor, buffer, sizes); }) == 0);
    REQUIRE (maxAbsDifference (buffer, expected) < 1.0e-5f);
    REQUIRE (allFinite (buffer));
}
