#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

TEST_CASE ("processBlock multiplica cada amostra por 0.5", "[01]")
{
    FirstBufferProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto buffer = makeNoise (2, 512, 0.9f);
    const auto input = copyOf (buffer);

    processor.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int n = 0; n < 512; ++n)
        {
            INFO ("canal " << ch << ", amostra " << n);
            REQUIRE_THAT (buffer.getSample (ch, n), WithinAbs (0.5f * input.getSample (ch, n), 1.0e-7));
        }
    }
}

TEST_CASE ("processBlock trata todos os canais, seja quantos forem", "[01]")
{
    const auto numChannels = GENERATE (1, 2, 6);
    INFO ("número de canais: " << numChannels);

    FirstBufferProcessor processor;
    processor.prepareToPlay (44100.0, 256);

    auto buffer = makeConstant (numChannels, 256, 0.8f);
    processor.processBlock (buffer);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        INFO ("canal " << ch);
        REQUIRE_THAT (buffer.getSample (ch, 0), WithinAbs (0.4f, 1.0e-7));
        REQUIRE_THAT (buffer.getSample (ch, 255), WithinAbs (0.4f, 1.0e-7));
    }
}

TEST_CASE ("processBlock usa getNumSamples(): funciona com blocos de qualquer tamanho", "[01]")
{
    FirstBufferProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    // 4096 amostras processadas em blocos de 1, 7, 64, 13, 512, ... (como um host faria)
    auto buffer = makeConstant (2, 4096, 1.0f);
    processInBlocks (processor, buffer, awkwardBlockSizes());

    // Cada amostra tem de ter sido multiplicada exatamente UMA vez.
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int n = 0; n < 4096; ++n)
        {
            INFO ("canal " << ch << ", amostra " << n);
            REQUIRE_THAT (buffer.getSample (ch, n), WithinAbs (0.5f, 1.0e-7));
        }
    }
}

TEST_CASE ("processBlock não toca em amostras fora do bloco recebido", "[01]")
{
    FirstBufferProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto buffer = makeConstant (2, 100, 1.0f);

    // Processa só as amostras 10..59 (um "bloco" que é uma vista para o meio do buffer)
    processRange (processor, buffer, 10, 50);

    for (int ch = 0; ch < 2; ++ch)
    {
        REQUIRE (buffer.getSample (ch, 9) == 1.0f);
        REQUIRE_THAT (buffer.getSample (ch, 10), WithinAbs (0.5f, 1.0e-7));
        REQUIRE_THAT (buffer.getSample (ch, 59), WithinAbs (0.5f, 1.0e-7));
        REQUIRE (buffer.getSample (ch, 60) == 1.0f);
    }
}

TEST_CASE ("findPeak devolve o maior |x| de todos os canais", "[01]")
{
    juce::AudioBuffer<float> buffer (2, 8);
    buffer.clear();
    buffer.setSample (0, 3, 0.25f);
    buffer.setSample (0, 5, -0.5f);
    buffer.setSample (1, 7, -0.8f); // o pico é negativo e está no último canal, na última amostra
    buffer.setSample (1, 0, 0.7f);

    REQUIRE_THAT (FirstBufferProcessor::findPeak (buffer), WithinAbs (0.8f, 1.0e-7));
}

TEST_CASE ("findPeak segue o processamento: o pico cai para metade", "[01]")
{
    FirstBufferProcessor processor;
    processor.prepareToPlay (48000.0, 1024);

    auto buffer = makeSine (2, 1024, 1000.0, 48000.0, 0.9f);
    const auto peakBefore = FirstBufferProcessor::findPeak (buffer);
    REQUIRE (peakBefore > 0.85f);

    processor.processBlock (buffer);

    REQUIRE_THAT (FirstBufferProcessor::findPeak (buffer), WithinAbs (0.5f * peakBefore, 1.0e-6));
}
