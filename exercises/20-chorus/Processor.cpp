#include "Processor.h"

void Chorus::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    delayLine.setMaximumDelayInSamples (static_cast<int> (maxDelayMs * 0.001 * sampleRate) + 2);
    delayLine.prepare ({ spec.sampleRate, spec.maximumBlockSize, static_cast<juce::uint32> (maxChannels) });

    for (auto& lfo : lfos)
        lfo.prepare (sampleRate);

    lfos[1].setPhaseOffset (0.25f); // 90° entre L e R

    depthMs.reset (sampleRate, 0.05);
    centreMs.reset (sampleRate, 0.05);
    mix.reset (sampleRate, 0.02);
    reset();
}

void Chorus::reset() noexcept
{
    delayLine.reset();

    for (auto& lfo : lfos)
        lfo.reset();

    depthMs.setCurrentAndTargetValue (depthTarget);
    centreMs.setCurrentAndTargetValue (centreTarget);
    mix.setCurrentAndTargetValue (mixTarget);
}

void Chorus::setRate (float hz) noexcept
{
    for (auto& lfo : lfos)
        lfo.setFrequency (hz);
}

void Chorus::setDepthMs (float ms) noexcept
{
    depthTarget = juce::jlimit (0.0f, 20.0f, ms);
    depthMs.setTargetValue (depthTarget);
}

void Chorus::setCentreDelayMs (float ms) noexcept
{
    centreTarget = juce::jlimit (1.0f, maxDelayMs - 21.0f, ms);
    centreMs.setTargetValue (centreTarget);
}

void Chorus::setMix (float newMix) noexcept
{
    mixTarget = juce::jlimit (0.0f, 1.0f, newMix);
    mix.setTargetValue (mixTarget);
}

void Chorus::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO: para cada amostra avança os SmoothedValue (centro, profundidade, mistura) uma vez; depois,
    //       para cada canal (até 2):
    //         - atraso em AMOSTRAS = (centro + profundidade · LFO do canal) em ms, convertido;
    //           nunca menos de 1 amostra (lê-se antes de escrever: ver exercício 12);
    //         - guarda-o em lastDelay[canal] (os testes usam-no);
    //         - lê a cópia atrasada (popSample com esse atraso) e escreve a entrada (pushSample);
    //         - saída = (1 − mistura) · original + mistura · cópia.
    juce::ignoreUnused (context);
}
