#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

/** Exercício 29 — Feedback Delay Network (FDN).

    N delay lines cujas saídas são misturadas por uma matriz ORTOGONAL e devolvidas às entradas.
    Uma matriz ortogonal não ganha nem perde energia; um ganho g_i < 1 em cada linha controla o
    decaimento. Comparado com os combs em paralelo do Schroeder, cada eco "visita" todas as linhas:
    a densidade de ecos cresce muito depressa e o som fica mais liso.

    Os exercícios 30 (difusor) e 31 (shimmer) usam esta classe. */
class FeedbackDelayNetwork
{
public:
    static constexpr int size = 8;
    using Frame = std::array<float, size>;

    /** Atrasos (ms) — valores diferentes e sem múltiplos comuns simples, entre 100 e 200 ms. */
    static constexpr std::array<float, size> delayTimesMs { 101.3f, 113.9f, 127.1f, 139.7f, 151.1f, 163.3f, 179.9f, 197.3f };

    /** Matriz de Householder aplicada "no sítio": v ← v − (2/N)·(Σ v)·[1, 1, ..., 1].
        É ortogonal (preserva ‖v‖) e é a sua própria inversa. Custa O(N), não O(N²). */
    static void householder (Frame& v) noexcept;

    void prepare (double sampleRate);
    void reset() noexcept;

    /** Ganho por linha para um RT60: g_i = 0.001^(D_i / (T60 · fs)) (como no exercício 28). */
    void setDecaySeconds (float rt60) noexcept;
    float getLineGain (int line) const noexcept { return gains[(size_t) line]; }

    /** Um passo da rede: lê as N linhas, mistura, aplica os ganhos, soma a entrada e escreve.
        Devolve as N saídas lidas (antes da mistura). */
    Frame processFrame (const Frame& input) noexcept;

    /** Conveniência: entrada mono (igual em todas as linhas, escalada) e saída estéreo
        (duas combinações diferentes das N saídas). */
    std::pair<float, float> processSample (float monoInput) noexcept;

    /** Energia total guardada nas linhas (soma dos quadrados): para verificar que, com g = 1,
        a rede não ganha nem perde energia. */
    double getStoredEnergy() const noexcept;

private:
    std::array<std::vector<float>, size> lines;
    std::array<int, size> indices {};
    Frame gains {};
    double sampleRate = 44100.0;
    float decay = 2.0f;
};

/** Reverb FDN estéreo com dry/wet (convenção juce::dsp). */
class FdnReverb
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept { fdn.reset(); }
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    void setDecaySeconds (float rt60) noexcept { fdn.setDecaySeconds (rt60); }
    void setMix (float newMix) noexcept { mix = juce::jlimit (0.0f, 1.0f, newMix); }

private:
    FeedbackDelayNetwork fdn;
    float mix = 0.3f;
};
