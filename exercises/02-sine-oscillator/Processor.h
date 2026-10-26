#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 02 — Oscilador sinusoidal.

    Um gerador: ignora o que vem no buffer e escreve um seno em todos os canais.
    A peça central é o acumulador de fase, que tem de continuar exatamente de onde ficou
    no bloco anterior. */
class SineOscillator
{
public:
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();

    /** Pode ser chamado entre blocos. A frequência nova aplica-se a partir da amostra seguinte. */
    void setFrequency (float newFrequencyHz) noexcept;
    void setAmplitude (float newAmplitude) noexcept { amplitude = newAmplitude; }

    /** Volta a fase a zero (a próxima amostra é sin(0) = 0). */
    void reset() noexcept;

    /** Substitui o conteúdo do buffer por amplitude · sin(fase), igual em todos os canais. */
    void processBlock (juce::AudioBuffer<float>& buffer);

    double getPhase() const noexcept { return phase; }

private:
    void updateIncrement() noexcept;

    double sampleRate = 0.0;
    float frequency = 440.0f;
    float amplitude = 0.5f;

    double phase = 0.0;          // em radianos, sempre em [0, 2π)
    double phaseIncrement = 0.0; // radianos por amostra
};
