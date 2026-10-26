#include "Processor.h"

#include <cmath>

double Nyquist::aliasedFrequency (double frequency, double sampleRate)
{
    // TODO(1): devolve a frequência (entre 0 e sampleRate/2) que efetivamente se ouve.
    //
    //  Duas ideias (ver README):
    //   - o espectro de um sinal amostrado repete-se a cada sampleRate;
    //   - e é simétrico: uma frequência acima de sampleRate/2 "dobra" para baixo.
    //
    //  std::fmod e std::abs podem dar jeito.
    juce::ignoreUnused (sampleRate);
    return frequency;
}

void NaiveSawOscillator::prepareToPlay (double newSampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (maximumExpectedSamplesPerBlock);
    sampleRate = newSampleRate;
    updateIncrement();
    reset();
}

void NaiveSawOscillator::setFrequency (float newFrequencyHz) noexcept
{
    frequency = newFrequencyHz;
    updateIncrement();
}

void NaiveSawOscillator::updateIncrement() noexcept
{
    // TODO(2): incremento em CICLOS por amostra (a fase deste oscilador vai de 0 a 1, não de 0 a 2π).
}

void NaiveSawOscillator::reset() noexcept
{
    phase = 0.0;
}

void NaiveSawOscillator::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(3): para cada amostra, transforma a fase [0, 1) numa rampa [-1, 1), escreve-a em todos os
    //          canais, e avança a fase mantendo-a em [0, 1).
    juce::ignoreUnused (buffer);
}
