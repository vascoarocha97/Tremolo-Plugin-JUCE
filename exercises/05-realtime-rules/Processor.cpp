#include "Processor.h"

#include <algorithm>

void PeakMeterProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (sampleRate, maximumExpectedSamplesPerBlock);

    // TODO(1): o que precisa de memória deve ser preparado AQUI, não no processBlock.
    std::lock_guard<std::mutex> lock (mutex);
    peaks.clear();
    history.clear();
}

void PeakMeterProcessor::releaseResources()
{
}

void PeakMeterProcessor::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(2): este processBlock quebra três regras de tempo-real. Encontra-as e corrige-as:
    //   - aloca memória (resize / push_back podem chamar o allocator);
    //   - bloqueia num mutex que a GUI também usa;
    //   - formata texto e escreve no log (DBG) em cada bloco.
    //
    // Pistas: std::atomic<float> por canal; juce::AbstractFifo + um array de tamanho fixo para o
    // histórico. Se o histórico estiver cheio, descarta o valor novo (nunca esperes pela GUI).
    std::lock_guard<std::mutex> lock (mutex);

    peaks.resize (static_cast<size_t> (buffer.getNumChannels()));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peaks[(size_t) channel] = buffer.getMagnitude (channel, 0, buffer.getNumSamples());

    const auto blockPeak = peaks.empty() ? 0.0f : *std::max_element (peaks.begin(), peaks.end());

    if (history.size() < static_cast<size_t> (historyCapacity))
        history.push_back (blockPeak);

    DBG ("bloco com " << buffer.getNumSamples() << " amostras, pico " << blockPeak);
}

float PeakMeterProcessor::getPeak (int channel) const noexcept
{
    // TODO(3): sem mutex.
    std::lock_guard<std::mutex> lock (mutex);

    if (! juce::isPositiveAndBelow (channel, static_cast<int> (peaks.size())))
        return 0.0f;

    return peaks[(size_t) channel];
}

int PeakMeterProcessor::readHistory (float* destination, int maxValues) noexcept
{
    // TODO(4): sem mutex: lê da FIFO lock-free.
    std::lock_guard<std::mutex> lock (mutex);

    const auto num = std::min (maxValues, static_cast<int> (history.size()));
    std::copy (history.begin(), history.begin() + num, destination);
    history.erase (history.begin(), history.begin() + num);
    return num;
}
