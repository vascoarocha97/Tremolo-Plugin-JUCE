#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace learn
{

/** Guarda as últimas N amostras (do lado da GUI). */
class SampleHistory
{
public:
    explicit SampleHistory (int size) : data (static_cast<size_t> (size), 0.0f) {}

    void append (const float* samples, int num);

    /** Copia as últimas `num` amostras, da mais antiga para a mais recente. */
    void copyLatest (float* destination, int num) const;

    int size() const noexcept { return static_cast<int> (data.size()); }

private:
    std::vector<float> data;
    int writeIndex = 0;
};

//==============================================================================
/** Osciloscópio com trigger na passagem ascendente por zero. Entrada a cinzento, saída a cor. */
class ScopeComponent final : public juce::Component
{
public:
    ScopeComponent();

    void update (const SampleHistory& input, const SampleHistory& output, double sampleRate);
    void paint (juce::Graphics&) override;

private:
    std::vector<float> inputSnapshot, outputSnapshot;
    double sampleRate = 44100.0;
};

//==============================================================================
/** Analisador de espectro (FFT de 4096 pontos, janela de Hann, eixo de frequência logarítmico). */
class SpectrumComponent final : public juce::Component
{
public:
    SpectrumComponent();

    void update (const SampleHistory& input, const SampleHistory& output, double sampleRate);
    void paint (juce::Graphics&) override;

private:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;

    void computeSpectrum (const SampleHistory& history, std::vector<float>& smoothedDb);
    float frequencyToX (float frequency, float width) const;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { static_cast<size_t> (fftSize),
                                                 juce::dsp::WindowingFunction<float>::hann, false };
    std::vector<float> fftData;
    std::vector<float> inputDb, outputDb;
    double sampleRate = 44100.0;
};

//==============================================================================
/** Medidor de pico L/R em dBFS com peak-hold. */
class LevelMeter final : public juce::Component
{
public:
    void setLevels (float left, float right);
    void paint (juce::Graphics&) override;

private:
    float levels[2] {}, holds[2] {};
    int holdCounters[2] {};
};

} // namespace learn
