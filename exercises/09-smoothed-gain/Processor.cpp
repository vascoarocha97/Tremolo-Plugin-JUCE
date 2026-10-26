#include "Processor.h"

float SmoothedGain::toLinear (float gainDb) noexcept
{
    return gainDb <= silenceDb ? 0.0f : juce::Decibels::decibelsToGain (gainDb);
}

void SmoothedGain::prepare (const juce::dsp::ProcessSpec& spec)
{
    // TODO(1): configura a duração da rampa do SmoothedValue (spec.sampleRate, rampLengthSeconds)
    //          e faz reset().
    juce::ignoreUnused (spec);
}

void SmoothedGain::reset() noexcept
{
    // TODO(2): salta (sem rampa) para o ganho guardado em targetGainDb.
}

void SmoothedGain::setGainDecibels (float newGainDb) noexcept
{
    // TODO(3): chamado de outra thread: guarda o valor de forma thread-safe.
    juce::ignoreUnused (newGainDb);
}

float SmoothedGain::getGainDecibels() const noexcept
{
    return targetGainDb.load();
}

void SmoothedGain::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(4):
    //  - lê o ganho alvo UMA vez por bloco e passa-o ao SmoothedValue (setTargetValue);
    //  - se context.isBypassed: não mexas no áudio, mas deixa o tempo do smoothing avançar (skip);
    //  - senão: para cada amostra, um getNextValue() aplicado a todos os canais.
    //
    // O bloco é context.getOutputBlock(): getNumChannels(), getNumSamples() e
    // getChannelPointer (canal) (devolve float*). Repara que os tamanhos são size_t.
    juce::ignoreUnused (context);
}
