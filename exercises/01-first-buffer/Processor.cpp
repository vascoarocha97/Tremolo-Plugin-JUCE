#include "Processor.h"

void FirstBufferProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    // Este processador não guarda estado, por isso não há nada a preparar.
    juce::ignoreUnused (sampleRate, maximumExpectedSamplesPerBlock);
}

void FirstBufferProcessor::releaseResources()
{
}

void FirstBufferProcessor::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(1): multiplica cada amostra de cada canal por `gain`.
    //
    //  - percorre os canais de 0 até buffer.getNumChannels()
    //  - para cada canal, pede um ponteiro de escrita com buffer.getWritePointer (channel)
    //  - percorre as amostras de 0 até buffer.getNumSamples()  (nunca um número fixo!)
    //
    // Escreve os ciclos à mão: o objetivo é perceber a forma do buffer.
    juce::ignoreUnused (buffer);
}

float FirstBufferProcessor::findPeak (const juce::AudioBuffer<float>& buffer)
{
    // TODO(2): devolve o maior valor absoluto (std::abs) de todas as amostras de todos os canais.
    //
    //  - aqui só lês: usa buffer.getReadPointer (channel), que devolve const float*
    juce::ignoreUnused (buffer);
    return 0.0f;
}
