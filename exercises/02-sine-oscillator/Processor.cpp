#include "Processor.h"

#include <cmath>
#include <numbers>

void SineOscillator::prepareToPlay (double newSampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (maximumExpectedSamplesPerBlock);

    // TODO(1): guarda o sample rate, atualiza o incremento de fase e faz reset à fase.
    juce::ignoreUnused (newSampleRate);
}

void SineOscillator::releaseResources()
{
}

void SineOscillator::setFrequency (float newFrequencyHz) noexcept
{
    frequency = newFrequencyHz;
    updateIncrement();
}

void SineOscillator::updateIncrement() noexcept
{
    // TODO(2): calcula phaseIncrement (radianos por amostra) a partir de `frequency` e `sampleRate`.
    //          Quantos radianos avança a fase numa amostra, se num segundo avança 2π·frequency?
    //          Cuidado: esta função pode ser chamada antes do prepareToPlay (sampleRate == 0).
}

void SineOscillator::reset() noexcept
{
    phase = 0.0;
}

void SineOscillator::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(3): para cada amostra n do bloco:
    //   - calcula amplitude · sin(phase)
    //   - escreve esse valor em TODOS os canais na posição n
    //   - avança a fase e mantém-na em [0, 2π)
    //
    // A fase é um membro da classe: o próximo bloco tem de continuar exatamente daqui.
    juce::ignoreUnused (buffer);
}
