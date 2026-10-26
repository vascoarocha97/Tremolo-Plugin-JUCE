// Testes dos próprios helpers (para confiarmos neles nos exercícios).

#include <learn_testing/LearnTesting.h>

#include <vector>

using namespace learn::test;

TEST_CASE ("ScopedAllocationCounter apanha new/std::vector", "[selftest]")
{
    ScopedAllocationCounter counter;
    std::vector<float> v (100);
    REQUIRE (counter.getCount() >= 1);
}

TEST_CASE ("ScopedAllocationCounter apanha AudioBuffer::setSize (malloc) em Debug", "[selftest]")
{
    if (! allocationTrackingCoversMalloc())
        SKIP ("Só em Debug com MSVC");

    juce::AudioBuffer<float> buffer;
    ScopedAllocationCounter counter;
    buffer.setSize (2, 4096);
    REQUIRE (counter.getCount() >= 1);
}

TEST_CASE ("ScopedAllocationCounter dá zero quando não há alocações", "[selftest]")
{
    auto buffer = makeNoise (2, 512);
    ScopedAllocationCounter counter;
    buffer.applyGain (0.5f);
    REQUIRE (counter.getCount() == 0);
}

TEST_CASE ("processInBlocks não aloca", "[selftest]")
{
    struct Nothing
    {
        void processBlock (juce::AudioBuffer<float>&) {}
    } nothing;

    auto buffer = makeNoise (2, 2048);
    const auto sizes = awkwardBlockSizes(); // criar o vector aloca: fora da contagem
    REQUIRE (countAllocations ([&] { processInBlocks (nothing, buffer, sizes); }) == 0);
}

TEST_CASE ("amplitudeAt mede a amplitude de um seno", "[selftest]")
{
    const auto sine = makeSine (1, 4800, 1000.0, 48000.0, 0.5f);
    REQUIRE_THAT (amplitudeAt (sine, 0, 1000.0, 48000.0), WithinAbs (0.5, 1.0e-3));
    REQUIRE (amplitudeAt (sine, 0, 3000.0, 48000.0) < 1.0e-3);

    // Frequência fora de um bin exato: erro < 1.5 dB
    const auto sine2 = makeSine (1, 4800, 1234.5, 48000.0, 0.5f);
    REQUIRE (std::abs (toDb (amplitudeAt (sine2, 0, 1234.5, 48000.0) / 0.5)) < 1.5);
}

TEST_CASE ("estimateFrequency", "[selftest]")
{
    const auto sine = makeSine (1, 48000, 440.0, 48000.0);
    REQUIRE_THAT (estimateFrequency (sine, 0, 48000.0), WithinRel (440.0, 0.01));
}
