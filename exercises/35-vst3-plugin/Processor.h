#pragma once

#include "../33-state-and-presets/Processor.h"  // o TEU estado e presets (33), que inclui o 32
#include "../34-editor-attachments/Processor.h" // o TEU editor (34)

/** Exercício 35 — O plugin VST3.

    Junta as peças dos exercícios 32–34 num plugin que um DAW pode carregar, e trata das coisas que
    só importam a um host: que layouts de canais aceitas, quanto tempo dura a cauda depois de o som
    parar, qual é o parâmetro de bypass (para o botão de bypass do próprio DAW) e um bypass sem
    cliques. O mesmo código compila como VST3 e como Standalone (CMakeLists.txt). */
class EchoPlugin : public StatefulEchoProcessor
{
public:
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    double getTailLengthSeconds() const override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    bool hasEditor() const override;

private:
    juce::AudioProcessorEditor* createEditor() override;

    /** Processa um pedaço com no máximo dryBuffer.getNumSamples() amostras. */
    void processChunk (juce::AudioBuffer<float>& chunk) noexcept;

    juce::AudioBuffer<float> dryBuffer;       // cópia da entrada para o crossfade do bypass
    juce::SmoothedValue<float> bypassAmount;  // 0 = eco, 1 = bypass
};
