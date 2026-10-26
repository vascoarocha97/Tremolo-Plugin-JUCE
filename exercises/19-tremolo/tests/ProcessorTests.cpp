#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>

using namespace learn::test;

namespace
{
    juce::AudioBuffer<float> processDc (float depth, float rate, float stereoPhase, Lfo::Shape shape = Lfo::Shape::sine,
                                        int numSamples = 48000)
    {
        Tremolo tremolo;
        tremolo.setRate (rate);
        tremolo.setDepth (depth);
        tremolo.setShape (shape);
        tremolo.setStereoPhase (stereoPhase);
        tremolo.prepare ({ 48000.0, 512, 2 });

        auto buffer = makeConstant (2, numSamples, 1.0f);
        processInBlocks (tremolo, buffer, awkwardBlockSizes());
        return buffer;
    }

    float minOf (const juce::AudioBuffer<float>& b, int ch)
    {
        const auto* d = b.getReadPointer (ch);
        return *std::min_element (d, d + b.getNumSamples());
    }
}

TEST_CASE ("Com profundidade 1 o ganho vai de 0 a 1", "[19]")
{
    const auto out = processDc (1.0f, 5.0f, 0.0f);
    REQUIRE_THAT (minOf (out, 0), WithinAbs (0.0, 1.0e-3));
    REQUIRE_THAT (peak (out, 0), WithinAbs (1.0, 1.0e-3));
}

TEST_CASE ("Com profundidade 0.5 o ganho vai de 0.5 a 1", "[19]")
{
    const auto out = processDc (0.5f, 5.0f, 0.0f);
    REQUIRE_THAT (minOf (out, 0), WithinAbs (0.5, 1.0e-3));
    REQUIRE_THAT (peak (out, 0), WithinAbs (1.0, 1.0e-3));
}

TEST_CASE ("O ganho segue o LFO: 1 − profundidade·unipolar(LFO)", "[19]")
{
    const auto out = processDc (0.8f, 4.0f, 0.0f, Lfo::Shape::triangle, 12000);

    for (int n = 0; n < 12000; n += 37)
    {
        const auto lfo = Lfo::toUnipolar (Lfo::valueAt (Lfo::Shape::triangle, 4.0 * n / 48000.0));
        INFO ("amostra " << n);
        REQUIRE_THAT (out.getSample (0, n), WithinAbs (1.0 - 0.8 * lfo, 1.0e-4));
    }
}

TEST_CASE ("Velocidade: 5 Hz são 5 mínimos por segundo", "[19]")
{
    const auto out = processDc (1.0f, 5.0f, 0.0f);

    // conta as vezes que o ganho desce abaixo de 0.1 vindo de cima
    int dips = 0;
    bool above = true;

    for (int n = 0; n < out.getNumSamples(); ++n)
    {
        const auto g = out.getSample (0, n);

        if (above && g < 0.1f) { ++dips; above = false; }
        if (! above && g > 0.5f) above = true;
    }

    REQUIRE (dips == 5);
}

TEST_CASE ("Fase estéreo de 180°: quando um lado desce o outro sobe", "[19]")
{
    const auto out = processDc (1.0f, 3.0f, 180.0f);

    for (int n = 0; n < out.getNumSamples(); n += 101)
        REQUIRE_THAT (out.getSample (0, n) + out.getSample (1, n), WithinAbs (1.0, 1.0e-3));

    const auto inPhase = processDc (1.0f, 3.0f, 0.0f);

    for (int n = 0; n < inPhase.getNumSamples(); n += 101)
        REQUIRE (inPhase.getSample (0, n) == inPhase.getSample (1, n));
}

TEST_CASE ("Mudar a profundidade de repente não faz clicks", "[19]")
{
    Tremolo tremolo;
    tremolo.setRate (0.1f); // muito lento: as mudanças que se veem são da profundidade
    tremolo.setDepth (0.0f);
    tremolo.prepare ({ 48000.0, 512, 1 });

    auto buffer = makeConstant (1, 9600, 1.0f);

    for (int block = 0; block < 20; ++block)
    {
        tremolo.setDepth (block % 2 == 0 ? 1.0f : 0.0f);
        processRange (tremolo, buffer, block * 480, 480);
    }

    REQUIRE (maxSampleJump (buffer, 0) < 0.01f);
}

TEST_CASE ("process() não aloca", "[19]")
{
    Tremolo tremolo;
    tremolo.prepare ({ 48000.0, 512, 2 });
    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (tremolo, buffer, sizes); }) == 0);
}
