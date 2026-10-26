#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    template <typename Filter>
    std::vector<float> impulseResponse (Filter& f, int length)
    {
        std::vector<float> h;

        for (int n = 0; n < length; ++n)
            h.push_back (f.processSample (n == 0 ? 1.0f : 0.0f));

        return h;
    }

    template <typename Filter>
    double gainDbAt (Filter& f, double frequency, double fs = 48000.0)
    {
        f.reset();
        auto input = makeSine (1, 48000, frequency, fs);
        auto output = copyOf (input);

        for (int n = 0; n < 48000; ++n)
            output.setSample (0, n, f.processSample (output.getSample (0, n)));

        return gainDb (input, output, 0, 24000);
    }
}

TEST_CASE ("Comb: ecos em D, 2D, 3D com ganhos 1, g, g²", "[27]")
{
    CombFilter comb;
    comb.prepare (100);
    comb.setFeedback (0.7f);
    comb.setDamping (0.0f);

    REQUIRE (comb.getDelaySamples() == 100);

    const auto h = impulseResponse (comb, 401);
    REQUIRE (h[0] == 0.0f);
    REQUIRE_THAT (h[100], WithinAbs (1.0, 1.0e-6));
    REQUIRE_THAT (h[200], WithinAbs (0.7, 1.0e-6));
    REQUIRE_THAT (h[300], WithinAbs (0.49, 1.0e-6));
    REQUIRE_THAT (h[400], WithinAbs (0.343, 1.0e-6));
    REQUIRE (h[150] == 0.0f);
}

TEST_CASE ("Comb com damping: os ecos ficam mais escuros (menos agudos) a cada volta", "[27]")
{
    CombFilter dark;
    dark.prepare (97);
    dark.setFeedback (0.8f);
    dark.setDamping (0.5f);

    // Os picos do pente estão em múltiplos de fs/D ≈ 494.8 Hz: compara um pico grave com um agudo
    const auto peakLow = gainDbAt (dark, 48000.0 / 97.0);
    const auto peakHigh = gainDbAt (dark, 30 * 48000.0 / 97.0); // ~14.8 kHz
    REQUIRE (peakLow > peakHigh + 6.0);

    // Sem damping, os picos têm todos a mesma altura: 1/(1 − g)
    CombFilter bright;
    bright.prepare (97);
    bright.setFeedback (0.8f);
    bright.setDamping (0.0f);
    REQUIRE_THAT (gainDbAt (bright, 30 * 48000.0 / 97.0), WithinAbs (toDb (1.0 / (1.0 - 0.8)), 0.3));
}

TEST_CASE ("Allpass: resposta a um impulso −g, ..., 1 − g², ..., g·(1 − g²)", "[27]")
{
    SchroederAllpass ap;
    ap.prepare (50);
    ap.setGain (0.6f);

    const auto h = impulseResponse (ap, 151);
    REQUIRE_THAT (h[0], WithinAbs (-0.6, 1.0e-6));
    REQUIRE_THAT (h[50], WithinAbs (1.0 - 0.36, 1.0e-6));
    REQUIRE_THAT (h[100], WithinAbs (0.6 * (1.0 - 0.36), 1.0e-6));
    REQUIRE (h[25] == 0.0f);

    // Energia total da resposta = 1 (um allpass não ganha nem perde energia)
    SchroederAllpass ap2;
    ap2.prepare (50);
    ap2.setGain (0.6f);
    double energy = 0.0;
    for (const auto v : impulseResponse (ap2, 20000))
        energy += static_cast<double> (v) * v;

    REQUIRE_THAT (energy, WithinAbs (1.0, 1.0e-3));
}

TEST_CASE ("Allpass: ganho de 0 dB a qualquer frequência", "[27]")
{
    SchroederAllpass ap;
    ap.prepare (73);
    ap.setGain (0.7f);

    for (const double f : { 50.0, 333.0, 1000.0, 4567.0, 15000.0 })
    {
        INFO ("f = " << f);
        REQUIRE_THAT (gainDbAt (ap, f), WithinAbs (0.0, 0.1));
    }
}

TEST_CASE ("CombAllpassDemo: canais independentes, blocos de qualquer tamanho, sem alocar", "[27]")
{
    CombAllpassDemo reference, demo;
    reference.setParameters (CombAllpassDemo::Mode::comb, 7.0f, 0.75f, 0.2f);
    demo.setParameters (CombAllpassDemo::Mode::comb, 7.0f, 0.75f, 0.2f);
    reference.prepare ({ 48000.0, 8192, 2 });
    demo.prepare ({ 48000.0, 512, 2 });

    auto expected = makeNoise (2, 8192);
    auto buffer = copyOf (expected);
    processWhole (reference, expected);

    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (demo, buffer, sizes); }) == 0);
    REQUIRE (maxAbsDifference (buffer, expected) < 1.0e-6f);
}
