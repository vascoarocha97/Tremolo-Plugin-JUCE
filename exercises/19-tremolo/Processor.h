#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../18-lfo/Processor.h" // o TEU LFO do exercício 18

#include <array>

/** Exercício 19 — Tremolo: modulação de amplitude por um LFO.

    ganho = 1 − profundidade · LFO_unipolar      (entre 1 − profundidade e 1)

    Em estéreo, o LFO do canal direito pode estar desfasado do esquerdo (até 180°: quando um
    lado sobe o outro desce, o que dá um efeito de "auto-pan"). */
class Tremolo
{
public:
    static constexpr int maxChannels = 2;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setRate (float hz) noexcept;
    void setDepth (float newDepth) noexcept;          // [0, 1], com smoothing
    void setShape (Lfo::Shape shape) noexcept;
    void setStereoPhase (float degrees) noexcept;     // desfasamento do canal direito [0, 180]

private:
    std::array<Lfo, maxChannels> lfos;
    juce::SmoothedValue<float> depth;
    double sampleRate = 44100.0;
    float depthTarget = 0.5f;
    float stereoPhaseDegrees = 0.0f;
};
