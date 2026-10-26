#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace learn
{

/** Base dos wrappers dos exercícios: um juce::AudioProcessor real, sem editor próprio.

    Cada exercício cria uma subclasse em app/ExerciseApp.cpp que:
      - cria os parâmetros (juce::AudioParameterFloat, ...) no construtor com addParameter();
      - em prepareToPlay()/processBlock() chama o código DSP do exercício;
      - devolve o título do exercício em getName().

    O host (shared/host) mostra os parâmetros num juce::GenericAudioProcessorEditor.
    Não precisas de editar estes wrappers para fazer os exercícios. */
class ExerciseAudioProcessor : public juce::AudioProcessor
{
public:
    ExerciseAudioProcessor()
        : AudioProcessor (BusesProperties()
                              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const auto& out = layouts.getMainOutputChannelSet();

        if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
            return false;

        return layouts.getMainInputChannelSet() == out;
    }

    void releaseResources() override {}

    //==============================================================================
    bool acceptsMidi() const override                     { return false; }
    bool producesMidi() const override                    { return false; }
    double getTailLengthSeconds() const override          { return 0.0; }

    int getNumPrograms() override                         { return 1; }
    int getCurrentProgram() override                      { return 0; }
    void setCurrentProgram (int) override                 {}
    const juce::String getProgramName (int) override      { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override   {}

    bool hasEditor() const override                       { return false; }

    using AudioProcessor::processBlock;

private:
    // No JUCE 9 o createEditor() é privado: quem quer um editor chama createEditorAndMakeActive().
    juce::AudioProcessorEditor* createEditor() override   { return nullptr; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExerciseAudioProcessor)
};

} // namespace learn
