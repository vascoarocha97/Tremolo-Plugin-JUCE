#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 15 — Biquad com os coeficientes do Audio EQ Cookbook (Robert Bristow-Johnson).

    H(z) = (b0 + b1·z⁻¹ + b2·z⁻²) / (1 + a1·z⁻¹ + a2·z⁻²)       (já normalizado: a0 = 1) */
struct BiquadCoefficients
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

/** As fórmulas do cookbook. Todas devolvem coeficientes já divididos por a0. */
namespace Rbj
{
    BiquadCoefficients lowPass (double sampleRate, double frequency, double q);
    BiquadCoefficients highPass (double sampleRate, double frequency, double q);
    /** Passa-banda com ganho de 0 dB no pico ("constant 0 dB peak gain" no cookbook). */
    BiquadCoefficients bandPass (double sampleRate, double frequency, double q);
    BiquadCoefficients notch (double sampleRate, double frequency, double q);
    BiquadCoefficients peaking (double sampleRate, double frequency, double q, double gainDb);
    BiquadCoefficients lowShelf (double sampleRate, double frequency, double q, double gainDb);
    BiquadCoefficients highShelf (double sampleRate, double frequency, double q, double gainDb);

    /** |H(e^jω)| à frequência dada: avalia o numerador e o denominador em z = e^(jω). */
    double magnitudeAt (const BiquadCoefficients& c, double frequency, double sampleRate);
}

/** Um biquad (um canal), na forma Transposed Direct Form II. */
class Biquad
{
public:
    void setCoefficients (const BiquadCoefficients& newCoefficients) noexcept;
    void reset() noexcept { s1 = s2 = 0.0f; }
    float processSample (float x) noexcept;

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float s1 = 0.0f, s2 = 0.0f; // os dois estados da TDF-II
};

/** Um EQ de uma banda com o teu biquad (convenção juce::dsp). */
class BiquadFilter
{
public:
    enum class Type { lowPass, highPass, bandPass, notch, peaking, lowShelf, highShelf };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. Os coeficientes são recalculados só quando algo muda. */
    void setParameters (Type newType, float frequency, float q, float gainDb) noexcept;
    BiquadCoefficients getCoefficients() const noexcept { return coefficients; }

private:
    void updateCoefficients() noexcept;

    std::vector<Biquad> filters;
    BiquadCoefficients coefficients;
    double sampleRate = 44100.0;
    Type type = Type::lowPass;
    float frequency = 1000.0f, q = 0.7071f, gainDb = 0.0f;
};
