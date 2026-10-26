#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <numbers>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    /** RT60 estimado pela curva de Schroeder (integração para trás da energia da resposta a um
        impulso), medindo o declive entre −5 dB e −25 dB e extrapolando para −60 dB (T20 × 3). */
    double estimateRt60 (const std::vector<float>& h)
    {
        std::vector<double> edc (h.size());
        double total = 0.0;

        for (size_t i = h.size(); i-- > 0;)
        {
            total += static_cast<double> (h[i]) * h[i];
            edc[i] = total;
        }

        auto timeAt = [&] (double db)
        {
            for (size_t i = 0; i < edc.size(); ++i)
                if (10.0 * std::log10 (edc[i] / edc[0]) <= db)
                    return static_cast<double> (i) / fs;

            return static_cast<double> (edc.size()) / fs;
        };

        return 3.0 * (timeAt (-25.0) - timeAt (-5.0));
    }

    std::vector<float> reverbImpulseResponse (float rt60, float damping, int channel = 0, double seconds = 4.0)
    {
        SchroederReverb reverb;
        reverb.setDecaySeconds (rt60);
        reverb.setDamping (damping);
        reverb.setMix (1.0f);
        reverb.prepare ({ fs, 512, 2 });

        auto buffer = makeImpulse (2, static_cast<int> (seconds * fs), 0);
        processInBlocks (reverb, buffer, { 512 });

        const auto* d = buffer.getReadPointer (channel);
        return { d, d + buffer.getNumSamples() };
    }
}

TEST_CASE ("feedbackForRt60: g = 0.001^(D / (T60·fs))", "[28]")
{
    REQUIRE_THAT (SchroederReverb::feedbackForRt60 (1000, 48000.0, 2.0f),
                  WithinAbs (std::pow (0.001, 1000.0 / 96000.0), 1.0e-6));

    // Um comb com este g, ao fim de T60 segundos, caiu exatamente 60 dB
    const auto g = SchroederReverb::feedbackForRt60 (1500, 48000.0, 1.5f);
    const auto voltas = 1.5 * 48000.0 / 1500.0;
    REQUIRE_THAT (20.0 * std::log10 (std::pow (g, voltas)), WithinAbs (-60.0, 0.01));

    // Um comb mais comprido dá menos voltas por segundo, por isso cada volta tem de perder mais: g menor
    REQUIRE (SchroederReverb::feedbackForRt60 (2000, fs, 2.0f) < SchroederReverb::feedbackForRt60 (1000, fs, 2.0f));
}

TEST_CASE ("O decaimento medido corresponde ao RT60 pedido (±20%)", "[28]")
{
    for (const float rt60 : { 0.8f, 2.0f })
    {
        INFO ("RT60 pedido: " << rt60 << " s");
        const auto measured = estimateRt60 (reverbImpulseResponse (rt60, 0.0f, 0, rt60 * 2.0 + 0.5));
        REQUIRE_THAT (measured, WithinRel (static_cast<double> (rt60), 0.2));
    }
}

TEST_CASE ("Com damping as altas frequências decaem mais depressa", "[28]")
{
    // Compara a energia acima de 5 kHz no fim da cauda com e sem damping
    auto highEnergyLate = [] (float damping)
    {
        const auto h = reverbImpulseResponse (2.0f, damping, 0, 2.0);
        juce::AudioBuffer<float> tail (1, 24000);
        for (int n = 0; n < 24000; ++n)
            tail.setSample (0, n, h[(size_t) (48000 + n)]);

        return amplitudeAt (tail, 0, 8000.0, fs) / juce::jmax (1.0e-12, amplitudeAt (tail, 0, 500.0, fs));
    };

    REQUIRE (highEnergyLate (0.7f) < 0.5 * highEnergyLate (0.0f));
}

TEST_CASE ("L e R são diferentes (descorrelacionados)", "[28]")
{
    const auto left = reverbImpulseResponse (1.5f, 0.2f, 0, 1.0);
    const auto right = reverbImpulseResponse (1.5f, 0.2f, 1, 1.0);

    double lr = 0.0, ll = 0.0, rr = 0.0;
    for (size_t n = 0; n < left.size(); ++n)
    {
        lr += static_cast<double> (left[n]) * right[n];
        ll += static_cast<double> (left[n]) * left[n];
        rr += static_cast<double> (right[n]) * right[n];
    }

    REQUIRE (std::abs (lr / std::sqrt (ll * rr)) < 0.5);
}

TEST_CASE ("Mix 0 é o sinal original; com ruído forte fica finito e limitado; não aloca", "[28]")
{
    SchroederReverb reverb;
    reverb.setDecaySeconds (5.0f);
    reverb.setMix (0.0f);
    reverb.prepare ({ fs, 512, 2 });

    auto buffer = makeNoise (2, 4096);
    const auto input = copyOf (buffer);
    processInBlocks (reverb, buffer, awkwardBlockSizes());
    REQUIRE (maxAbsDifference (buffer, input) < 1.0e-6f);

    reverb.setMix (1.0f);
    auto loud = makeNoise (2, 48000 * 6, 1.0f);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (reverb, loud, sizes); }) == 0);
    REQUIRE (allFinite (loud));
    REQUIRE (peak (loud, 0) < 10.0f);
}
