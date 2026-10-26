#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 16 — State Variable Filter, topology-preserving transform (TPT).

    Um filtro de 2.ª ordem com três saídas ao mesmo tempo (passa-baixo, passa-banda, passa-alto),
    que se mantém estável mesmo quando a frequência de corte muda em cada amostra.
    Forma de Zavalishin ("The Art of VA Filter Design"), a mesma do juce::dsp::StateVariableTPTFilter. */
class StateVariableFilter
{
public:
    struct Outputs
    {
        float lowPass = 0.0f;
        float bandPass = 0.0f;  // sem normalizar: ganho Q em fc (como no JUCE)
        float highPass = 0.0f;
    };

    /** Limita fc a [10 Hz, 0.49·fs] e Q a >= 0.1, e recalcula g, R2 e h. Pode ser chamado por amostra. */
    void setParameters (float cutoffHz, float q, double sampleRate) noexcept;

    void reset() noexcept { s1 = s2 = 0.0f; }
    Outputs processSample (float x) noexcept;

    float getR2() const noexcept { return R2; }

private:
    float g = 0.0f;   // tan(π·fc/fs): o ganho de cada integrador
    float R2 = 1.0f;  // 1/Q: o amortecimento (2R na notação do Zavalishin)
    float h = 1.0f;   // 1 / (1 + R2·g + g²): resolve o "ciclo sem atraso"
    float s1 = 0.0f, s2 = 0.0f; // estado dos dois integradores
};

/** SVF multicanal com saída escolhida e modulação opcional do corte por um LFO (convenção juce::dsp). */
class SvfProcessor
{
public:
    enum class Mode { lowPass, bandPass, highPass };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. modulationOctaves = 0 desliga o LFO. */
    void setParameters (Mode newMode, float cutoffHz, float q, float modulationOctaves, float lfoHz) noexcept;

private:
    std::vector<StateVariableFilter> filters;
    double sampleRate = 44100.0;
    Mode mode = Mode::lowPass;
    float cutoff = 1000.0f, resonance = 0.7071f, octaves = 0.0f, lfoRate = 0.5f;
    double lfoPhase = 0.0;
};
