#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "AnalyserComponents.h"
#include "HostAudioEngine.h"

namespace learn
{

/** Janela principal do standalone de um exercício. */
class HostComponent final : public juce::Component,
                            private juce::Timer
{
public:
    HostComponent (std::unique_ptr<juce::AudioProcessor> processorToHost,
                   const juce::String& exerciseNumber, const juce::String& webPage);
    ~HostComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Modo de verificação automática: toca durante ~3 s, escreve o resultado em `resultFile`
        e fecha a aplicação (código de saída 0 se houve som e não houve NaN). */
    void startSmokeTest (const juce::File& resultFile);

private:
    void timerCallback() override;
    void showAudioSettings();
    void chooseFile();
    void restartAudio();
    void updateSourceControls();
    static juce::File getDeviceSettingsFile();
    juce::File getProcessorStateFile() const;

    std::unique_ptr<juce::AudioProcessor> processor;
    HostAudioEngine engine;
    juce::AudioDeviceManager deviceManager;
    juce::String exerciseNumber, webPage;

    juce::Label titleLabel, statusLabel, parametersLabel;
    juce::TextButton webButton { juce::String::fromUTF8 ("Abrir visualização (web)") };
    juce::ComboBox sourceBox;
    juce::Slider levelSlider, frequencySlider, outputSlider;
    juce::Label levelLabel, frequencyLabel, outputLabel;
    juce::TextButton loadButton { juce::String::fromUTF8 ("Escolher ficheiro...") };
    juce::ToggleButton bypassButton { "Bypass (A/B)" };
    juce::TextButton settingsButton { juce::String::fromUTF8 ("Áudio...") };
    juce::TextButton resetButton { juce::String::fromUTF8 ("Reiniciar áudio") };
    juce::ComboBox programBox; // só aparece se o processador tiver programas (presets), exercício 33+

    juce::Viewport parameterViewport;
    juce::Component parameterContent; // editor próprio do exercício (se houver) + parâmetros genéricos
    std::unique_ptr<juce::AudioProcessorEditor> editor, genericEditor;

    ScopeComponent scope;
    SpectrumComponent spectrum;
    LevelMeter meter;

    SampleHistory inputHistory { 1 << 13 }, outputHistory { 1 << 13 };
    std::vector<float> pullBuffer = std::vector<float> (1 << 15);
    std::unique_ptr<juce::FileChooser> fileChooser;
    int clipDisplayCountdown = 0;

    juce::File smokeTestFile;
    int smokeTestTicks = -1;
    float smokeTestPeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HostComponent)
};

} // namespace learn
