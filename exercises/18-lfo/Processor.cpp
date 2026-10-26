#include "Processor.h"

#include <cmath>
#include <numbers>

void Lfo::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    setFrequency (frequency);
}

void Lfo::setFrequency (float hz) noexcept
{
    frequency = hz;
    // TODO(1): incremento em CICLOS por amostra.
}

float Lfo::valueAt (Shape shape, double phaseInCycles) noexcept
{
    // TODO(2): reduz a fase a [0, 1) (a fase pode vir negativa ou maior do que 1 por causa do desvio)
    //          e devolve o valor de cada forma (ver os comentários do enum Shape no .h e o README).
    //          Todas as formas começam em 0 e sobem, como o seno, exceto a quadrada.
    juce::ignoreUnused (shape, phaseInCycles);
    return 0.0f;
}

float Lfo::getNextSample() noexcept
{
    // TODO(3): o valor na fase atual (mais o desvio de fase) e depois avança a fase, mantendo-a em [0, 1).
    return 0.0f;
}
