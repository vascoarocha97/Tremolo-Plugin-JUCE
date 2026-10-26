#include "HostAudioEngine.h"

#include <cmath>

namespace learn
{

HostAudioEngine::HostAudioEngine (juce::AudioProcessor& processorToUse)
    : processor (processorToUse)
{
}

void HostAudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const auto sampleRate = device->getCurrentSampleRate();
    const auto blockSize = device->getCurrentBufferSizeSamples();

    // Tudo o que aloca memória acontece aqui, antes de o áudio arrancar.
    maxBlockSize = blockSize;
    workBuffer.setSize (2, blockSize);
    dryBuffer.setSize (2, blockSize);
    monoScratch.assign (static_cast<size_t> (blockSize), 0.0f);
    midiBuffer.ensureSize (256);

    source.prepare (sampleRate, blockSize);

    smoothedOutputGain.reset (sampleRate, 0.05);
    smoothedOutputGain.setCurrentAndTargetValue (outputGain.load());
    smoothedWet.reset (sampleRate, 0.02);
    smoothedWet.setCurrentAndTargetValue (bypassed.load() ? 0.0f : 1.0f);

    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    mutedByNaN.store (false);
    currentSampleRate.store (sampleRate);
    currentBlockSize.store (blockSize);
    isPrepared = true;
}

void HostAudioEngine::audioDeviceStopped()
{
    if (isPrepared)
        processor.releaseResources();

    isPrepared = false;
    currentSampleRate.store (0.0);
    currentBlockSize.store (0);
}

void HostAudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                        int numInputChannels,
                                                        float* const* outputChannelData,
                                                        int numOutputChannels, int numSamples,
                                                        const juce::AudioIODeviceCallbackContext&)
{
    // Evita a armadilha dos números denormais (ver módulo 9).
    const juce::ScopedNoDenormals noDenormals;

    // Alguns drivers chamam com mais amostras do que o tamanho anunciado: processa às fatias.
    int offset = 0;

    while (offset < numSamples)
    {
        const auto chunk = juce::jmin (numSamples - offset, juce::jmax (1, maxBlockSize));
        processChunk (inputChannelData, numInputChannels, outputChannelData, numOutputChannels,
                      offset, chunk);
        offset += chunk;
    }
}

void HostAudioEngine::processChunk (const float* const* inputs, int numInputs,
                                    float* const* outputs, int numOutputs, int offset,
                                    int numSamples) noexcept
{
    if (! isPrepared || numSamples > workBuffer.getNumSamples())
    {
        for (int ch = 0; ch < numOutputs; ++ch)
            if (outputs[ch] != nullptr)
                juce::FloatVectorOperations::clear (outputs[ch] + offset, numSamples);

        return;
    }

    // Vista sobre os primeiros numSamples do buffer de trabalho (não aloca).
    juce::AudioBuffer<float> buffer (workBuffer.getArrayOfWritePointers(), 2, 0, numSamples);
    juce::AudioBuffer<float> dry (dryBuffer.getArrayOfWritePointers(), 2, 0, numSamples);

    // 1. Fonte de teste
    source.render (buffer, inputs, numInputs, offset);

    for (int ch = 0; ch < 2; ++ch)
        dry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // 2. O processador do exercício (sempre a correr, para o bypass não "acordar" estado velho)
    processor.processBlock (buffer, midiBuffer);
    midiBuffer.clear();

    // 3. Proteção: NaN/inf => silêncio até reiniciar o áudio
    bool finite = true;

    for (int ch = 0; ch < 2 && finite; ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);

        for (int n = 0; n < numSamples; ++n)
        {
            if (! std::isfinite (data[n]))
            {
                finite = false;
                break;
            }
        }
    }

    if (! finite)
        mutedByNaN.store (true);

    if (mutedByNaN.load())
        buffer.clear();

    // 4. Bypass com crossfade (20 ms) entre seco e processado
    smoothedWet.setTargetValue (bypassed.load() ? 0.0f : 1.0f);

    for (int n = 0; n < numSamples; ++n)
    {
        const auto wet = smoothedWet.getNextValue();

        for (int ch = 0; ch < 2; ++ch)
            buffer.setSample (ch, n, wet * buffer.getSample (ch, n) + (1.0f - wet) * dry.getSample (ch, n));
    }

    // 5. Análise (osciloscópio, espectro): entrada vs. saída do processador, antes do ganho de saída
    auto* mono = monoScratch.data();

    for (int n = 0; n < numSamples; ++n)
        mono[n] = 0.5f * (dry.getSample (0, n) + dry.getSample (1, n));

    inputFifo.push (mono, numSamples);

    for (int n = 0; n < numSamples; ++n)
        mono[n] = 0.5f * (buffer.getSample (0, n) + buffer.getSample (1, n));

    outputFifo.push (mono, numSamples);

    // 6. Ganho de saída + limitador de proteção: nunca sai nada acima de 0 dBFS
    smoothedOutputGain.setTargetValue (outputGain.load());
    int clipped = 0;

    for (int n = 0; n < numSamples; ++n)
    {
        const auto gain = smoothedOutputGain.getNextValue();

        for (int ch = 0; ch < 2; ++ch)
        {
            auto sample = buffer.getSample (ch, n) * gain;

            if (std::abs (sample) > 1.0f)
            {
                sample = sample > 0.0f ? 1.0f : -1.0f;
                ++clipped;
            }

            buffer.setSample (ch, n, sample);
        }
    }

    if (clipped > 0)
        clipCount.fetch_add (clipped);

    // 7. Saída para o dispositivo
    for (int ch = 0; ch < numOutputs; ++ch)
        if (outputs[ch] != nullptr)
            juce::FloatVectorOperations::copy (outputs[ch] + offset,
                                               buffer.getReadPointer (juce::jmin (ch, 1)),
                                               numSamples);

    // 8. Medidores (pico da saída final)
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto magnitude = buffer.getMagnitude (ch, 0, numSamples);
        auto& peakStore = outputPeaks[(size_t) ch];
        auto previous = peakStore.load();

        while (magnitude > previous && ! peakStore.compare_exchange_weak (previous, magnitude))
        {
        }
    }
}

} // namespace learn
