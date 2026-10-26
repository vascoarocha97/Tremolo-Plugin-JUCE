#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../22-waveshaper/Processor.h" // as TUAS curvas do exercício 22

#include <vector>

/** Exercício 23 — Fuzz.

    Uma cadeia típica de pedal de fuzz:
      passa-alto de entrada (100 Hz) -> ganho enorme -> clip assimétrico -> DC blocker -> tom -> volume */

/** Remove a componente DC (média) do sinal: um passa-alto de 1.ª ordem com corte muito baixo.
        y[n] = x[n] − x[n−1] + R · y[n−1],      R = 1 − 2π·fc/fs */
class DcBlocker
{
public:
    void prepare (double sampleRate, float cutoffHz = 10.0f) noexcept;
    void reset() noexcept { x1 = y1 = 0.0f; }
    float processSample (float x) noexcept;

    float getR() const noexcept { return R; }

private:
    float R = 0.995f;
    float x1 = 0.0f, y1 = 0.0f;
};

class Fuzz
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setGainDecibels (float db) noexcept;    // o "fuzz": 0–50 dB
    void setTone (float newTone) noexcept;        // 0 = escuro (500 Hz), 1 = brilhante (10 kHz)
    void setOutputDecibels (float db) noexcept;

    /** Frequência de corte do filtro de tom para um valor de tom em [0, 1] (exponencial). */
    static float toneToFrequency (float tone) noexcept;

private:
    juce::dsp::FirstOrderTPTFilter<float> inputHighPass, toneLowPass;
    std::vector<DcBlocker> dcBlockers;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gain { 1.0f }, output { 1.0f };
    float gainTarget = 30.0f, outputTarget = 0.5f, tone = 0.5f;
};
