#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

namespace
{
    const char* const sliderIds[] { EchoParams::gain, EchoParams::time, EchoParams::feedback, EchoParams::tone, EchoParams::mix };

    float get (EchoProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load(); }

    void set (EchoProcessor& p, const char* id, float realValue)
    {
        auto* param = p.apvts.getParameter (id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    }
}

// Os testes correm na message thread (ScopedJuceInitialiser_GUI): os attachments atualizam os
// componentes de forma síncrona, por isso podemos verificar logo a seguir.

TEST_CASE ("Cada slider fica com a gama e o valor do seu parâmetro", "[34]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    EchoEditor editor (p);

    for (const auto* id : sliderIds)
    {
        INFO ("slider \"" << id << "\"");
        auto& slider = editor.getSlider (id);
        const auto& range = p.apvts.getParameter (id)->getNormalisableRange();
        REQUIRE_THAT (slider.getMinimum(), WithinAbs (range.start, 1.0e-4));
        REQUIRE_THAT (slider.getMaximum(), WithinAbs (range.end, 1.0e-4));
        REQUIRE_THAT (slider.getValue(), WithinAbs (get (p, id), 1.0e-3));
    }

    // e o skew do tempo passou para o slider: o meio do curso é 250 ms
    REQUIRE_THAT (editor.getSlider (EchoParams::time).proportionOfLengthToValue (0.5), WithinAbs (250.0, 1.0));
}

TEST_CASE ("Mexer num slider muda o parâmetro", "[34]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    EchoEditor editor (p);

    editor.getSlider (EchoParams::gain).setValue (-6.0, juce::sendNotificationSync);
    editor.getSlider (EchoParams::time).setValue (123.0, juce::sendNotificationSync);
    editor.getSlider (EchoParams::mix).setValue (0.8, juce::sendNotificationSync);

    REQUIRE_THAT (get (p, EchoParams::gain), WithinAbs (-6.0, 0.05));
    REQUIRE_THAT (get (p, EchoParams::time), WithinAbs (123.0, 0.1));
    REQUIRE_THAT (get (p, EchoParams::mix), WithinAbs (0.8, 0.01));
}

TEST_CASE ("O host muda o parâmetro (automação) e o slider acompanha", "[34]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    EchoEditor editor (p);

    set (p, EchoParams::feedback, 0.77f);
    set (p, EchoParams::tone, 2000.0f);

    REQUIRE_THAT (editor.getSlider (EchoParams::feedback).getValue(), WithinAbs (0.77, 0.01));
    REQUIRE_THAT (editor.getSlider (EchoParams::tone).getValue(), WithinAbs (2000.0, 2.0));
}

TEST_CASE ("ComboBox do modo e botão de bypass ligados nos dois sentidos", "[34]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    REQUIRE (p.getParameters().size() == 7);
    EchoEditor editor (p);

    auto& modeBox = editor.getModeBox();
    REQUIRE (modeBox.getNumItems() == 2);
    REQUIRE (modeBox.getItemText (1) == "Ping-pong");

    modeBox.setSelectedItemIndex (1, juce::sendNotificationSync);
    REQUIRE (get (p, EchoParams::mode) == 1.0f);

    set (p, EchoParams::mode, 0.0f);
    REQUIRE (modeBox.getSelectedItemIndex() == 0);

    editor.getBypassButton().setToggleState (true, juce::sendNotificationSync);
    REQUIRE (get (p, EchoParams::bypass) == 1.0f);

    set (p, EchoParams::bypass, 0.0f);
    REQUIRE_FALSE (editor.getBypassButton().getToggleState());
}

TEST_CASE ("Layout: tudo visível, dentro da janela e sem sobreposições", "[34]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EchoProcessor p;
    EchoEditor editor (p);

    REQUIRE (editor.getWidth() >= 200);
    REQUIRE (editor.getHeight() >= 150);

    std::vector<juce::Component*> components { &editor.getModeBox(), &editor.getBypassButton() };
    for (const auto* id : sliderIds)
        components.push_back (&editor.getSlider (id));

    for (auto* c : components)
    {
        INFO ("componente " << c->getName() << " " << c->getBounds().toString());
        REQUIRE (c->isVisible());
        REQUIRE (c->getParentComponent() == &editor);
        REQUIRE (c->getWidth() >= 20);
        REQUIRE (c->getHeight() >= 18);
        REQUIRE (editor.getLocalBounds().contains (c->getBounds()));
    }

    for (size_t i = 0; i < components.size(); ++i)
        for (size_t j = i + 1; j < components.size(); ++j)
            REQUIRE_FALSE (components[i]->getBounds().intersects (components[j]->getBounds()));

    // Redimensionar volta a distribuir
    editor.setSize (600, 300);
    REQUIRE (editor.getSlider (EchoParams::mix).getRight() > 450);
}

TEST_CASE ("O host pode abrir e fechar o editor várias vezes", "[34]")
{
    const juce::ScopedJuceInitialiser_GUI gui;
    EditorEchoProcessor p;
    REQUIRE (p.hasEditor());

    for (int i = 0; i < 3; ++i)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditorAndMakeActive());
        REQUIRE (editor != nullptr);
        REQUIRE (p.getActiveEditor() == editor.get());
        set (p, EchoParams::time, 100.0f + 50.0f * static_cast<float> (i)); // attachments vivos

        // O que um host faz ao fechar a janela do plugin: avisa o processador e depois apaga
        p.editorBeingDeleted (editor.get());
        editor.reset();
        REQUIRE (p.getActiveEditor() == nullptr);
    }

    set (p, EchoParams::time, 400.0f); // sem editor: nenhum attachment pendurado
    REQUIRE_THAT (get (p, EchoParams::time), WithinAbs (400.0, 0.1));
}
