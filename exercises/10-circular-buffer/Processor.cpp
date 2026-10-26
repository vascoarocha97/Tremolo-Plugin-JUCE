#include "Processor.h"

#include <algorithm>

void CircularDelayLine::prepare (int maximumDelayInSamples)
{
    // TODO(1): reserva o buffer (quantas posições são precisas para um atraso máximo de N?),
    //          põe-no a zeros e inicializa writeIndex.
    juce::ignoreUnused (maximumDelayInSamples);
}

void CircularDelayLine::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

void CircularDelayLine::push (float sample) noexcept
{
    // TODO(2): avança writeIndex (dando a volta no fim do buffer) e escreve a amostra.
    juce::ignoreUnused (sample);
}

float CircularDelayLine::read (int delayInSamples) const noexcept
{
    // TODO(3): limita o atraso a [0, máximo] e lê a posição writeIndex - atraso, dando a volta
    //          se ficar negativa. Sem std::fmod nem % com números negativos (ver README).
    juce::ignoreUnused (delayInSamples);
    return 0.0f;
}

void IntegerDelay::prepare (const juce::dsp::ProcessSpec& spec)
{
    // TODO(4): calcula maximumDelaySamples a partir de maximumDelaySeconds e do sample rate,
    //          cria uma CircularDelayLine por canal (spec.numChannels) e prepara cada uma.
    //          No fim, volta a aplicar o atraso atual (setDelaySamples), para ficar dentro do novo máximo.
    juce::ignoreUnused (spec);
}

void IntegerDelay::reset() noexcept
{
    for (auto& line : lines)
        line.reset();
}

void IntegerDelay::setDelaySamples (int newDelay) noexcept
{
    delaySamples = juce::jlimit (0, maximumDelaySamples, newDelay);
}

void IntegerDelay::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(5): para cada canal (com linha preparada) e cada amostra: escreve a entrada na linha
    //          e substitui-a pelo valor de há delaySamples amostras.
    juce::ignoreUnused (context);
}
