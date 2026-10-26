#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

namespace
{
    void set (EchoProcessor& p, const char* id, float realValue)
    {
        auto* param = p.apvts.getParameter (id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    }

    float get (EchoProcessor& p, const char* id)
    {
        return p.apvts.getRawParameterValue (id)->load();
    }

    /** Valores 0..1 de todos os parâmetros, "arredondados" ao passo de cada um (um bool com 0.31
        vale false, e é isso que fica guardado). */
    std::vector<float> normalisedValues (EchoProcessor& p)
    {
        std::vector<float> v;
        for (auto* param : p.getParameters())
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param);
            REQUIRE (ranged != nullptr);
            v.push_back (ranged->convertTo0to1 (ranged->convertFrom0to1 (ranged->getValue())));
        }
        return v;
    }

    void randomise (EchoProcessor& p, juce::int64 seed)
    {
        juce::Random random (seed);
        for (auto* param : p.getParameters())
            param->setValueNotifyingHost (random.nextFloat());
    }
}

TEST_CASE ("Guardar e repor: todos os parâmetros voltam ao valor guardado", "[33]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);

    randomise (p, 1);
    const auto saved = normalisedValues (p);

    juce::MemoryBlock state;
    EchoState::save (p.apvts, state);
    REQUIRE (state.getSize() > 0);

    randomise (p, 2);
    REQUIRE (normalisedValues (p) != saved);

    REQUIRE (EchoState::load (p.apvts, state.getData(), static_cast<int> (state.getSize())));
    const auto restored = normalisedValues (p);

    for (size_t i = 0; i < saved.size(); ++i)
    {
        INFO ("parâmetro " << p.getParameters()[(int) i]->getName (32));
        REQUIRE_THAT (restored[i], WithinAbs (saved[i], 1.0e-5));
    }
}

TEST_CASE ("O estado é XML legível, com a tag PARAMETERS e um filho por parâmetro", "[33]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    set (p, EchoParams::time, 123.0f);

    juce::MemoryBlock state;
    EchoState::save (p.apvts, state);

    const auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    REQUIRE (xml->hasTagName ("PARAMETERS"));
    REQUIRE (xml->getNumChildElements() == 7);

    auto* time = xml->getChildByAttribute ("id", EchoParams::time);
    REQUIRE (time != nullptr);
    REQUIRE_THAT (time->getDoubleAttribute ("value"), WithinAbs (123.0, 0.1)); // valor real, legível
}

TEST_CASE ("Dados inválidos são rejeitados sem mexer nos parâmetros", "[33]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    randomise (p, 3);
    const auto before = normalisedValues (p);

    REQUIRE_FALSE (EchoState::load (p.apvts, nullptr, 0));

    juce::MemoryBlock garbage;
    juce::Random random (4);
    for (int i = 0; i < 200; ++i)
        garbage.append (&i, 1);
    REQUIRE_FALSE (EchoState::load (p.apvts, garbage.getData(), static_cast<int> (garbage.getSize())));

    // XML válido, mas não é o nosso
    juce::MemoryBlock other;
    juce::AudioProcessor::copyXmlToBinary (juce::XmlElement ("SOMETHING_ELSE"), other);
    REQUIRE_FALSE (EchoState::load (p.apvts, other.getData(), static_cast<int> (other.getSize())));

    REQUIRE (normalisedValues (p) == before);
}

TEST_CASE ("Estado de uma versão antiga sem um parâmetro: esse parâmetro volta ao valor por omissão", "[33]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    set (p, EchoParams::time, 500.0f);

    juce::MemoryBlock state;
    EchoState::save (p.apvts, state);

    // "Versão antiga": tira o parâmetro tone do estado
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (xml != nullptr);
    auto* tone = xml->getChildByAttribute ("id", EchoParams::tone);
    REQUIRE (tone != nullptr);
    xml->removeChildElement (tone, true);
    juce::MemoryBlock oldState;
    juce::AudioProcessor::copyXmlToBinary (*xml, oldState);

    set (p, EchoParams::tone, 15000.0f);
    set (p, EchoParams::time, 50.0f);
    REQUIRE (EchoState::load (p.apvts, oldState.getData(), static_cast<int> (oldState.getSize())));

    REQUIRE_THAT (get (p, EchoParams::time), WithinAbs (500.0, 0.5));
    REQUIRE_THAT (get (p, EchoParams::tone), WithinAbs (6000.0, 5.0)); // omissão, não 15000
}

TEST_CASE ("Presets: os valores do preset, e os restantes parâmetros no valor por omissão", "[33]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    REQUIRE (factoryPresets().size() >= 3);

    randomise (p, 5);
    const Preset preset { "Teste", { { EchoParams::time, 200.0f }, { EchoParams::mode, 1.0f } } };
    applyPreset (p.apvts, preset);

    REQUIRE_THAT (get (p, EchoParams::time), WithinAbs (200.0, 0.5));
    REQUIRE (get (p, EchoParams::mode) == 1.0f);
    REQUIRE_THAT (get (p, EchoParams::feedback), WithinAbs (0.4, 1.0e-3));
    REQUIRE_THAT (get (p, EchoParams::gain), WithinAbs (0.0, 1.0e-3));
    REQUIRE (get (p, EchoParams::bypass) == 0.0f);

    for (const auto& factory : factoryPresets())
    {
        INFO ("preset " << factory.name);
        applyPreset (p.apvts, factory);
        for (const auto& [id, value] : factory.values)
            REQUIRE_THAT (get (p, id.toRawUTF8()), WithinAbs (value, 0.5));
    }
}

TEST_CASE ("StatefulEchoProcessor: o host guarda num plugin e repõe noutro; programas = presets", "[33]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    StatefulEchoProcessor a, b;
    REQUIRE (a.getParameters().size() == 7);

    randomise (a, 6);
    juce::MemoryBlock state;
    a.getStateInformation (state);
    b.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    const auto va = normalisedValues (a), vb = normalisedValues (b);
    for (size_t i = 0; i < va.size(); ++i)
        REQUIRE_THAT (vb[i], WithinAbs (va[i], 1.0e-5));

    REQUIRE (b.getNumPrograms() == static_cast<int> (factoryPresets().size()));
    b.setCurrentProgram (1);
    REQUIRE (b.getCurrentProgram() == 1);
    REQUIRE (b.getProgramName (1) == factoryPresets()[1].name);
    for (const auto& [id, value] : factoryPresets()[1].values)
        REQUIRE_THAT (get (b, id.toRawUTF8()), WithinAbs (value, 0.5));
}
