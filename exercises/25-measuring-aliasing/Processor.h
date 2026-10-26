#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

/** Exercício 25 — Medir aliasing com a FFT.

    Ferramentas de ANÁLISE (offline, fora da thread de áudio: aqui alocar é permitido).
    Vais usá-las para provar números: quanto aliasing tem uma distorção, e quanto o oversampling
    (exercício 26) o reduz. */
namespace Spectrum
{
    /** Espectro de magnitude em dB de `2^order` amostras de `signal`, com janela de Hann.
        Escala: um seno de amplitude 1 cujo período cabe um número inteiro de vezes na janela dá 0 dB.
        Devolve 2^order / 2 valores: o bin k corresponde à frequência k · sampleRate / 2^order. */
    std::vector<float> magnitudeDb (const float* signal, int order);

    /** Frequência central do bin k. */
    double binFrequency (int bin, int order, double sampleRate) noexcept;

    /** Mede o aliasing de um sinal periódico de fundamental f0 (ex.: um seno distorcido):
          - energia "harmónica": a dos bins a ±halfWidth bins de k·f0 (k = 1, 2, ... abaixo de Nyquist);
          - energia "não harmónica": a de todos os outros bins acima de DC (k > halfWidth);
        e devolve 10·log10(não harmónica / harmónica) em dB. Quanto mais negativo, menos aliasing. */
    double aliasingRatioDb (const float* signal, int order, double f0, double sampleRate, int halfWidth = 4);
}
