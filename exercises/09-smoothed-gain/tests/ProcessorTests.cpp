#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <thread>

using namespace learn::test;

namespace
{
    void processWithContext (SmoothedGain& g, juce::AudioBuffer<float>& buffer, bool bypassed = false)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        context.isBypassed = bypassed;
        g.process (context);
    }
}

TEST_CASE ("Ganho definido antes do prepare é aplicado logo, sem rampa", "[09]")
{
    SmoothedGain g;
    g.setGainDecibels (-6.0f);
    g.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeConstant (2, 512, 1.0f);
    processWithContext (g, buffer);

    const auto expected = juce::Decibels::decibelsToGain (-6.0f);
    REQUIRE_THAT (buffer.getSample (0, 0), WithinAbs (expected, 1.0e-6));
    REQUIRE_THAT (buffer.getSample (1, 511), WithinAbs (expected, 1.0e-6));
}

TEST_CASE ("Mudar o ganho faz uma rampa de 20 ms, sem saltos", "[09]")
{
    SmoothedGain g;
    g.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeConstant (2, 4800, 1.0f);
    g.setGainDecibels (-60.0f); // -60 dB = silêncio
    processInBlocks (g, buffer, awkwardBlockSizes());

    // 960 amostras de rampa de 1 até 0
    REQUIRE (maxSampleJump (buffer, 0) < 1.0f / 960.0f + 1.0e-5f);
    REQUIRE (buffer.getSample (0, 958) > 0.0f);
    REQUIRE (buffer.getSample (0, 959) == 0.0f);
    REQUIRE (peak (buffer, 1, 960) == 0.0f);
}

TEST_CASE ("reset() salta para o ganho atual (sem rampa)", "[09]")
{
    SmoothedGain g;
    g.prepare ({ 48000.0, 512, 1 });
    g.setGainDecibels (-20.0f);
    g.reset();

    auto buffer = makeConstant (1, 16, 1.0f);
    processWithContext (g, buffer);

    REQUIRE_THAT (buffer.getSample (0, 0), WithinAbs (0.1, 1.0e-6));
}

TEST_CASE ("Com context.isBypassed o áudio passa inalterado", "[09]")
{
    SmoothedGain g;
    g.setGainDecibels (-12.0f);
    g.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeNoise (2, 512);
    const auto input = copyOf (buffer);
    processWithContext (g, buffer, true);

    REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
}

TEST_CASE ("Encaixa num juce::dsp::ProcessorChain", "[09]")
{
    juce::dsp::ProcessorChain<SmoothedGain, juce::dsp::Gain<float>> chain;
    chain.get<0>().setGainDecibels (-6.0f);
    chain.get<1>().setGainDecibels (-6.0f);
    chain.prepare ({ 48000.0, 256, 2 });

    auto buffer = makeConstant (2, 256, 1.0f);
    juce::dsp::AudioBlock<float> block (buffer);
    chain.process (juce::dsp::ProcessContextReplacing<float> (block));

    REQUIRE_THAT (buffer.getSample (0, 255), WithinAbs (juce::Decibels::decibelsToGain (-12.0f), 1.0e-5));
}

TEST_CASE ("A GUI pode mudar o ganho enquanto o áudio corre (outra thread)", "[09]")
{
    SmoothedGain g;
    g.prepare ({ 48000.0, 256, 2 });

    std::atomic<bool> stop { false };

    std::thread gui ([&]
    {
        juce::Random random (3);

        while (! stop.load())
            g.setGainDecibels (-30.0f * random.nextFloat()); // entre -30 e 0 dB
    });

    auto buffer = makeConstant (2, 256, 1.0f);
    bool ok = true;

    for (int i = 0; i < 3000 && ok; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
            juce::FloatVectorOperations::fill (buffer.getWritePointer (ch), 1.0f, 256);

        processWithContext (g, buffer);
        ok = allFinite (buffer) && peak (buffer, 0) <= 1.0f + 1.0e-6f && buffer.getSample (0, 0) >= 0.03f;
    }

    stop.store (true);
    gui.join();
    REQUIRE (ok);

    // Depois de a GUI parar, o último valor é respeitado
    g.setGainDecibels (-12.0f);
    auto settle = makeConstant (2, 2048, 1.0f);
    processWithContext (g, settle);
    REQUIRE_THAT (settle.getSample (0, 2047), WithinAbs (juce::Decibels::decibelsToGain (-12.0f), 1.0e-5));
    REQUIRE (g.getGainDecibels() == -12.0f);
}

TEST_CASE ("process() não aloca", "[09]")
{
    SmoothedGain g;
    g.prepare ({ 48000.0, 512, 2 });
    auto buffer = makeNoise (2, 2048);
    const auto sizes = awkwardBlockSizes();

    REQUIRE (countAllocations ([&]
    {
        g.setGainDecibels (-3.0f);
        processInBlocks (g, buffer, sizes);
    }) == 0);
}
