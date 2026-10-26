#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 40 — O pedal.

    Escrito do zero: o README tem a especificação (parâmetros, ordem dos efeitos, comportamento) e
    os testes verificam-na de fora, como um host faria. Este ficheiro é só um ponto de partida que
    compila: deixa passar o som tal e qual.

    Podes (deves!) reaproveitar o teu código dos exercícios anteriores — copia-o para esta pasta.
    Qualquer .h/.cpp que criares aqui (fora de tests/) entra no build automaticamente.

    A única regra fixa: uma classe PedalProcessor, derivada de juce::AudioProcessor, com construtor
    sem argumentos, declarada neste Processor.h. */
class PedalProcessor : public juce::AudioProcessor
{
public:
    PedalProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    const juce::String getName() const override { return "Learn JUCE Pedal"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    bool hasEditor() const override { return false; }

private:
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalProcessor)
};
