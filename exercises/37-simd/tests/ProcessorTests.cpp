#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>
#include <vector>

using namespace learn::test;

namespace
{
    std::vector<float> randomData (int n, juce::int64 seed)
    {
        juce::Random random (seed);
        std::vector<float> v (static_cast<size_t> (n));
        for (auto& x : v)
            x = 2.0f * random.nextFloat() - 1.0f;
        return v;
    }

    template <typename Fn>
    double bestTime (Fn&& fn)
    {
        double best = 1.0e9;
        for (int run = 0; run < 7; ++run)
        {
            const auto start = juce::Time::getHighResolutionTicks();
            fn();
            best = std::min (best, juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - start));
        }
        return best;
    }

    volatile float sink = 0.0f; // impede o compilador de apagar contas cujo resultado não é usado
}

TEST_CASE ("mix (FloatVectorOperations) = dest += gain · src", "[37]")
{
    for (const auto n : { 0, 1, 3, 4, 5, 17, 1000, 4099 })
    {
        INFO (n << " amostras");
        const auto src = randomData (n + 3, 1);
        auto expected = randomData (n + 3, 2);
        auto actual = expected;

        // offsets 0..3: ponteiros alinhados e desalinhados
        for (int offset = 0; offset < 3; ++offset)
        {
            Simd::mixScalar (expected.data() + offset, src.data() + offset, 0.37f, n);
            Simd::mix (actual.data() + offset, src.data() + offset, 0.37f, n);
        }

        for (size_t i = 0; i < expected.size(); ++i)
            REQUIRE_THAT (actual[i], WithinAbs (expected[i], 1.0e-5));
    }
}

TEST_CASE ("peak (FloatVectorOperations) = max |x|", "[37]")
{
    REQUIRE (Simd::peak (nullptr, 0) == 0.0f);

    auto v = randomData (1001, 3);
    for (auto& x : v)
        x *= 0.5f;
    v[777] = -0.93f; // o pico é negativo
    REQUIRE (Simd::peak (v.data(), 1001) == 0.93f);
    v[12] = 0.95f;
    REQUIRE (Simd::peak (v.data(), 1001) == 0.95f);
    REQUIRE (Simd::peak (v.data() + 13, 5) <= 0.5f);
}

TEST_CASE ("sumOfSquares (SIMDRegister) = a referência, com qualquer alinhamento e tamanho", "[37]")
{
    INFO ("SIMDRegister<float> nesta máquina tem " << juce::dsp::SIMDRegister<float>::SIMDNumElements << " faixas");
    const auto data = randomData (5000, 4);

    for (const auto n : { 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 100, 1023, 4096 })
    {
        for (int offset = 0; offset < 4; ++offset)
        {
            INFO (n << " amostras a partir do índice " << offset);
            const auto expected = Simd::sumOfSquaresScalar (data.data() + offset, n);
            const auto actual = Simd::sumOfSquares (data.data() + offset, n);
            // a ordem das somas muda: o resultado não é bit a bit igual, mas quase
            REQUIRE_THAT (actual, WithinAbs (expected, 1.0e-6 + 1.0e-5 * std::abs (expected)));
        }
    }

    // rmsDecibels usa o teu sumOfSquares: um seno de amplitude 1 tem RMS −3.01 dB
    const auto sine = makeSine (1, 48000, 1000.0, 48000.0, 1.0f);
    REQUIRE_THAT (Simd::rmsDecibels (sine.getReadPointer (0), 48000), WithinAbs (-3.0103, 0.01));
}

TEST_CASE ("sumOfSquares com SIMD é pelo menos 2× mais rápida do que o ciclo escalar (em Release)", "[37]")
{
    const auto data = randomData (1 << 20, 5);
    const auto n = static_cast<int> (data.size());

    const auto scalarTime = bestTime ([&] { sink = Simd::sumOfSquaresScalar (data.data(), n); });
    const auto simdTime = bestTime ([&] { sink = Simd::sumOfSquares (data.data(), n); });
    const auto speedup = scalarTime / std::max (simdTime, 1.0e-9);

    // E o mix? O compilador costuma vetorizar SOZINHO um ciclo simples como o do mixScalar.
    auto dest = randomData (n, 6);
    const auto mixScalarTime = bestTime ([&] { Simd::mixScalar (dest.data(), data.data(), 0.5f, n); });
    const auto mixTime = bestTime ([&] { Simd::mix (dest.data(), data.data(), 0.5f, n); });

    INFO ("soma de quadrados: escalar " << scalarTime * 1.0e6 << " us, SIMD " << simdTime * 1.0e6 << " us (" << speedup << "x)");
    INFO ("mix: escalar " << mixScalarTime * 1.0e6 << " us, FloatVectorOperations " << mixTime * 1.0e6 << " us");

#if JUCE_DEBUG
    WARN ("Compilado em Debug: a medição de velocidade não conta. Usa ./learn check 37 (Release).");
    SUCCEED();
#else
    REQUIRE (speedup >= 2.0);
#endif
}

TEST_CASE ("Nada aloca", "[37]")
{
    auto a = randomData (512, 7);
    const auto b = randomData (512, 8);

    REQUIRE (countAllocations ([&] {
        Simd::mix (a.data(), b.data(), 0.5f, 512);
        sink = Simd::sumOfSquares (a.data() + 1, 511) + Simd::peak (a.data(), 512) + Simd::rmsDecibels (a.data(), 512);
    }) == 0);
}
