#include <learn_testing/LearnTesting.h>

#include <cmath>

#include "Processor.h"

#include <numbers>

using namespace learn::test;

TEST_CASE ("aliasedFrequency: abaixo de Nyquist a frequência não muda", "[03]")
{
    REQUIRE_THAT (Nyquist::aliasedFrequency (0.0, 48000.0), WithinAbs (0.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (1000.0, 48000.0), WithinAbs (1000.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (20000.0, 44100.0), WithinAbs (20000.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (24000.0, 48000.0), WithinAbs (24000.0, 1.0e-9));
}

TEST_CASE ("aliasedFrequency: acima de Nyquist dobra para baixo", "[03]")
{
    REQUIRE_THAT (Nyquist::aliasedFrequency (30000.0, 48000.0), WithinAbs (18000.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (25000.0, 44100.0), WithinAbs (19100.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (47000.0, 48000.0), WithinAbs (1000.0, 1.0e-9));
}

TEST_CASE ("aliasedFrequency: múltiplos do sample rate e frequências muito altas", "[03]")
{
    REQUIRE_THAT (Nyquist::aliasedFrequency (48000.0, 48000.0), WithinAbs (0.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (50000.0, 48000.0), WithinAbs (2000.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (100000.0, 48000.0), WithinAbs (4000.0, 1.0e-9));
    REQUIRE_THAT (Nyquist::aliasedFrequency (2.5 * 48000.0 + 100.0, 48000.0), WithinAbs (23900.0, 1.0e-9));
}

TEST_CASE ("aliasedFrequency: o resultado bate com o que um seno amostrado realmente faz", "[03]")
{
    // Amostrar sin(2π·f·n/fs) com f acima de Nyquist dá exatamente as mesmas amostras (a menos do sinal)
    // que um seno à frequência de alias.
    constexpr double fs = 8000.0;

    for (const double f : { 5000.0, 7300.0, 9100.0, 13500.0 })
    {
        const auto alias = Nyquist::aliasedFrequency (f, fs);
        INFO ("f = " << f << " Hz, alias calculado = " << alias << " Hz");

        for (int n = 0; n < 50; ++n)
        {
            const auto original = std::sin (2.0 * std::numbers::pi * f * n / fs);
            const auto aliased = std::sin (2.0 * std::numbers::pi * alias * n / fs);
            REQUIRE_THAT (std::abs (original), WithinAbs (std::abs (aliased), 1.0e-6));
        }
    }
}

TEST_CASE ("NaiveSawOscillator: rampa de -1 a +1", "[03]")
{
    NaiveSawOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setFrequency (1000.0f); // 48 amostras por período

    juce::AudioBuffer<float> buffer (1, 96);
    osc.processBlock (buffer);

    REQUIRE_THAT (buffer.getSample (0, 0), WithinAbs (-1.0, 1.0e-6));
    REQUIRE_THAT (buffer.getSample (0, 1), WithinAbs (-1.0 + 2.0 / 48.0, 1.0e-5));
    REQUIRE_THAT (buffer.getSample (0, 24), WithinAbs (0.0, 1.0e-5));
    REQUIRE_THAT (buffer.getSample (0, 47), WithinAbs (1.0 - 2.0 / 48.0, 1.0e-5));
    // Na amostra 48 a fase volta a 0: conforme o arredondamento, pode dar ≈ −1 ou ≈ +1 (é a
    // descontinuidade). A seguinte já é inequívoca: recomeçou.
    REQUIRE_THAT (std::abs (buffer.getSample (0, 48)), WithinAbs (1.0, 1.0e-4));
    REQUIRE_THAT (buffer.getSample (0, 49), WithinAbs (-1.0 + 2.0 / 48.0, 1.0e-4));
    REQUIRE_THAT (mean (buffer, 0), WithinAbs (-1.0 / 48.0, 1.0e-4));
}

TEST_CASE ("NaiveSawOscillator: continua entre blocos e escreve todos os canais", "[03]")
{
    NaiveSawOscillator reference;
    reference.prepareToPlay (44100.0, 8192);
    reference.setFrequency (311.13f);
    juce::AudioBuffer<float> expected (2, 8192);
    reference.processBlock (expected);

    NaiveSawOscillator osc;
    osc.prepareToPlay (44100.0, 512);
    osc.setFrequency (311.13f);
    juce::AudioBuffer<float> buffer (2, 8192);
    buffer.clear();
    processInBlocks (osc, buffer, awkwardBlockSizes());

    REQUIRE (peak (buffer, 1) > 0.9f);
    REQUIRE (maxAbsDifference (buffer, expected) < 1.0e-5f);
}

TEST_CASE ("O dente de serra ingénuo tem aliasing exatamente onde a fórmula prevê", "[03]")
{
    // f0 = 2500 Hz a 48 kHz: os harmónicos 10..19 (25 kHz..47.5 kHz) estão acima de Nyquist.
    // Um dente de serra ideal só teria energia em múltiplos de 2500 Hz.
    constexpr double fs = 48000.0, f0 = 2500.0;

    NaiveSawOscillator osc;
    osc.prepareToPlay (fs, 48000);
    osc.setFrequency (static_cast<float> (f0));
    juce::AudioBuffer<float> buffer (1, 48000);
    osc.processBlock (buffer);

    for (int k = 10; k < 20; ++k)
    {
        const auto alias = Nyquist::aliasedFrequency (k * f0, fs);
        const auto expectedAmplitude = 2.0 / (std::numbers::pi * k); // amplitude do harmónico k
        INFO ("harmónico " << k << " (" << k * f0 << " Hz) -> alias em " << alias << " Hz");

        REQUIRE (std::fmod (alias, f0) > 1.0); // não cai em cima de um harmónico verdadeiro
        REQUIRE (amplitudeAt (buffer, 0, alias, fs) > 0.4 * expectedAmplitude);
    }
}

TEST_CASE ("NaiveSawOscillator::processBlock não aloca memória", "[03]")
{
    NaiveSawOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (2, 2048);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (osc, buffer, sizes); }) == 0);
}
