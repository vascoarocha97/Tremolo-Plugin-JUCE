#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../27-comb-allpass/Processor.h" // os TEUS comb e allpass do exercício 27

#include <array>

/** Exercício 28 — Reverb de Schroeder (estrutura do Freeverb).

    Por canal: 8 combs (com damping) em PARALELO, somados, seguidos de 4 allpasses em SÉRIE.
    O canal direito usa atrasos ligeiramente maiores (+23 amostras a 44.1 kHz) para L e R não serem
    iguais (descorrelação = sensação de espaço). */
class SchroederReverb
{
public:
    static constexpr int numCombs = 8;
    static constexpr int numAllpasses = 4;

    /** Atrasos do Freeverb, em amostras a 44.1 kHz (escalados para o sample rate real). */
    static constexpr std::array<int, numCombs> combTunings { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static constexpr std::array<int, numAllpasses> allpassTunings { 556, 441, 341, 225 };
    static constexpr int stereoSpread = 23;

    /** Ganho de feedback para que um comb de `delaySamples` decaia 60 dB em `rt60Seconds`. */
    static float feedbackForRt60 (int delaySamples, double sampleRate, float rt60Seconds) noexcept;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setDecaySeconds (float rt60) noexcept;   // RT60
    void setDamping (float newDamping) noexcept;  // 0 = brilhante, 1 = muito escuro
    void setMix (float newMix) noexcept { mix = juce::jlimit (0.0f, 1.0f, newMix); }

private:
    void updateFeedback() noexcept;

    std::array<std::array<CombFilter, numCombs>, 2> combs;
    std::array<std::array<SchroederAllpass, numAllpasses>, 2> allpasses;
    double sampleRate = 44100.0;
    float decay = 2.0f, damping = 0.3f, mix = 0.3f;
};
