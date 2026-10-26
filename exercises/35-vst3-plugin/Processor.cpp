#include "Processor.h"

#include <cmath>

bool EchoPlugin::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // TODO(1): aceita mono → mono e estéreo → estéreo; recusa tudo o resto.
    //   layouts.getMainInputChannelSet(), getMainOutputChannelSet(), juce::AudioChannelSet::mono()/stereo().
    juce::ignoreUnused (layouts);
    return false;
}

double EchoPlugin::getTailLengthSeconds() const
{
    // TODO(2): quanto tempo dura o som depois de a entrada parar? O eco k tem amplitude fb^k: quando
    //   é que fica abaixo de −60 dB? Usa os atomics `time` (ms) e `feedback`. Cuidado com fb = 0.
    return 0.0;
}

juce::AudioProcessorParameter* EchoPlugin::getBypassParameter() const
{
    // TODO(3): devolve o nosso parâmetro de bypass (o host passa a usá-lo no seu botão de bypass).
    return nullptr;
}

bool EchoPlugin::hasEditor() const
{
    // TODO(5): o plugin tem editor: o teu EchoEditor do exercício 34.
    return false;
}

juce::AudioProcessorEditor* EchoPlugin::createEditor()
{
    // TODO(5): cria e devolve um EchoEditor (o host fica com a posse).
    return nullptr;
}

void EchoPlugin::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    StatefulEchoProcessor::prepareToPlay (sampleRate, samplesPerBlock);

    dryBuffer.setSize (2, samplesPerBlock, false, false, true);
    bypassAmount.reset (sampleRate, 0.02);
    bypassAmount.setCurrentAndTargetValue (bypass != nullptr && bypass->get() ? 1.0f : 0.0f);
}

void EchoPlugin::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    bypassAmount.setTargetValue (bypass != nullptr && bypass->get() ? 1.0f : 0.0f);

    // Alguns hosts mandam blocos maiores do que o prometido no prepareToPlay: processa aos pedaços,
    // com "vistas" sobre o buffer (não copiam nem alocam).
    const auto maxChunk = juce::jmax (1, dryBuffer.getNumSamples());

    for (int start = 0; start < buffer.getNumSamples(); start += maxChunk)
    {
        const auto num = juce::jmin (maxChunk, buffer.getNumSamples() - start);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, num);
        processChunk (chunk);
    }
}

void EchoPlugin::processChunk (juce::AudioBuffer<float>& chunk) noexcept
{
    const auto numChannels = juce::jmin (chunk.getNumChannels(), dryBuffer.getNumChannels());
    const auto numSamples = chunk.getNumSamples();

    // TODO(4): bypass sem cliques (ver README):
    //   1. copia a entrada (numChannels canais, numSamples amostras) para dryBuffer;
    //   2. processEcho (chunk)  — o eco corre SEMPRE, mesmo em bypass (porquê? README);
    //   3. por amostra: amount = bypassAmount.getNextValue() (0 = eco, 1 = bypass) e, em cada canal,
    //      saída = (1 − amount) · eco + amount · seco.
    juce::ignoreUnused (numChannels, numSamples);
    processEcho (chunk);
}
