#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;
    constexpr int blockSize = 480;

    /** Processa `buffer` em blocos de 480; liga o glitch no bloco `onBlock` e desliga no `offBlock`. */
    void run (Glitch& g, juce::AudioBuffer<float>& buffer, int onBlock, int offBlock)
    {
        for (int b = 0; b * blockSize < buffer.getNumSamples(); ++b)
        {
            if (b == onBlock)
                g.setActive (true);
            if (b == offBlock)
                g.setActive (false);

            processRange (g, buffer, b * blockSize, juce::jmin (blockSize, buffer.getNumSamples() - b * blockSize));
        }
    }
}

TEST_CASE ("Desligado: a saída é a entrada", "[39]")
{
    Glitch g;
    g.prepare ({ fs, 512, 2 });

    const auto input = makeNoise (2, 48000, 0.5f);
    auto buffer = copyOf (input);
    processInBlocks (g, buffer, awkwardBlockSizes());

    REQUIRE (maxAbsDifference (input, buffer) == 0.0f);
}

TEST_CASE ("Ligado: repete exatamente os últimos `slice` ms", "[39]")
{
    Glitch g;
    g.setSliceMs (100.0f); // 4800 amostras
    g.setFadeMs (4.0f);    // 192 amostras
    g.prepare ({ fs, 512, 2 });

    const auto input = makeNoise (2, 48000, 0.5f);
    auto buffer = copyOf (input);
    run (g, buffer, 50, 1000); // liga na amostra 24000

    REQUIRE (g.isRepeating());
    REQUIRE (g.getSliceLengthSamples() == 4800);
    REQUIRE (g.getFadeLengthSamples() == 192);

    // Depois do crossfade de entrada, até à zona de junção: é a fatia [24000 − 4800, 24000)
    float diff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int k = 192; k < 4800 - 192; ++k)
            diff = juce::jmax (diff, std::abs (buffer.getSample (ch, 24000 + k) - input.getSample (ch, 24000 - 4800 + k)));

    REQUIRE (diff == 0.0f);
}

TEST_CASE ("Em repetição, a saída é periódica com período = slice", "[39]")
{
    Glitch g;
    g.setSliceMs (75.0f); // 3600 amostras
    g.prepare ({ fs, 512, 2 });

    auto buffer = makeNoise (2, 48000, 0.5f);
    run (g, buffer, 20, 1000); // liga em 9600

    float diff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int n = 9600 + 400; n + 3600 < 48000; ++n)
            diff = juce::jmax (diff, std::abs (buffer.getSample (ch, n + 3600) - buffer.getSample (ch, n)));

    REQUIRE (diff < 1.0e-6f);
}

TEST_CASE ("Sem cliques: nas junções, ao ligar e ao desligar", "[39]")
{
    // Seno de 220 Hz: a fatia de 77 ms não tem um número inteiro de períodos, por isso, sem
    // crossfade, cada junção seria um degrau.
    Glitch g;
    g.setSliceMs (77.0f);
    g.setFadeMs (5.0f);
    g.prepare ({ fs, 512, 1 });

    auto buffer = makeSine (1, 96000, 220.0, fs, 0.5f);
    run (g, buffer, 40, 150); // liga a 19200, desliga a 72000

    // O maior passo entre amostras de um seno de 220 Hz e amplitude 0.5 é ~0.0144
    INFO ("maior salto: " << maxSampleJump (buffer, 0));
    REQUIRE (maxSampleJump (buffer, 0) < 0.03f);
}

TEST_CASE ("Desligar: volta ao sinal seco depois do crossfade", "[39]")
{
    Glitch g;
    g.prepare ({ fs, 512, 2 });

    const auto input = makeNoise (2, 48000, 0.5f);
    auto buffer = copyOf (input);
    run (g, buffer, 10, 60); // liga em 4800, desliga em 28800

    REQUIRE_FALSE (g.isRepeating());
    float diff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int n = 28800 + 1000; n < 48000; ++n)
            diff = juce::jmax (diff, std::abs (buffer.getSample (ch, n) - input.getSample (ch, n)));
    REQUIRE (diff == 0.0f);
}

TEST_CASE ("Ligar com fatia máxima e blocos irregulares: sem alocar, finito", "[39]")
{
    Glitch g;
    g.setSliceMs (Glitch::maxSliceMs);
    g.setFadeMs (Glitch::maxFadeMs);
    g.prepare ({ fs, 512, 2 });

    auto buffer = makeNoise (2, 3 * 48000, 0.5f);
    const auto sizes = awkwardBlockSizes();

    const auto allocations = countAllocations ([&] {
        processRange (g, buffer, 0, 48000);
        g.setActive (true);
        processInBlocks (g, buffer, sizes);
        g.setActive (false);
        processRange (g, buffer, 0, 4800);
    });

    REQUIRE (allocations == 0);
    REQUIRE (allFinite (buffer));
}
