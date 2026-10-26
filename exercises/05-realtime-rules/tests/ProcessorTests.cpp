#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <thread>
#include <vector>

using namespace learn::test;

TEST_CASE ("O áudio passa inalterado", "[05]")
{
    PeakMeterProcessor meter;
    meter.prepareToPlay (48000.0, 512);

    auto buffer = makeNoise (2, 512);
    const auto input = copyOf (buffer);
    meter.processBlock (buffer);

    REQUIRE (maxAbsDifference (buffer, input) == 0.0f);
}

TEST_CASE ("getPeak devolve o pico do último bloco de cada canal", "[05]")
{
    PeakMeterProcessor meter;
    meter.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 256);
    buffer.clear();
    buffer.setSample (0, 10, -0.7f);
    buffer.setSample (1, 200, 0.25f);
    meter.processBlock (buffer);

    REQUIRE_THAT (meter.getPeak (0), WithinAbs (0.7f, 1.0e-7));
    REQUIRE_THAT (meter.getPeak (1), WithinAbs (0.25f, 1.0e-7));
    REQUIRE (meter.getPeak (5) == 0.0f);   // canal que não existe neste bloco
    REQUIRE (meter.getPeak (99) == 0.0f);  // fora do limite: não pode rebentar

    buffer.clear();
    buffer.setSample (0, 0, 0.1f);
    meter.processBlock (buffer);

    REQUIRE_THAT (meter.getPeak (0), WithinAbs (0.1f, 1.0e-7)); // é o pico do ÚLTIMO bloco
    REQUIRE (meter.getPeak (1) == 0.0f);
}

TEST_CASE ("readHistory devolve um pico por bloco, por ordem, e esvazia", "[05]")
{
    PeakMeterProcessor meter;
    meter.prepareToPlay (48000.0, 512);

    for (int block = 1; block <= 5; ++block)
    {
        auto buffer = makeConstant (2, 64, 0.0f);
        buffer.setSample (block % 2, 3, 0.1f * block); // o pico de cada bloco está num canal diferente
        meter.processBlock (buffer);
    }

    float values[16] {};
    REQUIRE (meter.readHistory (values, 16) == 5);

    for (int i = 0; i < 5; ++i)
        REQUIRE_THAT (values[i], WithinAbs (0.1f * (i + 1), 1.0e-6));

    REQUIRE (meter.readHistory (values, 16) == 0);
}

TEST_CASE ("readHistory respeita maxValues", "[05]")
{
    PeakMeterProcessor meter;
    meter.prepareToPlay (48000.0, 512);

    auto buffer = makeConstant (1, 32, 0.5f);

    for (int i = 0; i < 10; ++i)
        meter.processBlock (buffer);

    float values[4] {};
    REQUIRE (meter.readHistory (values, 4) == 4);
    REQUIRE (meter.readHistory (values, 4) == 4);
    REQUIRE (meter.readHistory (values, 4) == 2);
}

TEST_CASE ("processBlock não aloca memória (nem quando a GUI não lê o histórico)", "[05]")
{
    PeakMeterProcessor meter;
    meter.prepareToPlay (48000.0, 512);

    auto buffer = makeNoise (2, 512);
    const auto sizes = awkwardBlockSizes();

    // 3000 blocos sem ninguém ler o histórico: mais do que historyCapacity
    const auto allocations = countAllocations ([&]
    {
        for (int i = 0; i < 300; ++i)
            processInBlocks (meter, buffer, sizes);
    });

    REQUIRE (allocations == 0);

    std::vector<float> values (4096);
    REQUIRE (meter.readHistory (values.data(), 4096) <= PeakMeterProcessor::historyCapacity);
}

TEST_CASE ("Áudio e GUI em threads diferentes ao mesmo tempo", "[05]")
{
    PeakMeterProcessor meter;
    meter.prepareToPlay (48000.0, 256);

    std::atomic<bool> audioFinished { false };
    int valuesRead = 0;
    bool allValuesValid = true;

    std::thread gui ([&]
    {
        std::vector<float> values (256);

        while (! audioFinished.load())
        {
            const auto p = meter.getPeak (0);
            allValuesValid = allValuesValid && p >= 0.0f && p <= 1.0f;

            const auto num = meter.readHistory (values.data(), 256);

            for (int i = 0; i < num; ++i)
                allValuesValid = allValuesValid && values[(size_t) i] >= 0.0f && values[(size_t) i] <= 1.0f;

            valuesRead += num;
        }

        valuesRead += meter.readHistory (values.data(), 256);
    });

    auto buffer = makeNoise (2, 256, 0.9f);

    for (int i = 0; i < 20000; ++i)
        meter.processBlock (buffer);

    audioFinished.store (true);
    gui.join();

    REQUIRE (allValuesValid);
    REQUIRE (valuesRead > 0);
    REQUIRE (valuesRead <= 20000);
}
