#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 08 — Parameter smoothing.

    Um "smoother" linear: quando o alvo muda, o valor vai até lá em linha reta durante um número
    fixo de amostras. É a mesma ideia (e o mesmo comportamento) que juce::SmoothedValue<float>
    com ValueSmoothingTypes::Linear. */
class LinearSmoother
{
public:
    /** Define a duração da rampa (rampLengthSeconds · sampleRate amostras, arredondado para baixo)
        e salta imediatamente para o alvo atual (sem rampa). */
    void reset (double sampleRate, double rampLengthSeconds) noexcept;

    /** Salta para `newValue` (valor atual e alvo), sem rampa. */
    void setCurrentAndTargetValue (float newValue) noexcept;

    /** Começa uma rampa nova, a partir do valor ATUAL, que chega a `newTarget` ao fim de
        exatamente rampLength amostras. Se o alvo não mudou, não faz nada.
        Se a rampa tiver 0 amostras, salta logo para o alvo. */
    void setTargetValue (float newTarget) noexcept;

    /** Avança uma amostra e devolve o valor novo. Na N-ésima chamada depois de setTargetValue
        (N = rampLength) devolve exatamente o alvo. Sem rampa ativa, devolve o alvo. */
    float getNextValue() noexcept;

    /** Equivale a chamar getNextValue() numSamples vezes. */
    void skip (int numSamples) noexcept;

    bool isSmoothing() const noexcept       { return countdown > 0; }
    float getCurrentValue() const noexcept  { return current; }
    float getTargetValue() const noexcept   { return target; }
    int getRampLengthInSamples() const noexcept { return rampLengthInSamples; }

private:
    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    int countdown = 0;
    int rampLengthInSamples = 0;
};

/** Ganho com smoothing: mudar o ganho de repente não faz click. */
class SmoothedGainProcessor
{
public:
    static constexpr double rampLengthSeconds = 0.05;

    SmoothedGainProcessor() { gain.setCurrentAndTargetValue (1.0f); }

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources() {}

    /** Ganho linear (não dB). Pode ser chamado entre blocos. */
    void setGain (float newGain) noexcept;

    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    LinearSmoother gain;
};
