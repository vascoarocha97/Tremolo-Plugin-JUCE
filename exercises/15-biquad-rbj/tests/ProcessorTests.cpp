#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    double magDb (const BiquadCoefficients& c, double f) { return toDb (Rbj::magnitudeAt (c, f, fs)); }

    /** Ganho medido passando um seno por um Biquad com estes coeficientes. */
    double measuredDb (const BiquadCoefficients& c, double f)
    {
        Biquad b;
        b.setCoefficients (c);
        auto input = makeSine (1, 48000, f, fs);
        auto output = copyOf (input);

        for (int n = 0; n < output.getNumSamples(); ++n)
            output.setSample (0, n, b.processSample (output.getSample (0, n)));

        return gainDb (input, output, 0, 24000);
    }
}

TEST_CASE ("magnitudeAt: um biquad \"fio\" (b0 = 1) tem ganho 1 em todo o lado", "[15]")
{
    const BiquadCoefficients identity;

    for (const double f : { 20.0, 1000.0, 23000.0 })
        REQUIRE_THAT (Rbj::magnitudeAt (identity, f, fs), WithinAbs (1.0, 1.0e-9));

    // b0 = b1 = 0.5 é a média de duas amostras: ganho 1 em DC e 0 em Nyquist
    const BiquadCoefficients average { 0.5, 0.5, 0.0, 0.0, 0.0 };
    REQUIRE_THAT (Rbj::magnitudeAt (average, 0.0, fs), WithinAbs (1.0, 1.0e-9));
    REQUIRE_THAT (Rbj::magnitudeAt (average, fs / 2, fs), WithinAbs (0.0, 1.0e-9));
}

TEST_CASE ("Passa-baixo: 0 dB em DC, |H(f0)| = Q, nada em Nyquist", "[15]")
{
    const auto lp = Rbj::lowPass (fs, 1000.0, 0.7071);
    REQUIRE_THAT (magDb (lp, 1.0), WithinAbs (0.0, 0.01));
    REQUIRE_THAT (magDb (lp, 1000.0), WithinAbs (-3.01, 0.02));
    REQUIRE (magDb (lp, 23999.0) < -100.0);

    const auto resonant = Rbj::lowPass (fs, 2000.0, 4.0);
    REQUIRE_THAT (Rbj::magnitudeAt (resonant, 2000.0, fs), WithinRel (4.0, 0.001));
}

TEST_CASE ("Passa-alto: o espelho do passa-baixo", "[15]")
{
    const auto hp = Rbj::highPass (fs, 1000.0, 0.7071);
    REQUIRE (magDb (hp, 1.0) < -100.0);
    REQUIRE_THAT (magDb (hp, 1000.0), WithinAbs (-3.01, 0.02));
    REQUIRE_THAT (magDb (hp, 23999.0), WithinAbs (0.0, 0.01));
}

TEST_CASE ("Passa-banda (0 dB no pico) e notch", "[15]")
{
    const auto bp = Rbj::bandPass (fs, 3000.0, 2.0);
    REQUIRE_THAT (magDb (bp, 3000.0), WithinAbs (0.0, 0.01));
    REQUIRE (magDb (bp, 300.0) < -20.0);

    const auto notch = Rbj::notch (fs, 3000.0, 2.0);
    REQUIRE (magDb (notch, 3000.0) < -100.0);
    REQUIRE_THAT (magDb (notch, 20.0), WithinAbs (0.0, 0.01));
}

TEST_CASE ("Peaking: o ganho pedido exatamente em f0, 0 dB longe", "[15]")
{
    for (const double gain : { -12.0, -3.0, 6.0, 15.0 })
    {
        INFO ("ganho " << gain << " dB");
        const auto peak = Rbj::peaking (fs, 800.0, 1.4, gain);
        REQUIRE_THAT (magDb (peak, 800.0), WithinAbs (gain, 0.01));
        REQUIRE_THAT (magDb (peak, 5.0), WithinAbs (0.0, 0.05));
    }
}

TEST_CASE ("Shelves: o ganho total de um lado, metade em f0, 0 dB do outro lado", "[15]")
{
    const auto low = Rbj::lowShelf (fs, 300.0, 0.7071, 9.0);
    REQUIRE_THAT (magDb (low, 2.0), WithinAbs (9.0, 0.05));
    REQUIRE_THAT (magDb (low, 300.0), WithinAbs (4.5, 0.05));
    REQUIRE_THAT (magDb (low, 23000.0), WithinAbs (0.0, 0.05));

    const auto high = Rbj::highShelf (fs, 4000.0, 0.7071, -6.0);
    REQUIRE_THAT (magDb (high, 23999.0), WithinAbs (-6.0, 0.05));
    REQUIRE_THAT (magDb (high, 4000.0), WithinAbs (-3.0, 0.05));
    REQUIRE_THAT (magDb (high, 2.0), WithinAbs (0.0, 0.05));
}

TEST_CASE ("As tuas fórmulas dão a mesma resposta que juce::dsp::IIR::Coefficients", "[15]")
{
    using JuceCoefficients = juce::dsp::IIR::Coefficients<double>;
    using JuceArray = juce::dsp::IIR::ArrayCoefficients<double>;

    struct Case
    {
        const char* name;
        BiquadCoefficients mine;
        JuceCoefficients theirs;
    };

    const Case cases[] = {
        { "lowPass", Rbj::lowPass (fs, 700.0, 1.3), JuceCoefficients (JuceArray::makeLowPass (fs, 700.0, 1.3)) },
        { "highPass", Rbj::highPass (fs, 120.0, 0.5), JuceCoefficients (JuceArray::makeHighPass (fs, 120.0, 0.5)) },
        { "notch", Rbj::notch (fs, 5000.0, 3.0), JuceCoefficients (JuceArray::makeNotch (fs, 5000.0, 3.0)) },
        { "peaking", Rbj::peaking (fs, 2500.0, 0.9, 7.5),
          JuceCoefficients (JuceArray::makePeakFilter (fs, 2500.0, 0.9, juce::Decibels::decibelsToGain (7.5))) },
        { "lowShelf", Rbj::lowShelf (fs, 200.0, 0.7, -8.0),
          JuceCoefficients (JuceArray::makeLowShelf (fs, 200.0, 0.7, juce::Decibels::decibelsToGain (-8.0))) },
        { "highShelf", Rbj::highShelf (fs, 6000.0, 0.7, 4.0),
          JuceCoefficients (JuceArray::makeHighShelf (fs, 6000.0, 0.7, juce::Decibels::decibelsToGain (4.0))) },
    };

    for (const auto& c : cases)
    {
        for (double f = 20.0; f < 23000.0; f *= 1.3)
        {
            INFO (c.name << " a " << f << " Hz");
            REQUIRE_THAT (Rbj::magnitudeAt (c.mine, f, fs),
                          WithinRel (c.theirs.getMagnitudeForFrequency (f, fs), 1.0e-6));
        }
    }
}

TEST_CASE ("Biquad (TDF-II): resposta a um impulso e ganho medido = magnitudeAt", "[15]")
{
    const auto c = Rbj::peaking (fs, 1000.0, 1.0, 6.0);

    Biquad b;
    b.setCoefficients (c);

    // h[0] = b0, h[1] = b1 − a1·b0, h[2] = b2 − a1·h[1] − a2·h[0]
    const auto h0 = b.processSample (1.0f);
    const auto h1 = b.processSample (0.0f);
    const auto h2 = b.processSample (0.0f);
    REQUIRE_THAT (h0, WithinAbs (c.b0, 1.0e-6));
    REQUIRE_THAT (h1, WithinAbs (c.b1 - c.a1 * c.b0, 1.0e-6));
    REQUIRE_THAT (h2, WithinAbs (c.b2 - c.a1 * h1 - c.a2 * h0, 1.0e-6));

    for (const double f : { 100.0, 1000.0, 7000.0 })
    {
        INFO ("f = " << f);
        REQUIRE_THAT (measuredDb (c, f), WithinAbs (magDb (c, f), 0.1));
    }
}

TEST_CASE ("BiquadFilter: processa todos os canais, recalcula só quando muda, não aloca", "[15]")
{
    BiquadFilter filter;
    filter.setParameters (BiquadFilter::Type::lowPass, 500.0f, 0.7071f, 0.0f);
    filter.prepare ({ fs, 512, 2 });

    REQUIRE_THAT (Rbj::magnitudeAt (filter.getCoefficients(), 500.0, fs), WithinAbs (0.7071, 1.0e-3));

    auto buffer = makeSine (2, 9600, 8000.0, fs);
    const auto sizes = awkwardBlockSizes();

    const auto allocations = countAllocations ([&]
    {
        filter.setParameters (BiquadFilter::Type::lowPass, 500.0f, 0.7071f, 0.0f);
        processInBlocks (filter, buffer, sizes);
    });

    REQUIRE (allocations == 0);
    REQUIRE (rms (buffer, 0, 4800) < 0.02);
    REQUIRE (rms (buffer, 1, 4800) < 0.02);

    filter.setParameters (BiquadFilter::Type::highShelf, 2000.0f, 0.7071f, 12.0f);
    REQUIRE_THAT (toDb (Rbj::magnitudeAt (filter.getCoefficients(), 23000.0, fs)), WithinAbs (12.0, 0.1));
}
