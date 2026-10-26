#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 10 — Buffer circular.

    Uma delay line de UM canal: guarda as últimas amostras num buffer de tamanho fixo e
    escreve por cima das mais antigas, dando a volta ao chegar ao fim. */
class CircularDelayLine
{
public:
    /** Reserva memória para atrasos de 0 até maximumDelayInSamples (inclusive) e limpa.
        Aloca: só fora da thread de áudio. */
    void prepare (int maximumDelayInSamples);

    /** Põe tudo a zero (sem alocar). */
    void reset() noexcept;

    /** Escreve a amostra mais recente. */
    void push (float sample) noexcept;

    /** A amostra escrita há `delayInSamples` pushes: read (0) é a última, read (1) a anterior...
        O atraso é limitado a [0, getMaximumDelayInSamples()]. */
    float read (int delayInSamples) const noexcept;

    int getMaximumDelayInSamples() const noexcept { return static_cast<int> (buffer.size()) - 1; }

private:
    std::vector<float> buffer;
    int writeIndex = 0; // onde está a amostra mais recente
};

/** Atrasa cada canal exatamente N amostras (convenção juce::dsp). */
class IntegerDelay
{
public:
    static constexpr double maximumDelaySeconds = 2.0;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. Limitado a [0, máximo]. */
    void setDelaySamples (int newDelay) noexcept;
    int getDelaySamples() const noexcept { return delaySamples; }
    int getMaximumDelaySamples() const noexcept { return maximumDelaySamples; }

private:
    std::vector<CircularDelayLine> lines; // uma por canal
    int delaySamples = 0;
    int maximumDelaySamples = 0;
};
