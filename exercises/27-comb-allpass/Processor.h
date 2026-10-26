#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 27 — Comb e allpass: os dois blocos clássicos das reverbs artificiais.
    O exercício 28 (reverb de Schroeder/Freeverb) usa estas duas classes. */

/** Filtro pente com feedback e amortecimento ("lowpass-feedback comb", como no Freeverb):

        saída     = linha[n − D]
        filtrado  = (1 − damping) · saída + damping · filtrado   (passa-baixo de 1 polo no ciclo)
        linha[n]  = entrada + feedback · filtrado

    Com damping = 0 é o pente com feedback "puro": ecos em D, 2D, 3D... com ganhos 1, g, g²... */
class CombFilter
{
public:
    /** Reserva a linha para um atraso de exatamente `delaySamples` (>= 1) e limpa. Aloca. */
    void prepare (int delaySamples);
    void reset() noexcept;

    void setFeedback (float newFeedback) noexcept { feedback = newFeedback; }
    void setDamping (float newDamping) noexcept   { damping = juce::jlimit (0.0f, 1.0f, newDamping); }

    float processSample (float x) noexcept;

    int getDelaySamples() const noexcept { return static_cast<int> (buffer.size()); }

private:
    std::vector<float> buffer;
    int index = 0;
    float feedback = 0.5f, damping = 0.0f;
    float filterState = 0.0f;
};

/** Allpass de Schroeder (verdadeiro):  H(z) = (−g + z⁻ᴰ) / (1 − g·z⁻ᴰ)
    Ganho 1 em todas as frequências; só "espalha" o sinal no tempo. */
class SchroederAllpass
{
public:
    void prepare (int delaySamples);
    void reset() noexcept;

    void setGain (float newGain) noexcept { gain = newGain; }
    float processSample (float x) noexcept;

private:
    std::vector<float> buffer;
    int index = 0;
    float gain = 0.5f;
};

/** Um comb ou um allpass por canal, para ouvir no standalone (convenção juce::dsp). */
class CombAllpassDemo
{
public:
    enum class Mode { comb, allpass };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Mudar o atraso realoca: o wrapper só o faz no prepare. */
    void setParameters (Mode newMode, float delayMs, float gain, float damping) noexcept;

private:
    std::vector<CombFilter> combs;
    std::vector<SchroederAllpass> allpasses;
    Mode mode = Mode::comb;
    double sampleRate = 44100.0;
    float delayMs = 10.0f;
};
