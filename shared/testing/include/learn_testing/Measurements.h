#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace learn::test
{

/** Converte amplitude linear em dB (com piso de -200 dB para não dar -inf). */
inline double toDb (double linear)
{
    return 20.0 * std::log10 (std::max (std::abs (linear), 1.0e-10));
}

/** Pico absoluto de um canal, no intervalo [start, start + num). num < 0 => até ao fim. */
inline float peak (const juce::AudioBuffer<float>& buffer, int channel, int start = 0, int num = -1)
{
    if (num < 0)
        num = buffer.getNumSamples() - start;

    return buffer.getMagnitude (channel, start, num);
}

/** Valor RMS de um canal, no intervalo [start, start + num). num < 0 => até ao fim. */
inline double rms (const juce::AudioBuffer<float>& buffer, int channel, int start = 0, int num = -1)
{
    if (num < 0)
        num = buffer.getNumSamples() - start;

    const auto* data = buffer.getReadPointer (channel, start);
    double sum = 0.0;

    for (int n = 0; n < num; ++n)
        sum += static_cast<double> (data[n]) * data[n];

    return num > 0 ? std::sqrt (sum / num) : 0.0;
}

/** Média (componente DC) de um canal. */
inline double mean (const juce::AudioBuffer<float>& buffer, int channel, int start = 0, int num = -1)
{
    if (num < 0)
        num = buffer.getNumSamples() - start;

    const auto* data = buffer.getReadPointer (channel, start);
    double sum = 0.0;

    for (int n = 0; n < num; ++n)
        sum += data[n];

    return num > 0 ? sum / num : 0.0;
}

/** Maior salto entre duas amostras consecutivas: |x[n] - x[n-1]|.
    Um salto grande num sinal que devia ser suave ouve-se como um "click". */
inline float maxSampleJump (const juce::AudioBuffer<float>& buffer, int channel, int start = 0,
                            int num = -1)
{
    if (num < 0)
        num = buffer.getNumSamples() - start;

    const auto* data = buffer.getReadPointer (channel, start);
    float maxJump = 0.0f;

    for (int n = 1; n < num; ++n)
        maxJump = std::max (maxJump, std::abs (data[n] - data[n - 1]));

    return maxJump;
}

/** true se não houver NaN nem infinitos em nenhum canal. */
inline bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
            if (! std::isfinite (data[n]))
                return false;
    }

    return true;
}

/** Amplitude (de pico) da componente sinusoidal à frequência dada.

    Faz uma DFT com janela de Hann num único ponto de frequência. Para um seno puro de amplitude A
    devolve ≈ A (erro < 1.5 dB mesmo que a frequência não caia exatamente num bin; exato quando
    frequency * num / sampleRate é inteiro). */
inline double amplitudeAt (const float* data, int num, double frequency, double sampleRate)
{
    double re = 0.0, im = 0.0, windowSum = 0.0;
    const auto w = 2.0 * std::numbers::pi * frequency / sampleRate;

    for (int n = 0; n < num; ++n)
    {
        const auto window = 0.5 - 0.5 * std::cos (2.0 * std::numbers::pi * n / num);
        windowSum += window;
        re += window * data[n] * std::cos (w * n);
        im -= window * data[n] * std::sin (w * n);
    }

    return 2.0 * std::sqrt (re * re + im * im) / windowSum;
}

inline double amplitudeAt (const juce::AudioBuffer<float>& buffer, int channel, double frequency,
                           double sampleRate, int start = 0, int num = -1)
{
    if (num < 0)
        num = buffer.getNumSamples() - start;

    return amplitudeAt (buffer.getReadPointer (channel, start), num, frequency, sampleRate);
}

/** Ganho em dB entre entrada e saída, medido por RMS a partir de `start` (para ignorar o
    transitório inicial de um filtro). */
inline double gainDb (const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output,
                      int channel = 0, int start = 0)
{
    return toDb (rms (output, channel, start) / std::max (rms (input, channel, start), 1.0e-12));
}

/** Frequência estimada pelo número de passagens ascendentes por zero. */
inline double estimateFrequency (const juce::AudioBuffer<float>& buffer, int channel,
                                 double sampleRate, int start = 0, int num = -1)
{
    if (num < 0)
        num = buffer.getNumSamples() - start;

    const auto* data = buffer.getReadPointer (channel, start);
    int first = -1, last = -1, crossings = 0;

    for (int n = 1; n < num; ++n)
    {
        if (data[n - 1] < 0.0f && data[n] >= 0.0f)
        {
            if (first < 0)
                first = n;

            last = n;
            ++crossings;
        }
    }

    if (crossings < 2)
        return 0.0;

    return (crossings - 1) * sampleRate / static_cast<double> (last - first);
}

/** Índice da primeira amostra com |x| > threshold, ou -1. */
inline int firstSampleAbove (const juce::AudioBuffer<float>& buffer, int channel, float threshold,
                             int start = 0)
{
    const auto* data = buffer.getReadPointer (channel);

    for (int n = start; n < buffer.getNumSamples(); ++n)
        if (std::abs (data[n]) > threshold)
            return n;

    return -1;
}

/** Diferença absoluta máxima entre dois buffers (mesmo tamanho). */
inline float maxAbsDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    jassert (a.getNumChannels() == b.getNumChannels() && a.getNumSamples() == b.getNumSamples());
    float maxDiff = 0.0f;

    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int n = 0; n < a.getNumSamples(); ++n)
            maxDiff = std::max (maxDiff, std::abs (a.getSample (ch, n) - b.getSample (ch, n)));

    return maxDiff;
}

} // namespace learn::test
