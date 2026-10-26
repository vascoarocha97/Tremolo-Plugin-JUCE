#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <numbers>

using namespace learn::test;

namespace
{
    juce::AudioBuffer<float> render (SineOscillator& osc, int numChannels, int numSamples)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        buffer.clear();
        osc.processBlock (buffer);
        return buffer;
    }
}

TEST_CASE ("As primeiras amostras seguem amplitude · sin(2π·f·n/fs)", "[02]")
{
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setFrequency (1000.0f);
    osc.setAmplitude (0.5f);

    const auto buffer = render (osc, 1, 64);

    for (int n = 0; n < 64; ++n)
    {
        const auto expected = 0.5 * std::sin (2.0 * std::numbers::pi * 1000.0 * n / 48000.0);
        INFO ("amostra " << n);
        REQUIRE_THAT (buffer.getSample (0, n), WithinAbs (expected, 1.0e-5));
    }
}

TEST_CASE ("A frequência está certa para vários sample rates", "[02]")
{
    const auto sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    INFO ("sample rate: " << sampleRate);

    SineOscillator osc;
    osc.prepareToPlay (sampleRate, 512);
    osc.setFrequency (1000.0f);
    osc.setAmplitude (0.8f);

    const auto buffer = render (osc, 1, static_cast<int> (sampleRate)); // 1 segundo

    REQUIRE_THAT (estimateFrequency (buffer, 0, sampleRate), WithinRel (1000.0, 0.002));
    REQUIRE_THAT (amplitudeAt (buffer, 0, 1000.0, sampleRate), WithinAbs (0.8, 0.01));
}

TEST_CASE ("Escreve o mesmo seno em todos os canais", "[02]")
{
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setFrequency (440.0f);

    const auto buffer = render (osc, 3, 512);

    REQUIRE (peak (buffer, 0) > 0.1f);

    for (int n = 0; n < 512; ++n)
    {
        INFO ("amostra " << n);
        REQUIRE (buffer.getSample (1, n) == buffer.getSample (0, n));
        REQUIRE (buffer.getSample (2, n) == buffer.getSample (0, n));
    }
}

TEST_CASE ("A fase continua entre blocos de qualquer tamanho", "[02]")
{
    // Referência: tudo num único bloco
    SineOscillator reference;
    reference.prepareToPlay (48000.0, 8192);
    reference.setFrequency (523.25f);
    const auto expected = render (reference, 2, 8192);

    // O mesmo, mas em blocos de 1, 7, 64, 13, ...
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setFrequency (523.25f);
    juce::AudioBuffer<float> buffer (2, 8192);
    buffer.clear();
    processInBlocks (osc, buffer, awkwardBlockSizes());

    REQUIRE (peak (buffer, 0) > 0.1f);
    REQUIRE (maxAbsDifference (buffer, expected) < 1.0e-5f);
}

TEST_CASE ("Mudar a frequência entre blocos não cria saltos (sem clicks)", "[02]")
{
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setAmplitude (1.0f);

    juce::AudioBuffer<float> buffer (1, 4096);
    buffer.clear();

    const float frequencies[] = { 200.0f, 2000.0f, 300.0f, 5000.0f, 100.0f, 1000.0f, 440.0f, 3000.0f };

    for (int block = 0; block < 8; ++block)
    {
        osc.setFrequency (frequencies[block]);
        processRange (osc, buffer, block * 512, 512);
    }

    // O maior salto possível entre amostras de um seno de amplitude 1 a 5 kHz é ≈ 2π·5000/48000 ≈ 0.65.
    // Se a fase "recomeçar" ou saltar na mudança de frequência, o salto passa disto.
    REQUIRE (peak (buffer, 0) > 0.9f);
    REQUIRE (maxSampleJump (buffer, 0) < 0.67f);
}

TEST_CASE ("A fase fica sempre em [0, 2π) (não cresce sem limite)", "[02]")
{
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 4800);
    osc.setFrequency (1234.5f);

    juce::AudioBuffer<float> buffer (1, 4800);

    for (int block = 0; block < 600; ++block) // 1 minuto de áudio
    {
        osc.processBlock (buffer);
        INFO ("bloco " << block);
        REQUIRE (osc.getPhase() >= 0.0);
        REQUIRE (osc.getPhase() < 2.0 * std::numbers::pi);
    }

    REQUIRE_THAT (estimateFrequency (buffer, 0, 48000.0), WithinRel (1234.5, 0.005));
}

TEST_CASE ("reset() volta a fase a zero", "[02]")
{
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setFrequency (1000.0f);

    juce::AudioBuffer<float> buffer (1, 100);
    osc.processBlock (buffer);
    REQUIRE (osc.getPhase() > 0.0);

    osc.reset();
    osc.processBlock (buffer);
    REQUIRE_THAT (buffer.getSample (0, 0), WithinAbs (0.0, 1.0e-6));
}

TEST_CASE ("processBlock não aloca memória", "[02]")
{
    SineOscillator osc;
    osc.prepareToPlay (48000.0, 512);
    osc.setFrequency (440.0f);

    juce::AudioBuffer<float> buffer (2, 512);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (osc, buffer, sizes); }) == 0);
}
