#pragma once

#include "../32-apvts-parameters/Processor.h" // o TEU EchoProcessor do exercício 32

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

/** Exercício 34 — Editor e attachments.

    O editor (a GUI do plugin) vive na message thread; o áudio noutra. Nunca falam diretamente:
    falam através dos PARÂMETROS. Um "attachment" liga um componente (Slider, ComboBox, Button) a um
    parâmetro do APVTS nos dois sentidos:
      - o utilizador mexe no slider  -> o parâmetro muda (e o host grava a automação);
      - o host muda o parâmetro       -> o slider mexe-se. */
class EchoEditor : public juce::AudioProcessorEditor
{
public:
    explicit EchoEditor (EchoProcessor&);
    ~EchoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Para os testes
    juce::Slider& getSlider (const juce::String& parameterId);
    juce::ComboBox& getModeBox() noexcept { return modeBox; }
    juce::ToggleButton& getBypassButton() noexcept { return bypassButton; }

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    /** Um botão rotativo com etiqueta. O attachment é declarado DEPOIS do slider: os membros são
        destruídos pela ordem inversa, por isso o attachment desaparece antes do slider. */
    struct Knob
    {
        juce::String parameterId;
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    EchoProcessor& echo;
    std::array<Knob, 5> knobs;

    juce::Label titleLabel;
    juce::ComboBox modeBox;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    juce::ToggleButton bypassButton { "Bypass" };
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoEditor)
};

/** O EchoProcessor do 32, agora com o teu editor. Já está feito. */
class EditorEchoProcessor : public EchoProcessor
{
public:
    bool hasEditor() const override { return true; }

private:
    juce::AudioProcessorEditor* createEditor() override { return new EchoEditor (*this); }
};
