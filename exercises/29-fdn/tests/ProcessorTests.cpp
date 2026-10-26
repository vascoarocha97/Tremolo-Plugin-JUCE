#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;
using Frame = FeedbackDelayNetwork::Frame;

namespace
{
    constexpr double fs = 48000.0;

    double norm (const Frame& v)
    {
        double s = 0.0;
        for (const auto x : v) s += static_cast<double> (x) * x;
        return std::sqrt (s);
    }

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
}

TEST_CASE ("Householder: v − (2/N)·Σv, ortogonal e a sua própria inversa", "[29]")
{
    Frame unit {};
    unit[0] = 1.0f;
    FeedbackDelayNetwork::householder (unit);
    REQUIRE_THAT (unit[0], WithinAbs (1.0 - 2.0 / 8.0, 1.0e-6));
    REQUIRE_THAT (unit[5], WithinAbs (-2.0 / 8.0, 1.0e-6));

    juce::Random random (3);

    for (int trial = 0; trial < 100; ++trial)
    {
        Frame v;
        for (auto& x : v) x = 2.0f * random.nextFloat() - 1.0f;
        const auto original = v;

        FeedbackDelayNetwork::householder (v);
        REQUIRE_THAT (norm (v), WithinRel (norm (original), 1.0e-5));   // preserva a energia

        FeedbackDelayNetwork::householder (v);
        for (size_t i = 0; i < v.size(); ++i)
            REQUIRE_THAT (v[i], WithinAbs (original[i], 1.0e-5));       // H·H = I
    }
}

TEST_CASE ("Com ganhos = 1 a rede não perde nem ganha energia", "[29]")
{
    FeedbackDelayNetwork fdn;
    fdn.prepare (fs);
    fdn.setDecaySeconds (1.0e9f); // g ≈ 1
    REQUIRE (fdn.getLineGain (0) > 0.99999f);

    // Injeta energia durante algum tempo, depois deixa a rede sozinha
    juce::Random random (5);
    Frame input {};

    for (int n = 0; n < 2000; ++n)
    {
        for (auto& x : input) x = 2.0f * random.nextFloat() - 1.0f;
        fdn.processFrame (input);
    }

    const auto energyBefore = fdn.getStoredEnergy();
    input.fill (0.0f);

    for (int n = 0; n < 48000; ++n)
        fdn.processFrame (input);

    REQUIRE (energyBefore > 1.0);
    REQUIRE_THAT (fdn.getStoredEnergy(), WithinRel (energyBefore, 1.0e-3));
}

TEST_CASE ("Ganhos por linha: g = 0.001^(D/(T60·fs)); linhas mais compridas têm g menor", "[29]")
{
    FeedbackDelayNetwork fdn;
    fdn.prepare (fs);
    fdn.setDecaySeconds (2.0f);

    const auto d0 = static_cast<int> (FeedbackDelayNetwork::delayTimesMs[0] * 0.001 * fs);
    REQUIRE_THAT (fdn.getLineGain (0), WithinAbs (std::pow (0.001, d0 / (2.0 * fs)), 1.0e-5));
    REQUIRE (fdn.getLineGain (7) < fdn.getLineGain (0));
}

TEST_CASE ("O decaimento medido corresponde ao RT60 pedido (±25%)", "[29]")
{
    for (const float rt60 : { 1.0f, 3.0f })
    {
        FeedbackDelayNetwork fdn;
        fdn.prepare (fs);
        fdn.setDecaySeconds (rt60);

        std::vector<float> h;
        const auto length = static_cast<int> ((rt60 * 1.5 + 0.5) * fs);

        for (int n = 0; n < length; ++n)
            h.push_back (fdn.processSample (n == 0 ? 1.0f : 0.0f).first);

        INFO ("RT60 pedido: " << rt60);
        REQUIRE_THAT (estimateRt60 (h), WithinRel (static_cast<double> (rt60), 0.25));
    }
}

TEST_CASE ("A densidade de ecos cresce com o tempo (mas devagar: é isso que o exercício 30 resolve)", "[29]")
{
    FeedbackDelayNetwork fdn;
    fdn.prepare (fs);
    fdn.setDecaySeconds (3.0f);

    std::vector<float> h;
    for (int n = 0; n < 48000; ++n)
        h.push_back (fdn.processSample (n == 0 ? 1.0f : 0.0f).first);

    auto fractionNonZero = [&] (double fromSeconds, double toSeconds)
    {
        int count = 0, total = 0;
        for (auto n = static_cast<size_t> (fromSeconds * fs); n < static_cast<size_t> (toSeconds * fs); ++n, ++total)
            if (std::abs (h[n]) > 1.0e-7f)
                ++count;
        return static_cast<double> (count) / total;
    };

    const auto early = fractionNonZero (0.0, 0.2);
    const auto late = fractionNonZero (0.7, 0.9);
    INFO ("fração de amostras com som: início " << early << ", 0.7–0.9 s " << late);

    REQUIRE (early < 0.01);        // ao início: meia dúzia de ecos isolados
    REQUIRE (late > 0.1);          // depois: muito mais ecos...
    REQUIRE (late > 20.0 * early); // ...a densidade cresce com o número de voltas
}

TEST_CASE ("FdnReverb: L ≠ R, mix 0 = original, finito, não aloca", "[29]")
{
    FdnReverb reverb;
    reverb.setDecaySeconds (2.0f);
    reverb.setMix (1.0f);
    reverb.prepare ({ fs, 512, 2 });

    auto impulse = makeImpulse (2, 48000, 0);
    processInBlocks (reverb, impulse, awkwardBlockSizes());

    double lr = 0.0, ll = 0.0, rr = 0.0;
    for (int n = 0; n < 48000; ++n)
    {
        lr += static_cast<double> (impulse.getSample (0, n)) * impulse.getSample (1, n);
        ll += static_cast<double> (impulse.getSample (0, n)) * impulse.getSample (0, n);
        rr += static_cast<double> (impulse.getSample (1, n)) * impulse.getSample (1, n);
    }
    REQUIRE (std::abs (lr / std::sqrt (ll * rr)) < 0.5);

    reverb.setMix (0.0f);
    auto buffer = makeNoise (2, 4096);
    const auto input = copyOf (buffer);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (reverb, buffer, sizes); }) == 0);
    REQUIRE (maxAbsDifference (buffer, input) < 1.0e-6f);
}
