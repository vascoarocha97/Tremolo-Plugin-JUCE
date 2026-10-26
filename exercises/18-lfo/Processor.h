#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 18 — LFO (low-frequency oscillator).

    Um oscilador lento (tipicamente 0.05–20 Hz) que não se ouve diretamente: serve para mexer
    noutros parâmetros (volume, pan, delay, corte de um filtro...). Gera um valor por amostra.

    Os exercícios 19 (tremolo), 20 (chorus) e 21 (flanger) usam ESTA classe — a tua. */
class Lfo
{
public:
    enum class Shape
    {
        sine,     // sin(2π·fase)
        triangle, // 0 -> 1 -> 0 -> −1 -> 0, em linhas retas (mesma fase que o seno)
        saw,      // rampa: 0 -> 1, salta para −1, sobe até 0 (mesma fase que o seno)
        square    // +1 na primeira metade do ciclo, −1 na segunda
    };

    void prepare (double newSampleRate) noexcept;

    void setFrequency (float hz) noexcept;
    void setShape (Shape newShape) noexcept { shape = newShape; }

    /** Desvio de fase em ciclos (0.25 = 90°). Usado para pôr L e R desfasados. */
    void setPhaseOffset (float cycles) noexcept { phaseOffset = cycles; }

    /** Volta a fase a `startPhase` (em ciclos, [0, 1)). */
    void reset (double startPhase = 0.0) noexcept { phase = startPhase; }

    /** Valor bipolar em [−1, 1] na fase atual (com o desvio) e avança uma amostra. */
    float getNextSample() noexcept;

    /** Converte bipolar [−1, 1] em unipolar [0, 1]. */
    static float toUnipolar (float bipolar) noexcept { return 0.5f * (bipolar + 1.0f); }

    /** A forma de onda num ponto do ciclo (fase em ciclos, qualquer valor real). */
    static float valueAt (Shape shape, double phaseInCycles) noexcept;

    double getPhase() const noexcept { return phase; }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;      // em ciclos, [0, 1)
    double increment = 0.0;  // ciclos por amostra
    float frequency = 1.0f;
    float phaseOffset = 0.0f;
    Shape shape = Shape::sine;
};
