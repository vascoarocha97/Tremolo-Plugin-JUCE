#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    juce::RangedAudioParameter& parameter (EchoProcessor& p, const char* id)
    {
        auto* param = p.apvts.getParameter (id);
        INFO ("parâmetro \"" << id << "\"");
        REQUIRE (param != nullptr);
        return *param;
    }

    /** Muda um parâmetro como o host faria (valor normalizado 0..1). */
    void set (EchoProcessor& p, const char* id, float realValue)
    {
        auto& param = parameter (p, id);
        param.setValueNotifyingHost (param.convertTo0to1 (realValue));
    }

    float defaultOf (juce::RangedAudioParameter& param)
    {
        return param.convertFrom0to1 (param.getDefaultValue());
    }
}

TEST_CASE ("Layout: 7 parâmetros com IDs, versão, gamas e valores por omissão", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui; // o APVTS usa um Timer: precisa da message thread
    EchoProcessor p;

    REQUIRE (p.getParameters().size() == 7);

    struct Expected { const char* id; float min, max, def; };
    const Expected floats[] { { EchoParams::gain, -24.0f, 12.0f, 0.0f },
                              { EchoParams::time, 1.0f, 1000.0f, 350.0f },
                              { EchoParams::feedback, 0.0f, 0.95f, 0.4f },
                              { EchoParams::tone, 500.0f, 20000.0f, 6000.0f },
                              { EchoParams::mix, 0.0f, 1.0f, 0.3f } };

    for (const auto& e : floats)
    {
        INFO ("parâmetro \"" << e.id << "\"");
        auto& param = parameter (p, e.id);
        REQUIRE (dynamic_cast<juce::AudioParameterFloat*> (&param) != nullptr);
        REQUIRE_THAT (param.getNormalisableRange().start, WithinAbs (e.min, 1.0e-4));
        REQUIRE_THAT (param.getNormalisableRange().end, WithinAbs (e.max, 1.0e-4));
        REQUIRE_THAT (defaultOf (param), WithinAbs (e.def, 0.05));
        REQUIRE (param.getVersionHint() == EchoParams::version);
    }

    REQUIRE (parameter (p, EchoParams::gain).getLabel() == "dB");

    auto* mode = dynamic_cast<juce::AudioParameterChoice*> (&parameter (p, EchoParams::mode));
    REQUIRE (mode != nullptr);
    REQUIRE (mode->choices == juce::StringArray { "Mono", "Ping-pong" });
    REQUIRE (mode->getIndex() == 0);
    REQUIRE (mode->getVersionHint() == EchoParams::version);

    auto* bypass = dynamic_cast<juce::AudioParameterBool*> (&parameter (p, EchoParams::bypass));
    REQUIRE (bypass != nullptr);
    REQUIRE_FALSE (bypass->get());
    REQUIRE (bypass->getVersionHint() == EchoParams::version);
}

TEST_CASE ("Skew: o meio do curso do tempo é 250 ms e o do tom é 3 kHz", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;

    REQUIRE_THAT (parameter (p, EchoParams::time).convertFrom0to1 (0.5f), WithinAbs (250.0, 1.0));
    REQUIRE_THAT (parameter (p, EchoParams::tone).convertFrom0to1 (0.5f), WithinAbs (3000.0, 5.0));
}

TEST_CASE ("O processBlock lê os valores reais dos parâmetros: ganho −6 dB com mistura 0", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);

    set (p, EchoParams::gain, -6.0f);
    set (p, EchoParams::mix, 0.0f);
    REQUIRE_THAT (p.apvts.getRawParameterValue (EchoParams::gain)->load(), WithinAbs (-6.0, 1.0e-3)); // valor real, não 0..1

    p.prepareToPlay (fs, 512);
    const auto input = makeSine (2, 24000, 440.0, fs, 0.5f);
    auto buffer = copyOf (input);
    processInBlocks (p, buffer, { 512 });

    REQUIRE_THAT (gainDb (input, buffer, 0, 4800), WithinAbs (-6.0, 0.1));
    REQUIRE_THAT (gainDb (input, buffer, 1, 4800), WithinAbs (-6.0, 0.1));

    // E muda durante a reprodução (o smoothing leva ~20 ms)
    set (p, EchoParams::gain, 0.0f);
    buffer = copyOf (input);
    processInBlocks (p, buffer, { 512 });
    REQUIRE_THAT (gainDb (input, buffer, 0, 4800), WithinAbs (0.0, 0.1));
}

TEST_CASE ("Tempo: com mistura 1 e feedback 0, um impulso sai exatamente 100 ms depois", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);

    set (p, EchoParams::time, 100.0f);
    set (p, EchoParams::feedback, 0.0f);
    set (p, EchoParams::mix, 1.0f);
    set (p, EchoParams::tone, 20000.0f);
    p.prepareToPlay (fs, 512); // depois de mudar os parâmetros: os smoothers começam no valor certo

    auto buffer = makeImpulse (2, 12000, 0);
    processInBlocks (p, buffer, awkwardBlockSizes());

    REQUIRE (firstSampleAbove (buffer, 0, 0.1f) == 4800);
    REQUIRE (firstSampleAbove (buffer, 1, 0.1f) == 4800);
    REQUIRE (peak (buffer, 0, 6000) < 1.0e-4f); // feedback 0: só um eco
}

TEST_CASE ("Modo ping-pong: o 1.º eco sai só à esquerda, o 2.º só à direita", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui;

    auto render = [] (int modeIndex)
    {
        EchoProcessor p;
        REQUIRE (p.getParameters().size() == 7);
        set (p, EchoParams::time, 100.0f);
        set (p, EchoParams::feedback, 0.5f);
        set (p, EchoParams::mix, 1.0f);
        set (p, EchoParams::tone, 20000.0f);
        set (p, EchoParams::mode, static_cast<float> (modeIndex));
        p.prepareToPlay (fs, 512);

        auto buffer = makeImpulse (2, 12000, 0);
        processInBlocks (p, buffer, { 512 });
        return buffer;
    };

    const auto mono = render (0);
    REQUIRE (std::abs (mono.getSample (0, 4800)) > 0.3f);
    REQUIRE (std::abs (mono.getSample (1, 4800)) > 0.3f);

    const auto pingPong = render (1);
    REQUIRE (std::abs (pingPong.getSample (0, 4800)) > 0.3f);
    REQUIRE (peak (pingPong, 1, 0, 9000) < 1.0e-4f);
    REQUIRE (peak (pingPong, 1, 9000, 1000) > 0.1f);
    REQUIRE (peak (pingPong, 0, 6000, 6000) < 1.0e-4f);
}

TEST_CASE ("Bypass: a saída é exatamente a entrada", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);

    set (p, EchoParams::bypass, 1.0f);
    set (p, EchoParams::gain, 12.0f);
    p.prepareToPlay (fs, 512);

    const auto input = makeNoise (2, 4800, 0.5f);
    auto buffer = copyOf (input);
    processInBlocks (p, buffer, awkwardBlockSizes());

    REQUIRE (maxAbsDifference (input, buffer) == 0.0f);
}

TEST_CASE ("Automação entre blocos: sem alocações e sem NaN", "[32]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    p.prepareToPlay (fs, 512);

    auto buffer = makeNoise (2, 48000, 0.5f);
    juce::Random random (5);
    const auto sizes = awkwardBlockSizes();
    int position = 0, allocations = 0;
    size_t i = 0;

    while (position < buffer.getNumSamples())
    {
        // o "host" mexe em tudo entre blocos (isto não é contado: é o host)
        for (auto* param : p.getParameters())
            param->setValueNotifyingHost (random.nextFloat());

        const auto num = juce::jmin (sizes[i++ % sizes.size()], buffer.getNumSamples() - position);
        allocations += countAllocations ([&] { processRange (p, buffer, position, num); });
        position += num;
    }

    REQUIRE (allocations == 0);
    REQUIRE (allFinite (buffer));
}
