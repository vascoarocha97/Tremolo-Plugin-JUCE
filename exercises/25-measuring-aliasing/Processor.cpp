#include "Processor.h"

#include <cmath>

std::vector<float> Spectrum::magnitudeDb (const float* signal, int order)
{
    // TODO(1): (ver README, secção "A FFT do JUCE")
    //   - um std::vector<float> de 2·size zeros, com as primeiras size amostras do sinal;
    //   - janela de Hann NORMALIZADA (juce::dsp::WindowingFunction<float>, normalise = true);
    //   - juce::dsp::FFT (order).performFrequencyOnlyForwardTransform (dados, true);
    //   - converte cada um dos size/2 primeiros valores para dB com a escala certa
    //     (um seno de amplitude A dá |X| = A·size/2 com a janela normalizada).
    juce::ignoreUnused (signal, order);
    return {};
}

double Spectrum::binFrequency (int bin, int order, double sampleRate) noexcept
{
    // TODO(2)
    juce::ignoreUnused (bin, order, sampleRate);
    return 0.0;
}

double Spectrum::aliasingRatioDb (const float* signal, int order, double f0, double sampleRate, int halfWidth)
{
    // TODO(3):
    //   - o espectro em dB (magnitudeDb);
    //   - marca como "harmónicos" os bins a ±halfWidth de k·f0 (k = 1, 2, ... enquanto k·f0 < fs/2);
    //   - soma a POTÊNCIA (10^(dB/10)) dos bins harmónicos e dos outros, ignorando os bins 0..halfWidth (DC);
    //   - devolve 10·log10(outros / harmónicos), protegido contra divisões por zero.
    juce::ignoreUnused (signal, order, f0, sampleRate, halfWidth);
    return 0.0;
}
