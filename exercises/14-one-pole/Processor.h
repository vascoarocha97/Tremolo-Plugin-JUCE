#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 14 — Filtro de um polo.

    O filtro IIR mais simples que existe: a saída aproxima-se da entrada uma fração `a` de cada vez.
        passa-baixo:  y[n] = y[n-1] + a · (x[n] - y[n-1])
        passa-alto:   x[n] - passa-baixo[n]
    Um canal só; o processador mais abaixo usa um por canal. */
class OnePoleFilter
{
public:
    enum class Type
    {
        lowPass,
        highPass
    };

    /** Calcula o coeficiente `a` para a frequência de corte dada (limitada a [1 Hz, 0.49·fs]). */
    void setCutoff (float cutoffHz, double sampleRate) noexcept;
    void setType (Type newType) noexcept { type = newType; }

    /** Põe o estado (a "memória" do filtro) num valor. */
    void reset (float initialValue = 0.0f) noexcept { state = initialValue; }

    float processSample (float x) noexcept;

    float getCoefficient() const noexcept { return a; }

private:
    float a = 1.0f;      // 0 < a <= 1: quanto o estado anda em direção à entrada em cada amostra
    float state = 0.0f;  // a saída do passa-baixo na amostra anterior
    Type type = Type::lowPass;
};

/** Um OnePoleFilter por canal (convenção juce::dsp). */
class OnePoleProcessor
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setCutoff (float cutoffHz) noexcept;
    void setType (OnePoleFilter::Type newType) noexcept;

private:
    std::vector<OnePoleFilter> filters;
    double sampleRate = 44100.0;
    float cutoff = 1000.0f;
    OnePoleFilter::Type type = OnePoleFilter::Type::lowPass;
};
