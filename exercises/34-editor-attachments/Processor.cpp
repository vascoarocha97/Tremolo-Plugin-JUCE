#include "Processor.h"

EchoEditor::EchoEditor (EchoProcessor& p)
    : AudioProcessorEditor (p), echo (p)
{
    titleLabel.setText ("Eco", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    const char* ids[] { EchoParams::gain, EchoParams::time, EchoParams::feedback, EchoParams::tone, EchoParams::mix };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.parameterId = ids[i];

        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        addAndMakeVisible (knob.slider);

        if (auto* param = echo.apvts.getParameter (knob.parameterId))
            knob.label.setText (param->getName (32), juce::dontSendNotification);

        knob.label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (knob.label);

        // TODO(1): liga knob.slider ao parâmetro knob.parameterId com um SliderAttachment
        //   (guarda-o em knob.attachment). O attachment copia a gama, o valor e o valor por omissão.
    }

    addAndMakeVisible (modeBox);
    addAndMakeVisible (bypassButton);

    // TODO(2): - enche a modeBox com as escolhas do parâmetro "mode" (AudioParameterChoice::choices,
    //            ids a partir de 1) e liga-a com um ComboBoxAttachment (a ordem importa: ver README);
    //          - liga o bypassButton com um ButtonAttachment.

    setSize (380, 250); // chama resized(): por isso fica no fim, quando os componentes já existem
}

EchoEditor::~EchoEditor() = default;

juce::Slider& EchoEditor::getSlider (const juce::String& parameterId)
{
    for (auto& knob : knobs)
        if (knob.parameterId == parameterId)
            return knob.slider;

    jassertfalse;
    return knobs[0].slider;
}

void EchoEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void EchoEditor::resized()
{
    // TODO(3): distribui os componentes pela janela (ver o esboço no README), com
    //   getLocalBounds() e Rectangle::removeFromTop / removeFromBottom / removeFromLeft / reduced:
    //   título em cima; modeBox e bypassButton numa linha em baixo; no meio, 5 colunas iguais, cada uma
    //   com knob.label em cima e knob.slider por baixo. Nada pode ficar fora da janela nem sobreposto.
}
