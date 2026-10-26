#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    /** Hard clip de um seno de 5 kHz com o fator de oversampling dado. Devolve o sinal processado. */
    juce::AudioBuffer<float> clipFiveK (int factorLog2, Shapers::Shape shape = Shapers::Shape::hardClip)
    {
        OversampledShaper shaper;
        shaper.setOversamplingFactorLog2 (factorLog2);
        shaper.setShape (shape);
        shaper.setDriveDecibels (18.0f);
        shaper.prepare ({ fs, 512, 1 });

        auto buffer = makeSine (1, 48000, 5000.0, fs, 1.0f);
        processInBlocks (shaper, buffer, { 512 });
        return buffer;
    }

    /** Amplitude (dB) a uma frequência, na 2.ª metade do buffer. */
    double levelAt (const juce::AudioBuffer<float>& b, double frequency)
    {
        return toDb (amplitudeAt (b, 0, frequency, fs, 24000, 24000));
    }
}

TEST_CASE ("Sem oversampling, o hard clip de 5 kHz tem aliasing em 3 kHz e 13 kHz", "[26]")
{
    // Harmónicos de 5 kHz: 15 k (ok), 25 k -> 23 k, 35 k -> 13 k, 45 k -> 3 k, 55 k -> 7 k ...
    const auto plain = clipFiveK (0);
    REQUIRE (levelAt (plain, 3000.0) > -40.0);
    REQUIRE (levelAt (plain, 13000.0) > -40.0);
}

TEST_CASE ("Com 4× de oversampling, esse aliasing cai pelo menos 20 dB", "[26]")
{
    const auto plain = clipFiveK (0);
    const auto oversampled = clipFiveK (2);

    for (const double alias : { 3000.0, 7000.0, 13000.0 })
    {
        INFO ("frequência de alias " << alias << " Hz");
        REQUIRE (levelAt (oversampled, alias) < levelAt (plain, alias) - 20.0);
    }

    // ...mas os harmónicos verdadeiros abaixo de Nyquist continuam lá (o 3.º, a 15 kHz)
    REQUIRE_THAT (levelAt (oversampled, 15000.0), WithinAbs (levelAt (plain, 15000.0), 3.0));
    REQUIRE_THAT (levelAt (oversampled, 5000.0), WithinAbs (levelAt (plain, 5000.0), 1.0));
}

TEST_CASE ("Mais oversampling, menos aliasing (com hard clip)", "[26]")
{
    REQUIRE (levelAt (clipFiveK (3), 3000.0) < levelAt (clipFiveK (1), 3000.0));
}

TEST_CASE ("O oversampling tem latência, e ela é reportada", "[26]")
{
    OversampledShaper plain;
    plain.setOversamplingFactorLog2 (0);
    plain.prepare ({ fs, 512, 2 });
    REQUIRE (plain.getLatencyInSamples() == 0.0f);

    OversampledShaper shaper;
    shaper.setOversamplingFactorLog2 (2);
    shaper.setDriveDecibels (-40.0f); // praticamente linear: o impulso passa sem distorcer
    shaper.setShape (Shapers::Shape::tanh);
    shaper.prepare ({ fs, 512, 2 });
    REQUIRE (shaper.getOversamplingFactor() == 4);
    REQUIRE (shaper.getLatencyInSamples() > 0.0f);

    // Um impulso sai atrasado (o máximo da resposta fica perto da latência reportada)
    auto buffer = makeImpulse (2, 512, 0, 100.0f);
    processWhole (shaper, buffer);

    int peakIndex = 0;
    for (int n = 1; n < 512; ++n)
        if (std::abs (buffer.getSample (0, n)) > std::abs (buffer.getSample (0, peakIndex)))
            peakIndex = n;

    REQUIRE (std::abs (peakIndex - shaper.getLatencyInSamples()) <= 3.0f);
}

TEST_CASE ("process() não aloca (os buffers são reservados no prepare)", "[26]")
{
    OversampledShaper shaper;
    shaper.setOversamplingFactorLog2 (3);
    shaper.prepare ({ fs, 512, 2 });

    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (shaper, buffer, sizes); }) == 0);
    REQUIRE (allFinite (buffer));
}
