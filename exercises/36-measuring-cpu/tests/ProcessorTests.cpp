#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    template <typename Filter>
    double bestTimeSeconds (Filter& filter, const juce::AudioBuffer<float>& input, const DriveFilterSettings& settings)
    {
        double best = 1.0e9;

        for (int run = 0; run < 5; ++run)
        {
            auto buffer = copyOf (input);
            filter.reset();
            const auto start = juce::Time::getHighResolutionTicks();

            juce::dsp::AudioBlock<float> block (buffer);
            for (size_t pos = 0; pos < block.getNumSamples(); pos += 512)
            {
                auto sub = block.getSubBlock (pos, std::min<size_t> (512, block.getNumSamples() - pos));
                filter.process (juce::dsp::ProcessContextReplacing<float> (sub), settings);
            }

            best = std::min (best, juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - start));
        }

        return best;
    }
}

TEST_CASE ("CpuLoadMeter: carga = tempo / orçamento, suavizada, com pico e overruns", "[36]")
{
    CpuLoadMeter meter;
    meter.prepare (fs);

    meter.addMeasurement (0.005, 480); // 480 amostras a 48 kHz = 10 ms de orçamento
    REQUIRE_THAT (meter.getLoad(), WithinAbs (0.5, 1.0e-5));
    REQUIRE_THAT (meter.getPeak(), WithinAbs (0.5, 1.0e-5));

    meter.addMeasurement (0.010, 480); // 100%
    REQUIRE_THAT (meter.getLoad(), WithinAbs (0.55, 1.0e-5)); // 0.5 + 0.1 · (1.0 − 0.5)
    REQUIRE_THAT (meter.getPeak(), WithinAbs (1.0, 1.0e-5));
    REQUIRE (meter.getOverruns() == 0);

    meter.addMeasurement (0.015, 480); // 150%: o áudio falhou
    REQUIRE (meter.getOverruns() == 1);
    REQUIRE_THAT (meter.getPeak(), WithinAbs (1.5, 1.0e-5));

    meter.addMeasurement (0.001, 0); // bloco vazio: ignora
    REQUIRE (meter.getOverruns() == 1);

    meter.reset();
    REQUIRE (meter.getLoad() == 0.0f);
    REQUIRE (meter.getPeak() == 0.0f);
    REQUIRE (meter.getOverruns() == 0);
}

TEST_CASE ("ScopedMeasurement mede tempo real (um sleep de ~5 ms num orçamento de 10 ms)", "[36]")
{
    CpuLoadMeter meter;
    meter.prepare (fs);

    {
        const CpuLoadMeter::ScopedMeasurement measure (meter, 480);
        juce::Thread::sleep (5);
    }

    INFO ("carga medida: " << meter.getLoad());
    REQUIRE (meter.getLoad() > 0.3f);  // o sleep do Windows não é exato (resolução do temporizador)
    REQUIRE (meter.getLoad() < 3.0f); // o Windows às vezes dorme mais do que se pede
}

TEST_CASE ("FastDriveFilter dá o mesmo resultado que a referência", "[36]")
{
    const DriveFilterSettings settings { 18.0f, 2500.0f, -3.0f };
    const auto input = makeNoise (2, 48000, 0.8f);

    NaiveDriveFilter naive;
    FastDriveFilter fast;
    naive.prepare ({ fs, 512, 2 });
    fast.prepare ({ fs, 512, 2 });

    auto expected = copyOf (input);
    auto actual = copyOf (input);

    // blocos de tamanhos diferentes: o estado tem de passar de um bloco para o seguinte
    struct Wrapper
    {
        void process (const juce::dsp::ProcessContextReplacing<float>& c) noexcept { filter.process (c, s); }
        FastDriveFilter& filter;
        const DriveFilterSettings& s;
    } wrapper { fast, settings };

    juce::dsp::AudioBlock<float> block (expected);
    naive.process (juce::dsp::ProcessContextReplacing<float> (block), settings);
    processInBlocks (wrapper, actual, awkwardBlockSizes());

    REQUIRE (maxAbsDifference (expected, actual) < 1.0e-6f);
    REQUIRE (rms (actual, 0) > 0.05); // e não é silêncio
}

TEST_CASE ("FastDriveFilter é pelo menos 3× mais rápido (em Release)", "[36]")
{
    const DriveFilterSettings settings;
    const auto input = makeNoise (2, 10 * 48000, 0.5f);

    NaiveDriveFilter naive;
    FastDriveFilter fast;
    naive.prepare ({ fs, 512, 2 });
    fast.prepare ({ fs, 512, 2 });

    const auto naiveTime = bestTimeSeconds (naive, input, settings);
    const auto fastTime = bestTimeSeconds (fast, input, settings);
    const auto speedup = naiveTime / std::max (fastTime, 1.0e-9);

    // 10 s de áudio estéreo: quanto do orçamento de tempo-real usa cada versão?
    INFO ("referência: " << naiveTime * 1000.0 << " ms (" << naiveTime / 10.0 * 100.0 << "% de CPU)");
    INFO ("a tua:      " << fastTime * 1000.0 << " ms (" << fastTime / 10.0 * 100.0 << "% de CPU)");
    INFO ("aceleração: " << speedup << "x");

#if JUCE_DEBUG
    WARN ("Compilado em Debug: a medição de velocidade não conta. Usa ./learn check 36 (Release).");
    SUCCEED();
#else
    REQUIRE (speedup >= 3.0);
#endif
}

TEST_CASE ("Nada aloca no processamento nem na medição", "[36]")
{
    FastDriveFilter fast;
    fast.prepare ({ fs, 512, 2 });
    CpuLoadMeter meter;
    meter.prepare (fs);

    auto buffer = makeNoise (2, 512, 0.5f);
    juce::dsp::AudioBlock<float> block (buffer);

    REQUIRE (countAllocations ([&] {
        const CpuLoadMeter::ScopedMeasurement measure (meter, 512);
        fast.process (juce::dsp::ProcessContextReplacing<float> (block), {});
    }) == 0);
    REQUIRE (meter.getLoad() > 0.0f);
}
