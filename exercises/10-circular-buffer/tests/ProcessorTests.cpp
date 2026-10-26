#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

TEST_CASE ("CircularDelayLine: read(0) é a última amostra, read(k) a de há k pushes", "[10]")
{
    CircularDelayLine line;
    line.prepare (16);

    for (int i = 1; i <= 10; ++i)
        line.push (static_cast<float> (i));

    REQUIRE (line.read (0) == 10.0f);
    REQUIRE (line.read (1) == 9.0f);
    REQUIRE (line.read (3) == 7.0f);
    REQUIRE (line.read (9) == 1.0f);
    REQUIRE (line.read (10) == 0.0f); // antes do primeiro push: silêncio
}

TEST_CASE ("CircularDelayLine: dá a volta ao buffer (wrap) sem perder o atraso máximo", "[10]")
{
    CircularDelayLine line;
    line.prepare (4); // atrasos 0..4

    REQUIRE (line.getMaximumDelayInSamples() == 4);

    for (int i = 1; i <= 23; ++i)
    {
        line.push (static_cast<float> (i));

        for (int d = 0; d <= 4; ++d)
        {
            INFO ("depois do push " << i << ", read(" << d << ")");
            REQUIRE (line.read (d) == static_cast<float> (juce::jmax (0, i - d)));
        }
    }
}

TEST_CASE ("CircularDelayLine: atrasos fora da gama são limitados e reset() limpa", "[10]")
{
    CircularDelayLine line;
    line.prepare (4);

    for (int i = 1; i <= 8; ++i)
        line.push (static_cast<float> (i));

    REQUIRE (line.read (100) == line.read (4));
    REQUIRE (line.read (-5) == line.read (0));

    line.reset();

    for (int d = 0; d <= 4; ++d)
        REQUIRE (line.read (d) == 0.0f);
}

TEST_CASE ("IntegerDelay: um impulso sai exatamente N amostras depois", "[10]")
{
    const auto delay = GENERATE (0, 1, 7, 480, 96000);
    INFO ("atraso: " << delay);

    IntegerDelay processor;
    processor.prepare ({ 48000.0, 512, 2 });
    processor.setDelaySamples (delay);
    REQUIRE (processor.getDelaySamples() == delay);

    auto buffer = makeImpulse (2, delay + 600, 3);
    processInBlocks (processor, buffer, awkwardBlockSizes());

    for (int ch = 0; ch < 2; ++ch)
    {
        REQUIRE (firstSampleAbove (buffer, ch, 0.5f) == 3 + delay);
        REQUIRE (buffer.getSample (ch, 3 + delay) == 1.0f);
        REQUIRE (peak (buffer, ch, 4 + delay) == 0.0f);
    }
}

TEST_CASE ("IntegerDelay: ruído longo — saída[n] = entrada[n - N] sempre, canais independentes", "[10]")
{
    IntegerDelay processor;
    processor.prepare ({ 1000.0, 512, 2 }); // 1 kHz: o buffer tem 2001 amostras, damos muitas voltas
    processor.setDelaySamples (1234);

    const auto input = makeNoise (2, 20000);
    auto buffer = copyOf (input);
    processInBlocks (processor, buffer, awkwardBlockSizes());

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int n = 0; n < 20000; ++n)
        {
            const auto expected = n >= 1234 ? input.getSample (ch, n - 1234) : 0.0f;
            INFO ("canal " << ch << ", amostra " << n);
            REQUIRE (buffer.getSample (ch, n) == expected);
        }
    }
}

TEST_CASE ("IntegerDelay: o atraso é limitado a 2 segundos", "[10]")
{
    IntegerDelay processor;
    processor.prepare ({ 48000.0, 512, 1 });
    processor.setDelaySamples (10'000'000);
    REQUIRE (processor.getDelaySamples() == 96000);
    processor.setDelaySamples (-3);
    REQUIRE (processor.getDelaySamples() == 0);
}

TEST_CASE ("IntegerDelay::process não aloca", "[10]")
{
    IntegerDelay processor;
    processor.prepare ({ 48000.0, 512, 2 });
    processor.setDelaySamples (300);

    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (processor, buffer, sizes); }) == 0);
}
