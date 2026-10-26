#include "Processor.h"

#include <cmath>

double Quantiser::stepSize (int bits) noexcept
{
    // TODO(1): tamanho de 1 LSB na escala [-1, 1) para `bits` bits.
    //          Pensa num inteiro com sinal de N bits: quantos valores há entre 0 e o máximo?
    juce::ignoreUnused (bits);
    return 1.0;
}

float Quantiser::quantise (float x, int bits) noexcept
{
    // TODO(2): arredonda x para o múltiplo de stepSize(bits) mais próximo (std::round)
    //          e limita o resultado a [-1, 1 - LSB].
    juce::ignoreUnused (bits);
    return x;
}

void BitDepthReducer::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (sampleRate, maximumExpectedSamplesPerBlock);
    random.setSeed (12345); // determinístico: os testes dão sempre o mesmo resultado
}

void BitDepthReducer::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(3): quantiza todas as amostras de todos os canais para `bits` bits.
    //
    // TODO(4): se ditherEnabled, ANTES de quantizar soma a cada amostra ruído TPDF com amplitude
    //          de ±1 LSB. Ruído TPDF = DIFERENÇA de dois números aleatórios uniformes independentes
    //          (random.nextFloat() devolve um uniforme em [0, 1): a soma de dois teria média 1,
    //          e um dither com média ≠ 0 acrescenta um desvio DC).
    juce::ignoreUnused (buffer);
}
