#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

// Uma mini-validação ao estilo do pluginval (Tracktion): o que um host vai fazer ao teu plugin.

namespace
{
    constexpr double fs = 48000.0;

    void set (EchoProcessor& p, const char* id, float realValue)
    {
        auto* param = p.apvts.getParameter (id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    }

    juce::AudioProcessor::BusesLayout layout (const juce::AudioChannelSet& in, const juce::AudioChannelSet& out)
    {
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (in);
        l.outputBuses.add (out);
        return l;
    }
}

TEST_CASE ("Layouts: mono→mono e estéreo→estéreo sim; o resto não", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoPlugin p;
    using Set = juce::AudioChannelSet;

    REQUIRE (p.checkBusesLayoutSupported (layout (Set::stereo(), Set::stereo())));
    REQUIRE (p.checkBusesLayoutSupported (layout (Set::mono(), Set::mono())));
    REQUIRE_FALSE (p.checkBusesLayoutSupported (layout (Set::mono(), Set::stereo())));
    REQUIRE_FALSE (p.checkBusesLayoutSupported (layout (Set::create5point1(), Set::create5point1())));
    REQUIRE_FALSE (p.checkBusesLayoutSupported (layout (Set::disabled(), Set::stereo())));

    // E funciona mesmo em mono
    REQUIRE (p.setBusesLayout (layout (Set::mono(), Set::mono())));
    set (p, EchoParams::time, 100.0f);
    set (p, EchoParams::mix, 1.0f);
    p.prepareToPlay (fs, 512);
    auto buffer = makeImpulse (1, 12000, 0);
    processInBlocks (p, buffer, { 512 });
    REQUIRE (allFinite (buffer));
    REQUIRE (peak (buffer, 0, 4700, 200) > 0.3f);
}

TEST_CASE ("Cauda: tempo × (1 + ln 0.001 / ln fb)", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoPlugin p;

    set (p, EchoParams::time, 500.0f);
    set (p, EchoParams::feedback, 0.5f);
    REQUIRE_THAT (p.getTailLengthSeconds(), WithinRel (0.5 * (1.0 + std::log (0.001) / std::log (0.5)), 0.01));

    set (p, EchoParams::feedback, 0.0f);
    REQUIRE_THAT (p.getTailLengthSeconds(), WithinRel (0.5, 0.01));

    set (p, EchoParams::time, 1000.0f);
    set (p, EchoParams::feedback, 0.95f);
    REQUIRE (p.getTailLengthSeconds() > 100.0); // ~135 s: é mesmo longo
}

TEST_CASE ("O parâmetro de bypass do host é o nosso parâmetro bypass", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoPlugin p;

    REQUIRE (p.getBypassParameter() != nullptr);
    REQUIRE (p.getBypassParameter() == p.apvts.getParameter (EchoParams::bypass));
}

TEST_CASE ("Bypass sem cliques: crossfade de ~20 ms e depois exatamente a entrada", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoPlugin p;
    set (p, EchoParams::mix, 1.0f);
    set (p, EchoParams::time, 50.0f);
    p.prepareToPlay (fs, 480);

    const auto input = makeSine (2, 48000, 220.0, fs, 0.5f);
    auto buffer = copyOf (input);

    for (int start = 0; start < 24000; start += 480)
        processRange (p, buffer, start, 480);

    set (p, EchoParams::bypass, 1.0f); // o utilizador carrega no bypass a meio

    for (int start = 24000; start < 48000; start += 480)
        processRange (p, buffer, start, 480);

    auto maxDiff = [&] (int from, int to)
    {
        float diff = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int n = from; n < to; ++n)
                diff = juce::jmax (diff, std::abs (buffer.getSample (ch, n) - input.getSample (ch, n)));
        return diff;
    };

    // logo a seguir: ainda não é a entrada (está a meio do crossfade)...
    REQUIRE (maxDiff (24000, 24480) > 0.01f);
    // ...20 ms depois é exatamente a entrada
    const auto diff = maxDiff (24000 + 1200, 48000);
    REQUIRE (diff == 0.0f);
}

TEST_CASE ("Bypass: ligar a meio não salta (sem degraus maiores do que o próprio sinal)", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoPlugin p;
    set (p, EchoParams::mix, 1.0f);
    set (p, EchoParams::feedback, 0.0f);
    set (p, EchoParams::time, 3.0f); // o eco é o seno atrasado ~3 ms: dry e wet muito diferentes
    p.prepareToPlay (fs, 480);

    const auto input = makeSine (2, 9600, 220.0, fs, 0.5f);
    auto buffer = copyOf (input);
    processRange (p, buffer, 0, 4800);
    set (p, EchoParams::bypass, 1.0f);
    processRange (p, buffer, 4800, 4800);

    // O maior passo entre amostras de um seno de 220 Hz e amplitude 0.5 é ~0.0144.
    REQUIRE (maxSampleJump (buffer, 0) < 0.03f);
}

TEST_CASE ("Editor, estado e programas: as peças dos exercícios 33 e 34 estão ligadas", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoPlugin a, b;

    REQUIRE (a.hasEditor());
    std::unique_ptr<juce::AudioProcessorEditor> editor (a.createEditorAndMakeActive());
    REQUIRE (dynamic_cast<EchoEditor*> (editor.get()) != nullptr);
    a.editorBeingDeleted (editor.get());
    editor.reset();

    set (a, EchoParams::time, 777.0f);
    juce::MemoryBlock state;
    a.getStateInformation (state);
    b.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE_THAT (b.apvts.getRawParameterValue (EchoParams::time)->load(), WithinAbs (777.0, 0.5));

    REQUIRE (a.getNumPrograms() == static_cast<int> (factoryPresets().size()));
    REQUIRE (a.getName() == "Eco");
    REQUIRE_FALSE (a.acceptsMidi());
}

TEST_CASE ("Mini-pluginval: sample rates, blocos irregulares (até 2× o prometido), automação", "[35]")
{
    const juce::ScopedJuceInitialiser_GUI gui;

    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        for (const auto blockSize : { 1, 64, 512 })
        {
            INFO (sampleRate << " Hz, bloco " << blockSize);
            EchoPlugin p;
            p.prepareToPlay (sampleRate, blockSize);

            auto buffer = makeNoise (2, static_cast<int> (sampleRate / 4), 0.5f);
            juce::Random random (static_cast<juce::int64> (sampleRate) + blockSize);
            std::vector<int> sizes;
            for (int i = 0; i < 64; ++i)
                sizes.push_back (1 + random.nextInt (2 * blockSize)); // alguns hosts excedem o prometido

            int position = 0, allocations = 0;
            size_t i = 0;

            while (position < buffer.getNumSamples())
            {
                for (auto* param : p.getParameters())
                    if (random.nextInt (4) == 0)
                        param->setValueNotifyingHost (random.nextFloat());

                const auto num = juce::jmin (sizes[i++ % sizes.size()], buffer.getNumSamples() - position);
                allocations += countAllocations ([&] { processRange (p, buffer, position, num); });
                position += num;
            }

            REQUIRE (allocations == 0);
            REQUIRE (allFinite (buffer));
            REQUIRE (peak (buffer, 0) < 8.0f);

            // O host pode parar e voltar a preparar
            p.releaseResources();
            p.prepareToPlay (sampleRate, blockSize);
            auto again = makeNoise (2, 1024, 0.5f);
            processInBlocks (p, again, { blockSize });
            REQUIRE (allFinite (again));
        }
    }
}
