#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../29-fdn/Processor.h" // a TUA FDN do exercício 29

#include <vector>

/** Exercício 31 — Shimmer: um pitch shifter DENTRO do feedback de uma reverb.

    Cada vez que o som dá uma volta ao ciclo, sobe uma oitava: a cauda "brilha" e sobe
    (o som "shimmer" associado às produções de Brian Eno e Daniel Lanois).

    PitchShifter é o mais simples que existe: duas leituras de uma delay line cujo atraso
    varia linearmente (a leitura anda à velocidade `ratio`), com crossfade Hann entre elas
    para esconder o salto quando o atraso dá a volta. */
class PitchShifter
{
public:
    void prepare (double sampleRate, float windowMs = 50.0f);
    void reset() noexcept;

    void setSemitones (float semitones) noexcept;
    float getRatio() const noexcept { return ratio; }

    float processSample (float input) noexcept;

    /** Ganho de crossfade de uma leitura com fase p ∈ [0, 1): sin²(π·p).
        Nota: gain(p) + gain(p + 0.5) = sin² + cos² = 1 — o volume não oscila. */
    static float crossfadeGain (float phase) noexcept;

private:
    float readDelayed (float delaySamples) const noexcept;

    std::vector<float> buffer;
    int writeIndex = 0;
    float windowSamples = 2400.0f;
    float phase = 0.0f;
    float ratio = 2.0f;
};

class ShimmerReverb
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    void setDecaySeconds (float rt60) noexcept { fdn.setDecaySeconds (rt60); }
    void setShimmer (float amount) noexcept { shimmer = juce::jlimit (0.0f, 1.0f, amount); }
    void setSemitones (float semitones) noexcept { shifter.setSemitones (semitones); }
    void setMix (float newMix) noexcept { mix = juce::jlimit (0.0f, 1.0f, newMix); }

private:
    FeedbackDelayNetwork fdn;
    PitchShifter shifter;
    float shimmer = 0.4f;
    float mix = 0.4f;

    float lastWet = 0.0f;       // saída da reverb na amostra anterior (vai voltar à entrada)
    float lowpassState = 0.0f;  // filtro passa-baixo dentro do ciclo de feedback
    float lowpassCoeff = 0.5f;
};
