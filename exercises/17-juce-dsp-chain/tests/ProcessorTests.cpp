#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    double measureDb (BandFilterChain& chain, double frequency, int channels = 2)
    {
        chain.reset();
        auto input = makeSine (channels, 48000, frequency, fs);
        auto output = copyOf (input);
        processInBlocks (chain, output, { 480 });
        return gainDb (input, output, channels - 1, 24000);
    }
}

TEST_CASE ("A banda passa o meio e corta graves e agudos (−3 dB nos cortes)", "[17]")
{
    BandFilterChain chain;
    chain.setLowCut (200.0f);
    chain.setHighCut (5000.0f);
    chain.prepare ({ fs, 512, 2 });

    REQUIRE_THAT (measureDb (chain, 1000.0), WithinAbs (0.0, 0.3));
    REQUIRE_THAT (measureDb (chain, 200.0), WithinAbs (-3.0, 0.4));
    REQUIRE_THAT (measureDb (chain, 5000.0), WithinAbs (-3.0, 0.4));
    REQUIRE (measureDb (chain, 50.0) < -20.0);    // 2 oitavas abaixo: ~−24 dB
    REQUIRE (measureDb (chain, 20000.0) < -20.0); // 2 oitavas acima
}

TEST_CASE ("Os coeficientes são os de juce::dsp::IIR (passa-alto e passa-baixo, Q = 1/√2)", "[17]")
{
    BandFilterChain chain;
    chain.setLowCut (120.0f);
    chain.setHighCut (9000.0f);
    chain.prepare ({ fs, 512, 1 });

    const auto expectedHp = juce::dsp::IIR::Coefficients<float>::makeHighPass (fs, 120.0f);
    const auto expectedLp = juce::dsp::IIR::Coefficients<float>::makeLowPass (fs, 9000.0f);

    for (const double f : { 40.0, 120.0, 1000.0, 9000.0, 15000.0 })
    {
        INFO ("f = " << f);
        REQUIRE_THAT (chain.getLowCutCoefficients().getMagnitudeForFrequency (f, fs),
                      WithinAbs (expectedHp->getMagnitudeForFrequency (f, fs), 1.0e-5));
        REQUIRE_THAT (chain.getHighCutCoefficients().getMagnitudeForFrequency (f, fs),
                      WithinAbs (expectedLp->getMagnitudeForFrequency (f, fs), 1.0e-5));
    }
}

TEST_CASE ("O ganho da cadeia é aplicado", "[17]")
{
    BandFilterChain chain;
    chain.setLowCut (20.0f);
    chain.setHighCut (20000.0f);
    chain.setGainDecibels (-6.0f);
    chain.prepare ({ fs, 512, 2 });

    REQUIRE_THAT (measureDb (chain, 1000.0), WithinAbs (-6.0, 0.3));
}

TEST_CASE ("Todos os canais são filtrados (ProcessorDuplicator)", "[17]")
{
    BandFilterChain chain;
    chain.setLowCut (500.0f);
    chain.setHighCut (2000.0f);
    chain.prepare ({ fs, 512, 4 });

    for (int ch = 0; ch < 4; ++ch)
    {
        chain.reset();
        auto input = makeSine (4, 24000, 8000.0, fs);
        auto output = copyOf (input);
        processInBlocks (chain, output, { 480 });
        INFO ("canal " << ch);
        REQUIRE (gainDb (input, output, ch, 12000) < -15.0);
    }
}

TEST_CASE ("Mudar os cortes em cada bloco não aloca memória", "[17]")
{
    BandFilterChain chain;
    chain.setLowCut (100.0f);
    chain.setHighCut (8000.0f);
    chain.prepare ({ fs, 512, 2 });

    auto buffer = makeNoise (2, 512);

    const auto allocations = countAllocations ([&]
    {
        for (int i = 0; i < 200; ++i)
        {
            chain.setLowCut (100.0f + static_cast<float> (i));
            chain.setHighCut (8000.0f - 10.0f * static_cast<float> (i));
            chain.setGainDecibels (static_cast<float> (i % 7) - 3.0f);
            processWhole (chain, buffer);
        }
    });

    REQUIRE (allocations == 0);
    REQUIRE (allFinite (buffer));

    // E os coeficientes acompanharam a última mudança
    REQUIRE_THAT (toDb (chain.getLowCutCoefficients().getMagnitudeForFrequency (299.0, fs)), WithinAbs (-3.0, 0.05));
}
