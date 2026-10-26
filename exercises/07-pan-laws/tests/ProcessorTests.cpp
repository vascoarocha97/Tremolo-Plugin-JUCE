#include <learn_testing/LearnTesting.h>
#include <juce_dsp/juce_dsp.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

TEST_CASE ("Extremos: −1 só esquerda, +1 só direita (todas as leis)", "[07]")
{
    for (auto law : { PanLaw::linear, PanLaw::constantPower, PanLaw::compromise4p5dB })
    {
        INFO ("lei " << static_cast<int> (law));
        const auto hardLeft = Pan::computeGains (-1.0f, law);
        const auto hardRight = Pan::computeGains (1.0f, law);

        REQUIRE_THAT (hardLeft.left, WithinAbs (1.0, 1.0e-6));
        REQUIRE_THAT (hardLeft.right, WithinAbs (0.0, 1.0e-6));
        REQUIRE_THAT (hardRight.left, WithinAbs (0.0, 1.0e-6));
        REQUIRE_THAT (hardRight.right, WithinAbs (1.0, 1.0e-6));
    }
}

TEST_CASE ("Ganho no centro: −6 dB, −3 dB e −4.5 dB", "[07]")
{
    const auto linear = Pan::computeGains (0.0f, PanLaw::linear);
    const auto power = Pan::computeGains (0.0f, PanLaw::constantPower);
    const auto compromise = Pan::computeGains (0.0f, PanLaw::compromise4p5dB);

    REQUIRE_THAT (toDb (linear.left), WithinAbs (-6.02, 0.01));
    REQUIRE_THAT (toDb (power.left), WithinAbs (-3.01, 0.01));
    REQUIRE_THAT (toDb (compromise.left), WithinAbs (-4.52, 0.01));

    REQUIRE (linear.left == linear.right);
    REQUIRE_THAT (power.left, WithinAbs (power.right, 1.0e-6));
}

TEST_CASE ("Lei linear: L + R = 1 em qualquer posição (soma mono constante)", "[07]")
{
    for (float p = -1.0f; p <= 1.0f; p += 0.05f)
    {
        INFO ("pan = " << p);
        const auto g = Pan::computeGains (p, PanLaw::linear);
        REQUIRE_THAT (g.left + g.right, WithinAbs (1.0, 1.0e-6));
    }
}

TEST_CASE ("Potência constante: L² + R² = 1 em qualquer posição", "[07]")
{
    for (float p = -1.0f; p <= 1.0f; p += 0.05f)
    {
        INFO ("pan = " << p);
        const auto g = Pan::computeGains (p, PanLaw::constantPower);
        REQUIRE_THAT (g.left * g.left + g.right * g.right, WithinAbs (1.0, 1.0e-5));
    }
}

TEST_CASE ("O pan é limitado a [-1, 1] e é monótono", "[07]")
{
    const auto beyond = Pan::computeGains (3.0f, PanLaw::constantPower);
    REQUIRE_THAT (beyond.right, WithinAbs (1.0, 1.0e-6));
    REQUIRE_THAT (beyond.left, WithinAbs (0.0, 1.0e-6));

    float previousRight = -1.0f;

    for (float p = -1.0f; p <= 1.0f; p += 0.1f)
    {
        const auto g = Pan::computeGains (p, PanLaw::compromise4p5dB);
        REQUIRE (g.right >= previousRight);
        previousRight = g.right;
    }
}

TEST_CASE ("Comparação com juce::dsp::Panner (que normaliza o centro para 0 dB)", "[07]")
{
    // O Panner do JUCE multiplica os ganhos por um "boost" para o centro ficar a 0 dB:
    // sin3dB -> ×√2, sin4p5dB -> ×2^(3/4), linear -> ×2. As curvas são as mesmas que as tuas.
    struct Case { PanLaw law; juce::dsp::PannerRule rule; float boost; };
    const Case cases[] = { { PanLaw::linear, juce::dsp::PannerRule::linear, 2.0f },
                           { PanLaw::constantPower, juce::dsp::PannerRule::sin3dB, std::sqrt (2.0f) },
                           { PanLaw::compromise4p5dB, juce::dsp::PannerRule::sin4p5dB, std::pow (2.0f, 0.75f) } };

    for (const auto& c : cases)
    {
        for (const auto p : { -0.8f, -0.3f, 0.0f, 0.5f, 0.9f })
        {
            juce::dsp::Panner<float> panner;
            panner.setRule (c.rule);
            panner.setPan (p);
            panner.prepare ({ 48000.0, 64, 2 });

            auto buffer = makeConstant (2, 64, 1.0f);
            juce::dsp::AudioBlock<float> block (buffer);
            panner.process (juce::dsp::ProcessContextReplacing<float> (block));

            const auto mine = Pan::computeGains (p, c.law);
            INFO ("lei " << static_cast<int> (c.law) << ", pan " << p);
            REQUIRE_THAT (mine.left * c.boost, WithinAbs (buffer.getSample (0, 63), 1.0e-4));
            REQUIRE_THAT (mine.right * c.boost, WithinAbs (buffer.getSample (1, 63), 1.0e-4));
        }
    }
}

TEST_CASE ("StereoPanner coloca a média dos canais na posição pedida", "[07]")
{
    StereoPanner panner;
    panner.prepareToPlay (48000.0, 512);
    panner.setLaw (PanLaw::constantPower);
    panner.setPan (0.5f);

    juce::AudioBuffer<float> buffer (2, 512);
    for (int n = 0; n < 512; ++n)
    {
        buffer.setSample (0, n, 0.6f); // L e R diferentes: a fonte mono é a média (0.4)
        buffer.setSample (1, n, 0.2f);
    }

    processInBlocks (panner, buffer, awkwardBlockSizes());

    const auto g = Pan::computeGains (0.5f, PanLaw::constantPower);
    REQUIRE_THAT (buffer.getSample (0, 300), WithinAbs (0.4f * g.left, 1.0e-6));
    REQUIRE_THAT (buffer.getSample (1, 300), WithinAbs (0.4f * g.right, 1.0e-6));
}

TEST_CASE ("StereoPanner deixa buffers mono intactos e não aloca", "[07]")
{
    StereoPanner panner;
    panner.prepareToPlay (48000.0, 512);
    panner.setPan (-0.7f);

    auto mono = makeNoise (1, 128);
    const auto input = copyOf (mono);
    panner.processBlock (mono);
    REQUIRE (maxAbsDifference (mono, input) == 0.0f);

    auto stereo = makeNoise (2, 512);
    REQUIRE (countAllocations ([&] { panner.processBlock (stereo); }) == 0);
}
