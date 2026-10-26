#include "Processor.h"

#include <cmath>

float Gain::decibelsToGain (float decibels) noexcept
{
    // TODO(1): ganho = 10^(dB / 20). Abaixo de (ou igual a) minusInfinityDb devolve 0.
    juce::ignoreUnused (decibels);
    return 1.0f;
}

float Gain::gainToDecibels (float gain) noexcept
{
    // TODO(2): dB = 20 · log10(ganho). Ganhos <= 0 dão minusInfinityDb, e o resultado nunca
    //          fica abaixo de minusInfinityDb.
    juce::ignoreUnused (gain);
    return 0.0f;
}

float Gain::powerRatioToDecibels (float powerRatio) noexcept
{
    // TODO(3): para razões de POTÊNCIA o fator é 10 e não 20 (ver README). Mesmo piso que acima.
    juce::ignoreUnused (powerRatio);
    return 0.0f;
}

void DecibelGainProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (sampleRate, maximumExpectedSamplesPerBlock);
}

void DecibelGainProcessor::setGainDecibels (float newGainDb) noexcept
{
    // TODO(4): converte para ganho linear AQUI (uma vez), não em cada amostra.
    juce::ignoreUnused (newGainDb);
}

void DecibelGainProcessor::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(5): multiplica todas as amostras por gainLinear.
    juce::ignoreUnused (buffer);
}
