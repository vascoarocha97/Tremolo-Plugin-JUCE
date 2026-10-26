#include "Processor.h"

#include <cmath>

void LinearSmoother::reset (double sampleRate, double rampLengthSeconds) noexcept
{
    rampLengthInSamples = static_cast<int> (std::floor (rampLengthSeconds * sampleRate));
    setCurrentAndTargetValue (target);
}

void LinearSmoother::setCurrentAndTargetValue (float newValue) noexcept
{
    current = target = newValue;
    countdown = 0;
}

void LinearSmoother::setTargetValue (float newTarget) noexcept
{
    // TODO(1): começa uma rampa nova a partir do valor atual (ver o comentário no .h):
    //   - se o alvo é o mesmo, não faz nada;
    //   - se a rampa tem 0 amostras, salta para o alvo;
    //   - senão: guarda o alvo, a contagem decrescente (countdown) e o passo por amostra.
    juce::ignoreUnused (newTarget);
}

float LinearSmoother::getNextValue() noexcept
{
    // TODO(2): avança uma amostra. Na última amostra da rampa o valor tem de ser EXATAMENTE o alvo
    //          (somar o passo N vezes em float não dá exatamente o alvo).
    return current;
}

void LinearSmoother::skip (int numSamples) noexcept
{
    // TODO(3): o mesmo que chamar getNextValue() numSamples vezes, mas sem o ciclo.
    juce::ignoreUnused (numSamples);
}

void SmoothedGainProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (maximumExpectedSamplesPerBlock);
    gain.reset (sampleRate, rampLengthSeconds);
}

void SmoothedGainProcessor::setGain (float newGain) noexcept
{
    gain.setTargetValue (newGain);
}

void SmoothedGainProcessor::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(4): aplica o ganho com smoothing. Atenção à ordem dos ciclos: o smoother deve avançar
    //          UMA vez por instante de tempo (amostra), e todos os canais recebem o mesmo valor.
    juce::ignoreUnused (buffer);
}
