#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <set>

using namespace learn::test;

namespace
{
    juce::AudioBuffer<float> crush (const juce::AudioBuffer<float>& input, int bits, float factor, float mix = 1.0f)
    {
        Bitcrusher crusher;
        crusher.setBits (bits);
        crusher.setDownsampleFactor (factor);
        crusher.setMix (mix);
        crusher.prepare ({ 48000.0, 512, static_cast<juce::uint32> (input.getNumChannels()) });

        auto output = copyOf (input);
        processInBlocks (crusher, output, awkwardBlockSizes());
        return output;
    }

    /** Comprimentos dos "patamares" (amostras seguidas iguais) do canal 0. */
    std::vector<int> runLengths (const juce::AudioBuffer<float>& b, int count)
    {
        std::vector<int> runs;
        int length = 1;

        for (int n = 1; n < b.getNumSamples() && (int) runs.size() < count; ++n)
        {
            if (b.getSample (0, n) == b.getSample (0, n - 1))
            {
                ++length;
            }
            else
            {
                runs.push_back (length);
                length = 1;
            }
        }

        return runs;
    }
}

TEST_CASE ("Fator 4: cada valor fica 4 amostras, e é a 1.ª amostra do grupo (quantizada)", "[24]")
{
    const auto input = makeNoise (1, 4000, 0.9f, 3); // ruído: amostras vizinhas nunca são iguais
    const auto output = crush (input, 16, 4.0f);

    for (int n = 0; n < 4000; ++n)
    {
        const auto groupStart = (n / 4) * 4;
        INFO ("amostra " << n);
        REQUIRE (output.getSample (0, n) == Quantiser::quantise (input.getSample (0, groupStart), 16));
    }
}

TEST_CASE ("Fator 1 não segura nada; fator 2.5 alterna patamares de 2 e 3", "[24]")
{
    const auto input = makeNoise (1, 2000, 0.9f, 4);

    const auto noHold = crush (input, 16, 1.0f);
    for (int n = 0; n < 2000; ++n)
        REQUIRE (noHold.getSample (0, n) == Quantiser::quantise (input.getSample (0, n), 16));

    const auto runs = runLengths (crush (input, 16, 2.5f), 40);
    REQUIRE (runs.size() == 40);
    for (const auto r : runs)
        REQUIRE ((r == 2 || r == 3));

    // em média 2.5
    double total = 0.0;
    for (const auto r : runs) total += r;
    REQUIRE_THAT (total / runs.size(), WithinAbs (2.5, 0.05));
}

TEST_CASE ("Redução de bits: no máximo 2^bits níveis", "[24]")
{
    const auto output = crush (makeNoise (2, 8000, 0.99f), 3, 1.0f);
    std::set<float> levels;

    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 8000; ++n)
            levels.insert (output.getSample (ch, n));

    REQUIRE (levels.size() <= 8);
    REQUIRE (levels.size() >= 6);
}

TEST_CASE ("Sample & hold sem filtro => aliasing: 5 kHz com fator 8 aparece a 1 kHz", "[24]")
{
    // 48 kHz / 8 = 6 kHz de sample rate efetivo; 5 kHz está acima de 3 kHz (o novo Nyquist)
    // e dobra para 6 − 5 = 1 kHz (exercício 03).
    const auto output = crush (makeSine (1, 48000, 5000.0, 48000.0, 0.8f), 16, 8.0f);
    REQUIRE (toDb (amplitudeAt (output, 0, 1000.0, 48000.0)) > -12.0);
}

TEST_CASE ("Mix 0 deixa o sinal igual; process() não aloca", "[24]")
{
    const auto input = makeNoise (2, 4096);
    const auto output = crush (input, 4, 6.0f, 0.0f);
    REQUIRE (maxAbsDifference (output, input) == 0.0f);

    Bitcrusher crusher;
    crusher.setBits (6);
    crusher.setDownsampleFactor (3.3f);
    crusher.prepare ({ 48000.0, 512, 2 });
    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (crusher, buffer, sizes); }) == 0);
}
