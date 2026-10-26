#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 11 — Delay fracionário (interpolação linear). */
class FractionalDelayLine
{
public:
    /** Reserva memória para atrasos de 0 até maximumDelayInSamples. Aloca. */
    void prepare (int maximumDelayInSamples);
    void reset() noexcept;
    void push (float sample) noexcept;

    /** Atraso inteiro (como no exercício 10), limitado a [0, máximo]. */
    float read (int delayInSamples) const noexcept;

    /** Atraso fracionário com interpolação linear entre as duas amostras vizinhas.
        Limitado a [0, máximo]. readLinear (3.0f) == read (3). */
    float readLinear (float delayInSamples) const noexcept;

    int getMaximumDelayInSamples() const noexcept { return maximumDelay; }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    int maximumDelay = 0;
};

/** Delay com tempo em milissegundos, que pode mudar suavemente (convenção juce::dsp). */
class FractionalDelay
{
public:
    static constexpr double maximumDelaySeconds = 2.0;
    static constexpr double smoothingSeconds = 0.05;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. O atraso efetivo desliza até ao novo valor em 50 ms. */
    void setDelayMilliseconds (float newDelayMs) noexcept;

private:
    std::vector<FractionalDelayLine> lines;
    juce::SmoothedValue<float> delayInSamples;
    double sampleRate = 44100.0;
    float targetDelayMs = 0.0f;
};
