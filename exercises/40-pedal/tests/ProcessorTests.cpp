#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <set>

using namespace learn::test;

// Testes de caixa preta: só usam a interface do juce::AudioProcessor e os IDs da especificação
// (README). Não importa como organizas o código por dentro.

namespace
{
    constexpr double fs = 48000.0;

    juce::RangedAudioParameter* find (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* param : p.getParameters())
            if (auto* withId = dynamic_cast<juce::RangedAudioParameter*> (param))
                if (withId->getParameterID() == id)
                    return withId;
        return nullptr;
    }

    void set (juce::AudioProcessor& p, const juce::String& id, float realValue)
    {
        auto* param = find (p, id);
        INFO ("parâmetro \"" << id << "\"");
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    }

    /** Tudo "neutro": cada teste liga só o que quer medir. */
    void allOff (juce::AudioProcessor& p)
    {
        set (p, "fuzz_on", 0.0f);
        set (p, "crush_on", 0.0f);
        set (p, "glitch_on", 0.0f);
        set (p, "reverb_mix", 0.0f);
        set (p, "output", 0.0f);
        set (p, "bypass", 0.0f);
    }

    void processAll (juce::AudioProcessor& p, juce::AudioBuffer<float>& buffer, int blockSize = 480)
    {
        for (int start = 0; start < buffer.getNumSamples(); start += blockSize)
            processRange (p, buffer, start, juce::jmin (blockSize, buffer.getNumSamples() - start));
    }

    int peakIndex (const juce::AudioBuffer<float>& b, int channel)
    {
        int index = 0;
        for (int n = 1; n < b.getNumSamples(); ++n)
            if (std::abs (b.getSample (channel, n)) > std::abs (b.getSample (channel, index)))
                index = n;
        return index;
    }
}

TEST_CASE ("Parâmetros: IDs, tipos, gamas e valores por omissão da especificação", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor p;

    struct Spec { const char* id; const char* type; float min, max, def; };
    const Spec spec[] { { "fuzz_on", "bool", 0, 1, 1 },
                        { "fuzz_drive", "float", 0, 40, 20 },
                        { "fuzz_tone", "float", 500, 12000, 4000 },
                        { "crush_on", "bool", 0, 1, 0 },
                        { "crush_bits", "int", 2, 16, 8 },
                        { "crush_downsample", "int", 1, 32, 1 },
                        { "glitch_on", "bool", 0, 1, 0 },
                        { "glitch_slice", "float", 20, 1000, 125 },
                        { "reverb_mix", "float", 0, 1, 0.25f },
                        { "reverb_decay", "float", 0.2f, 20, 2.5f },
                        { "output", "float", -24, 12, 0 },
                        { "bypass", "bool", 0, 1, 0 } };

    REQUIRE (p.getParameters().size() == 12);

    for (const auto& s : spec)
    {
        INFO ("parâmetro \"" << s.id << "\"");
        auto* param = find (p, s.id);
        REQUIRE (param != nullptr);

        const juce::String type (s.type);
        if (type == "bool")  REQUIRE (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr);
        if (type == "int")   REQUIRE (dynamic_cast<juce::AudioParameterInt*> (param) != nullptr);
        if (type == "float") REQUIRE (dynamic_cast<juce::AudioParameterFloat*> (param) != nullptr);

        REQUIRE_THAT (param->getNormalisableRange().start, WithinAbs (s.min, 1.0e-3));
        REQUIRE_THAT (param->getNormalisableRange().end, WithinAbs (s.max, 1.0e-3));
        REQUIRE_THAT (param->convertFrom0to1 (param->getDefaultValue()), WithinAbs (s.def, 0.02 * (s.max - s.min)));
    }

    REQUIRE (p.getBypassParameter() == find (p, "bypass"));
}

TEST_CASE ("Estado: o que um plugin grava, outro repõe", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor a, b;
    REQUIRE (a.getParameters().size() == 12);

    juce::Random random (1);
    for (auto* param : a.getParameters())
        param->setValueNotifyingHost (random.nextFloat());

    juce::MemoryBlock state;
    a.getStateInformation (state);
    b.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    for (int i = 0; i < a.getParameters().size(); ++i)
    {
        auto* pa = dynamic_cast<juce::RangedAudioParameter*> (a.getParameters()[i]);
        auto* pb = dynamic_cast<juce::RangedAudioParameter*> (b.getParameters()[i]);
        INFO ("parâmetro " << pa->getParameterID());
        REQUIRE_THAT (pb->convertFrom0to1 (pb->getValue()), WithinAbs (pa->convertFrom0to1 (pa->getValue()), 1.0e-3));
    }

    // Lixo não pode crashar
    const char garbage[] = "isto não é um estado";
    b.setStateInformation (garbage, sizeof (garbage));
}

TEST_CASE ("Tudo desligado: um impulso sai na posição da latência reportada, que não muda com o fuzz", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor p;
    REQUIRE (p.getParameters().size() == 12);
    allOff (p);
    p.prepareToPlay (fs, 512);

    const auto latency = p.getLatencySamples();
    INFO ("latência reportada: " << latency);
    REQUIRE (latency >= 0);
    REQUIRE (latency < 4800);

    auto buffer = makeImpulse (2, 9600, 100);
    processAll (p, buffer);

    REQUIRE (std::abs (peakIndex (buffer, 0) - (100 + latency)) <= 1);
    REQUIRE (std::abs (buffer.getSample (0, peakIndex (buffer, 0))) > 0.7f);

    set (p, "fuzz_on", 1.0f);
    REQUIRE (p.getLatencySamples() == latency);
}

TEST_CASE ("Fuzz: harmónicos quando ligado, e aliasing baixo (oversampling)", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;

    auto render = [] (bool fuzz, double frequency, float drive)
    {
        PedalProcessor p;
        REQUIRE (p.getParameters().size() == 12);
        allOff (p);
        set (p, "fuzz_on", fuzz ? 1.0f : 0.0f);
        set (p, "fuzz_drive", drive);
        set (p, "fuzz_tone", 12000.0f);
        p.prepareToPlay (fs, 512);
        auto buffer = makeSine (2, 48000, frequency, fs, 0.5f);
        processAll (p, buffer);
        return buffer;
    };

    auto dbc = [] (const juce::AudioBuffer<float>& b, double f0, double f)
    {
        return toDb (amplitudeAt (b, 0, f, fs, 9600, 32768) / amplitudeAt (b, 0, f0, fs, 9600, 32768));
    };

    const auto clean = render (false, 220.0, 30.0f);
    const auto fuzzed = render (true, 220.0, 30.0f);
    INFO ("3.º harmónico: desligado " << dbc (clean, 220, 660) << " dBc, ligado " << dbc (fuzzed, 220, 660) << " dBc");
    REQUIRE (dbc (clean, 220, 660) < -60.0);
    REQUIRE (dbc (fuzzed, 220, 660) > -30.0);

    // 5 kHz com muito drive: o 7.º harmónico (35 kHz) dobraria para 48 − 35 = 13 kHz
    const auto high = render (true, 5000.0, 30.0f);
    INFO ("alias a 13 kHz: " << dbc (high, 5000, 13000) << " dBc");
    REQUIRE (dbc (high, 5000, 13000) < -40.0);
}

TEST_CASE ("Crush: poucos níveis com poucos bits; degraus com downsample", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor p;
    REQUIRE (p.getParameters().size() == 12);
    allOff (p);
    set (p, "crush_on", 1.0f);
    set (p, "crush_bits", 4.0f);
    set (p, "crush_downsample", 1.0f);
    p.prepareToPlay (fs, 512);

    auto buffer = makeSine (1, 24000, 50.0, fs, 0.9f);
    juce::AudioBuffer<float> stereo (2, 24000);
    stereo.copyFrom (0, 0, buffer, 0, 0, 24000);
    stereo.copyFrom (1, 0, buffer, 0, 0, 24000);
    processAll (p, stereo);

    std::set<float> levels;
    for (int n = 4800; n < 24000; ++n)
        levels.insert (stereo.getSample (0, n));

    INFO ("níveis diferentes com 4 bits: " << levels.size());
    REQUIRE (levels.size() >= 4);
    REQUIRE (levels.size() <= 17);

    // downsample 8: a saída só muda de 8 em 8 amostras
    set (p, "crush_bits", 16.0f);
    set (p, "crush_downsample", 8.0f);
    auto noise = makeNoise (2, 24000, 0.5f);
    processAll (p, noise);
    int changes = 0;
    for (int n = 4801; n < 24000; ++n)
        changes += noise.getSample (0, n) != noise.getSample (0, n - 1) ? 1 : 0;
    INFO ("mudanças em 19199 amostras: " << changes);
    REQUIRE (changes <= 19199 / 8 + 2);
    REQUIRE (changes >= 19199 / 8 - 2);
}

TEST_CASE ("Glitch: ligado, a saída fica periódica com período = fatia", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor p;
    REQUIRE (p.getParameters().size() == 12);
    allOff (p);
    set (p, "glitch_slice", 100.0f);
    p.prepareToPlay (fs, 480);

    auto buffer = makeNoise (2, 72000, 0.5f);
    processRange (p, buffer, 0, 24000);
    set (p, "glitch_on", 1.0f);
    for (int start = 24000; start < 72000; start += 480)
        processRange (p, buffer, start, 480);

    float diff = 0.0f;
    for (int n = 24000 + 1000; n + 4800 < 72000; ++n)
        diff = juce::jmax (diff, std::abs (buffer.getSample (0, n + 4800) - buffer.getSample (0, n)));
    REQUIRE (diff < 1.0e-4f);
}

TEST_CASE ("Reverb: cauda depois de um impulso, mais longa com mais decay", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;

    auto tailEnergy = [] (float decay)
    {
        PedalProcessor p;
        REQUIRE (p.getParameters().size() == 12);
        allOff (p);
        set (p, "reverb_mix", 0.5f);
        set (p, "reverb_decay", decay);
        p.prepareToPlay (fs, 512);
        auto buffer = makeImpulse (2, 96000, 0);
        processAll (p, buffer);
        return rms (buffer, 0, 48000, 24000); // de 1.0 a 1.5 s
    };

    const auto shortTail = tailEnergy (0.5f);
    const auto longTail = tailEnergy (4.0f);
    INFO ("energia entre 1.0 e 1.5 s: decay 0.5 s → " << toDb (shortTail) << " dB, decay 4 s → " << toDb (longTail) << " dB");
    REQUIRE (longTail > 1.0e-4);
    REQUIRE (toDb (longTail) - toDb (shortTail) > 20.0);
}

TEST_CASE ("Bypass: depois do crossfade, a entrada atrasada pela latência (sem cliques)", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor p;
    REQUIRE (p.getParameters().size() == 12);
    set (p, "fuzz_on", 1.0f);
    set (p, "fuzz_drive", 30.0f);
    p.prepareToPlay (fs, 480);
    const auto latency = p.getLatencySamples();

    const auto input = makeSine (2, 48000, 220.0, fs, 0.5f);
    auto buffer = copyOf (input);
    processRange (p, buffer, 0, 24000);
    set (p, "bypass", 1.0f);
    for (int start = 24000; start < 48000; start += 480)
        processRange (p, buffer, start, 480);

    float diff = 0.0f;
    for (int n = 24000 + 4800; n < 48000; ++n)
        diff = juce::jmax (diff, std::abs (buffer.getSample (0, n) - input.getSample (0, n - latency)));
    REQUIRE (diff < 1.0e-5f);

    // A transição não salta (o maior passo de um seno de 220 Hz com amplitude 0.5 é ~0.0144;
    // o fuzz, saturado, tem passos maiores: aceitamos até 0.3).
    REQUIRE (maxSampleJump (buffer, 0) < 0.3f);
}

TEST_CASE ("Mini-pluginval: layouts, sample rates, blocos irregulares, automação, sem alocar", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    using Set = juce::AudioChannelSet;

    {
        PedalProcessor p;
        juce::AudioProcessor::BusesLayout stereo, surround;
        stereo.inputBuses.add (Set::stereo());
        stereo.outputBuses.add (Set::stereo());
        surround.inputBuses.add (Set::create5point1());
        surround.outputBuses.add (Set::create5point1());
        REQUIRE (p.checkBusesLayoutSupported (stereo));
        REQUIRE_FALSE (p.checkBusesLayoutSupported (surround));
    }

    for (const auto sampleRate : { 44100.0, 96000.0 })
    {
        for (const auto blockSize : { 64, 512 })
        {
            INFO (sampleRate << " Hz, bloco " << blockSize);
            PedalProcessor p;
            REQUIRE (p.getParameters().size() == 12);
            p.prepareToPlay (sampleRate, blockSize);

            auto buffer = makeNoise (2, static_cast<int> (sampleRate / 2), 0.5f);
            juce::Random random (static_cast<juce::int64> (sampleRate) + blockSize);
            std::vector<int> sizes;
            for (int i = 0; i < 64; ++i)
                sizes.push_back (1 + random.nextInt (2 * blockSize));

            int position = 0, allocations = 0;
            size_t i = 0;
            while (position < buffer.getNumSamples())
            {
                for (auto* param : p.getParameters())
                    if (random.nextInt (5) == 0)
                        param->setValueNotifyingHost (random.nextFloat());

                const auto num = juce::jmin (sizes[i++ % sizes.size()], buffer.getNumSamples() - position);
                allocations += countAllocations ([&] { processRange (p, buffer, position, num); });
                position += num;
            }

            REQUIRE (allocations == 0);
            REQUIRE (allFinite (buffer));
            REQUIRE (peak (buffer, 0) < 8.0f);
        }
    }
}

TEST_CASE ("Editor: existe, abre e fecha", "[40]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    PedalProcessor p;

    REQUIRE (p.hasEditor());
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditorAndMakeActive());
    REQUIRE (editor != nullptr);
    REQUIRE (editor->getWidth() > 100);
    REQUIRE (editor->getHeight() > 100);
    p.editorBeingDeleted (editor.get());
    editor.reset();

    set (p, "reverb_mix", 0.5f);
    REQUIRE (p.getTailLengthSeconds() > 0.0);
}
