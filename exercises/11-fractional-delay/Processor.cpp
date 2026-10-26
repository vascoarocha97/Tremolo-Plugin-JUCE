#include "Processor.h"

#include <algorithm>
#include <cmath>

void FractionalDelayLine::prepare (int maximumDelayInSamples)
{
    maximumDelay = juce::jmax (0, maximumDelayInSamples);
    // +2: a amostra atual, as N anteriores, e mais uma para interpolar no atraso máximo
    buffer.assign (static_cast<size_t> (maximumDelay + 2), 0.0f);
    writeIndex = 0;
}

void FractionalDelayLine::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

void FractionalDelayLine::push (float sample) noexcept
{
    // TODO(1): igual ao exercício 10 — copia a tua solução.
    juce::ignoreUnused (sample);
}

float FractionalDelayLine::read (int delayInSamples) const noexcept
{
    // TODO(2): igual ao exercício 10 (o máximo aqui é `maximumDelay`) — copia a tua solução.
    juce::ignoreUnused (delayInSamples);
    return 0.0f;
}

float FractionalDelayLine::readLinear (float delayInSamples) const noexcept
{
    // TODO(3): interpolação linear:
    //   - limita o atraso a [0, maximumDelay];
    //   - separa-o em parte inteira e fração;
    //   - devolve o valor na reta entre read (inteira) e read (inteira + 1).
    juce::ignoreUnused (delayInSamples);
    return 0.0f;
}

void FractionalDelay::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    lines.resize (spec.numChannels);

    for (auto& line : lines)
        line.prepare (static_cast<int> (maximumDelaySeconds * sampleRate));

    delayInSamples.reset (sampleRate, smoothingSeconds);
    reset();
}

void FractionalDelay::reset() noexcept
{
    for (auto& line : lines)
        line.reset();

    delayInSamples.setCurrentAndTargetValue (static_cast<float> (targetDelayMs * 0.001 * sampleRate));
}

void FractionalDelay::setDelayMilliseconds (float newDelayMs) noexcept
{
    targetDelayMs = juce::jlimit (0.0f, static_cast<float> (maximumDelaySeconds * 1000.0), newDelayMs);
    delayInSamples.setTargetValue (static_cast<float> (targetDelayMs * 0.001 * sampleRate));
}

void FractionalDelay::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(4): para cada amostra, obtém o atraso (com smoothing) UMA vez e usa-o em todos os canais:
    //          push da entrada, saída = readLinear (atraso).
    juce::ignoreUnused (context);
}
