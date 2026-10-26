#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    juce::AudioBuffer<float> shift (float semitones, double frequency)
    {
        PitchShifter shifter;
        shifter.prepare (fs);
        shifter.setSemitones (semitones);

        auto buffer = makeSine (1, 48000, frequency, fs, 0.5f);
        auto* data = buffer.getWritePointer (0);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
            data[n] = shifter.processSample (data[n]);

        return buffer;
    }
}

TEST_CASE ("Crossfade Hann: as duas leituras somam sempre 1", "[31]")
{
    REQUIRE_THAT (PitchShifter::crossfadeGain (0.0f), WithinAbs (0.0, 1.0e-6));
    REQUIRE_THAT (PitchShifter::crossfadeGain (0.5f), WithinAbs (1.0, 1.0e-6));

    for (int i = 0; i < 100; ++i)
    {
        const auto p = static_cast<float> (i) / 200.0f; // [0, 0.5)
        INFO ("fase " << p);
        REQUIRE_THAT (PitchShifter::crossfadeGain (p) + PitchShifter::crossfadeGain (p + 0.5f), WithinAbs (1.0, 1.0e-5));
    }
}

TEST_CASE ("Semitons -> razão: +12 = 2, −12 = 0.5, +7 ≈ 1.498", "[31]")
{
    PitchShifter shifter;
    shifter.prepare (fs);

    shifter.setSemitones (12.0f);
    REQUIRE_THAT (shifter.getRatio(), WithinRel (2.0, 1.0e-5));
    shifter.setSemitones (-12.0f);
    REQUIRE_THAT (shifter.getRatio(), WithinRel (0.5, 1.0e-5));
    shifter.setSemitones (7.0f);
    REQUIRE_THAT (shifter.getRatio(), WithinRel (1.4983, 1.0e-3));
}

TEST_CASE ("+12 semitons: 440 Hz sai como 880 Hz", "[31]")
{
    const auto out = shift (12.0f, 440.0);

    const auto at880 = amplitudeAt (out, 0, 880.0, fs, 4800, 38400);
    const auto at440 = amplitudeAt (out, 0, 440.0, fs, 4800, 38400);
    INFO ("amplitude a 880 Hz: " << at880 << ", a 440 Hz: " << at440);

    REQUIRE (at880 > 0.3);          // a entrada tinha 0.5: perde-se um pouco no crossfade
    REQUIRE (at880 > 10.0 * at440); // e a frequência original quase desapareceu
}

TEST_CASE ("−12 semitons: 440 Hz sai como 220 Hz; 0 semitons não muda nada", "[31]")
{
    const auto down = shift (-12.0f, 440.0);
    REQUIRE (amplitudeAt (down, 0, 220.0, fs, 4800, 38400) > 10.0 * amplitudeAt (down, 0, 440.0, fs, 4800, 38400));

    const auto same = shift (0.0f, 440.0);
    REQUIRE_THAT (rms (same, 0, 4800), WithinRel (0.5 / std::sqrt (2.0), 0.02));
    REQUIRE_THAT (amplitudeAt (same, 0, 440.0, fs, 4800, 38400), WithinRel (0.5, 0.05));
}

TEST_CASE ("Shimmer: a cauda ganha a oitava de cima, fica estável e não aloca", "[31]")
{
    auto render = [] (float shimmer)
    {
        ShimmerReverb reverb;
        reverb.setDecaySeconds (4.0f);
        reverb.setShimmer (shimmer);
        reverb.setSemitones (12.0f);
        reverb.setMix (1.0f);
        reverb.prepare ({ fs, 512, 2 });

        // 0.3 s de 440 Hz seguidos de silêncio
        auto buffer = makeSine (2, 10 * 48000, 440.0, fs, 0.5f);
        for (int ch = 0; ch < 2; ++ch)
            buffer.clear (ch, 14400, buffer.getNumSamples() - 14400);

        const auto sizes = awkwardBlockSizes();
        const auto allocations = countAllocations ([&] { processInBlocks (reverb, buffer, sizes); });
        REQUIRE (allocations == 0);
        return buffer;
    };

    const auto plain = render (0.0f);
    const auto shimmer = render (0.8f);
    REQUIRE (allFinite (shimmer));

    // Na cauda (1–2 s): sem shimmer quase não há 880 Hz; com shimmer há bastante.
    const auto plain880 = amplitudeAt (plain, 0, 880.0, fs, 48000, 48000);
    const auto shimmer880 = amplitudeAt (shimmer, 0, 880.0, fs, 48000, 48000);
    INFO ("880 Hz na cauda: sem shimmer " << plain880 << ", com shimmer " << shimmer880);
    REQUIRE (shimmer880 > 5.0 * plain880);

    // Estável: limitado e, no fim, a cauda está a morrer.
    REQUIRE (peak (shimmer, 0) < 2.0f);
    REQUIRE (rms (shimmer, 0, 9 * 48000, 48000) < 0.25 * rms (shimmer, 0, 48000, 48000));
}
