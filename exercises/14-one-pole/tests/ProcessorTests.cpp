#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <numbers>

using namespace learn::test;

namespace
{
    /** Ganho (dB) de um OnePoleFilter a uma frequência, medido com um seno em regime estacionário. */
    double measureGainDb (OnePoleFilter::Type type, float cutoff, double frequency, double fs = 48000.0)
    {
        OnePoleFilter filter;
        filter.setCutoff (cutoff, fs);
        filter.setType (type);

        auto input = makeSine (1, 48000, frequency, fs);
        auto output = copyOf (input);

        for (int n = 0; n < output.getNumSamples(); ++n)
            output.setSample (0, n, filter.processSample (output.getSample (0, n)));

        return gainDb (input, output, 0, 24000); // ignora o transitório inicial
    }
}

TEST_CASE ("O coeficiente é a = 1 − e^(−2π·fc/fs) (limitado a uma gama segura)", "[14]")
{
    OnePoleFilter f;

    f.setCutoff (1000.0f, 48000.0);
    REQUIRE_THAT (f.getCoefficient(), WithinAbs (1.0 - std::exp (-2.0 * std::numbers::pi * 1000.0 / 48000.0), 1.0e-6));

    f.setCutoff (0.0f, 48000.0); // abaixo de 1 Hz -> 1 Hz
    REQUIRE (f.getCoefficient() > 0.0f);

    f.setCutoff (1.0e6f, 48000.0); // acima de 0.49·fs -> 0.49·fs
    REQUIRE_THAT (f.getCoefficient(), WithinAbs (1.0 - std::exp (-2.0 * std::numbers::pi * 0.49), 1.0e-6));
}

TEST_CASE ("Passa-baixo: o DC passa inteiro; passa-alto: o DC desaparece", "[14]")
{
    OnePoleFilter lp, hp;
    lp.setCutoff (200.0f, 48000.0);
    hp.setCutoff (200.0f, 48000.0);
    hp.setType (OnePoleFilter::Type::highPass);

    float yLow = 0.0f, yHigh = 0.0f;

    for (int n = 0; n < 48000; ++n)
    {
        yLow = lp.processSample (1.0f);
        yHigh = hp.processSample (1.0f);
    }

    REQUIRE_THAT (yLow, WithinAbs (1.0, 1.0e-5));
    REQUIRE_THAT (yHigh, WithinAbs (0.0, 1.0e-5));
}

TEST_CASE ("Resposta ao degrau: 63.2% ao fim de uma constante de tempo τ = 1/(2π·fc)", "[14]")
{
    constexpr double fs = 48000.0, fc = 100.0;
    const auto tauSamples = fs / (2.0 * std::numbers::pi * fc); // ≈ 76.4 amostras

    OnePoleFilter f;
    f.setCutoff (static_cast<float> (fc), fs);

    std::vector<float> y;

    for (int n = 0; n < 1000; ++n)
        y.push_back (f.processSample (1.0f));

    // y[n] = 1 − (1 − a)^(n+1): a amostra n tem n+1 passos
    const auto n = static_cast<int> (std::round (tauSamples)) - 1;
    REQUIRE_THAT (y[(size_t) n], WithinAbs (1.0 - std::exp (-1.0), 0.01));
    REQUIRE (y[(size_t) n / 2] < 0.5f);
    REQUIRE (y[999] > 0.99f);
}

TEST_CASE ("Passa-baixo: ≈ −3 dB na frequência de corte e −6 dB por oitava acima", "[14]")
{
    REQUIRE_THAT (measureGainDb (OnePoleFilter::Type::lowPass, 200.0f, 50.0), WithinAbs (-0.26, 0.15));
    REQUIRE_THAT (measureGainDb (OnePoleFilter::Type::lowPass, 200.0f, 200.0), WithinAbs (-3.0, 0.2));
    REQUIRE_THAT (measureGainDb (OnePoleFilter::Type::lowPass, 200.0f, 2000.0), WithinAbs (-20.0, 1.0));
    REQUIRE_THAT (measureGainDb (OnePoleFilter::Type::lowPass, 200.0f, 4000.0), WithinAbs (-26.0, 1.2));
}

TEST_CASE ("Passa-alto: deixa passar as altas e corta as baixas", "[14]")
{
    REQUIRE_THAT (measureGainDb (OnePoleFilter::Type::highPass, 1000.0f, 10000.0), WithinAbs (0.0, 1.0));
    REQUIRE (measureGainDb (OnePoleFilter::Type::highPass, 1000.0f, 50.0) < -24.0);
}

TEST_CASE ("Passa-baixo + passa-alto = o sinal original", "[14]")
{
    OnePoleFilter lp, hp;
    lp.setCutoff (700.0f, 44100.0);
    hp.setCutoff (700.0f, 44100.0);
    hp.setType (OnePoleFilter::Type::highPass);

    const auto input = makeNoise (1, 4000);

    for (int n = 0; n < 4000; ++n)
    {
        const auto x = input.getSample (0, n);
        REQUIRE_THAT (lp.processSample (x) + hp.processSample (x), WithinAbs (x, 1.0e-6));
    }
}

TEST_CASE ("OnePoleProcessor: canais independentes, blocos de qualquer tamanho, sem alocar", "[14]")
{
    OnePoleProcessor reference;
    reference.setCutoff (300.0f);
    reference.prepare ({ 48000.0, 8192, 2 });
    auto expected = makeNoise (2, 8192);
    auto buffer = copyOf (expected);
    processWhole (reference, expected);

    OnePoleProcessor processor;
    processor.setCutoff (300.0f);
    processor.prepare ({ 48000.0, 512, 2 });

    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (processor, buffer, sizes); }) == 0);
    REQUIRE (maxAbsDifference (buffer, expected) < 1.0e-6f);
    REQUIRE (rms (buffer, 0) < 0.5 * rms (makeNoise (2, 8192), 0)); // filtrou de facto
}
