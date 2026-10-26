#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "TestSignalSource.h"

#include <array>
#include <atomic>
#include <vector>

namespace learn
{

/** FIFO lock-free (um produtor, um consumidor) para mandar amostras do áudio para a GUI. */
class AnalysisFifo
{
public:
    explicit AnalysisFifo (int capacity) : fifo (capacity), data (static_cast<size_t> (capacity)) {}

    /** Thread de áudio. Se a GUI não acompanhar, as amostras em excesso são descartadas. */
    void push (const float* samples, int num) noexcept
    {
        const auto scope = fifo.write (num);
        copyIn (samples, scope.startIndex1, scope.blockSize1);
        copyIn (samples + scope.blockSize1, scope.startIndex2, scope.blockSize2);
    }

    /** Message thread. Devolve o número de amostras lidas. */
    int pull (float* destination, int maxNum) noexcept
    {
        const auto scope = fifo.read (juce::jmin (maxNum, fifo.getNumReady()));
        copyOut (destination, scope.startIndex1, scope.blockSize1);
        copyOut (destination + scope.blockSize1, scope.startIndex2, scope.blockSize2);
        return scope.blockSize1 + scope.blockSize2;
    }

private:
    void copyIn (const float* src, int start, int num) noexcept
    {
        if (num > 0)
            std::copy (src, src + num, data.begin() + start);
    }

    void copyOut (float* dest, int start, int num) noexcept
    {
        if (num > 0)
            std::copy (data.begin() + start, data.begin() + start + num, dest);
    }

    juce::AbstractFifo fifo;
    std::vector<float> data;
};

//==============================================================================
/** O "motor" do standalone: é aqui que o processador do exercício é chamado.

    É a mesma ideia que o juce::AudioProcessorPlayer (que o wrapper Standalone oficial usa):
      audioDeviceAboutToStart  ->  processor.prepareToPlay (sampleRate, blockSize)
      audioDeviceIOCallback... ->  processor.processBlock (buffer, midi)
      audioDeviceStopped       ->  processor.releaseResources()

    À volta do processador junta: fonte de teste, bypass com crossfade, ganho de saída e um
    limitador de proteção (NaN/inf => silêncio; |x| > 1 => clip). */
class HostAudioEngine final : public juce::AudioIODeviceCallback
{
public:
    explicit HostAudioEngine (juce::AudioProcessor& processorToUse);

    TestSignalSource& getSource() noexcept { return source; }

    void setBypassed (bool shouldBeBypassed) noexcept { bypassed.store (shouldBeBypassed); }
    void setOutputGainDb (float db) noexcept           { outputGain.store (juce::Decibels::decibelsToGain (db)); }

    AnalysisFifo& getInputFifo() noexcept  { return inputFifo; }
    AnalysisFifo& getOutputFifo() noexcept { return outputFifo; }

    /** Pico desde a última leitura (e faz reset). */
    float takeOutputPeak (int channel) noexcept { return outputPeaks[(size_t) juce::jlimit (0, 1, channel)].exchange (0.0f); }

    /** Número de amostras cortadas pelo limitador desde a última leitura. */
    int takeClipCount() noexcept   { return clipCount.exchange (0); }

    /** true se o processador produziu NaN/inf e a saída foi silenciada. */
    bool isMutedByNaN() const noexcept { return mutedByNaN.load(); }

    double getSampleRate() const noexcept { return currentSampleRate.load(); }
    int getBlockSize() const noexcept     { return currentBlockSize.load(); }

    //==============================================================================
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

private:
    void processChunk (const float* const* inputs, int numInputs, float* const* outputs,
                       int numOutputs, int offset, int numSamples) noexcept;

    juce::AudioProcessor& processor;
    TestSignalSource source;

    juce::AudioBuffer<float> workBuffer, dryBuffer;
    std::vector<float> monoScratch;
    juce::MidiBuffer midiBuffer;

    std::atomic<bool> bypassed { false };
    std::atomic<float> outputGain { 0.25f };
    juce::SmoothedValue<float> smoothedOutputGain, smoothedWet;

    AnalysisFifo inputFifo { 1 << 15 }, outputFifo { 1 << 15 };
    std::array<std::atomic<float>, 2> outputPeaks {};
    std::atomic<int> clipCount { 0 };
    std::atomic<bool> mutedByNaN { false };

    std::atomic<double> currentSampleRate { 0.0 };
    std::atomic<int> currentBlockSize { 0 };
    int maxBlockSize = 0;
    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostAudioEngine)
};

} // namespace learn
