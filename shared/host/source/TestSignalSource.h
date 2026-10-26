#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <atomic>
#include <memory>

namespace learn
{

enum class SourceType
{
    silence = 0,
    demoLoop,
    sine,
    naiveSaw,
    sweep,
    noise,
    clicks,
    file,
    liveInput
};

/** Gera o sinal que entra no processador do exercício.

    Thread-safety:
      - setType/setFrequency/setLevelDb: qualquer thread (std::atomic).
      - prepare(): antes de o áudio arrancar (pode alocar).
      - render(): thread de áudio (não aloca, não bloqueia).
      - loadFile(): message thread. O ficheiro é passado ao áudio por troca de ponteiros sob um
        juce::SpinLock; a thread de áudio só faz *try*-lock e, se falhar, toca silêncio nesse bloco. */
class TestSignalSource
{
public:
    TestSignalSource();
    ~TestSignalSource();

    static juce::StringArray getSourceNames();

    void setType (SourceType newType) noexcept      { type.store (newType); }
    SourceType getType() const noexcept             { return type.load(); }
    void setFrequency (float hz) noexcept           { frequency.store (hz); }
    void setLevelDb (float db) noexcept             { levelGain.store (juce::Decibels::decibelsToGain (db)); }

    void prepare (double newSampleRate, int maximumBlockSize);

    void render (juce::AudioBuffer<float>& buffer, const float* const* inputs, int numInputs,
                 int inputOffset) noexcept;

    bool loadFile (const juce::File& file, juce::String& errorMessage);
    juce::String getLoadedFileName() const;

private:
    void renderGenerator (juce::AudioBuffer<float>& buffer) noexcept;
    void renderLooped (const juce::AudioBuffer<float>* loop, int& position,
                       juce::AudioBuffer<float>& buffer) noexcept;
    void resampleFileIfNeeded();

    std::atomic<SourceType> type { SourceType::demoLoop };
    std::atomic<float> frequency { 220.0f };
    std::atomic<float> levelGain { 0.5f };
    juce::SmoothedValue<float> smoothedLevel;

    double sampleRate = 44100.0;
    std::atomic<double> preparedSampleRate { 44100.0 }; // cópia para a message thread (ficheiros)
    double phase = 0.0;
    double sweepTime = 0.0;
    int clickCounter = 0;
    juce::Random random;

    juce::AudioBuffer<float> demoLoop;
    double demoLoopRate = 0.0;
    int demoLoopPosition = 0;

    juce::SpinLock fileLock;
    std::unique_ptr<juce::AudioBuffer<float>> fileOriginal, filePlayback;
    double fileOriginalRate = 0.0;
    int filePosition = 0;
    juce::String fileName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestSignalSource)
};

} // namespace learn
