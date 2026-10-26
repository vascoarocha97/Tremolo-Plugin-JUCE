#include "Processor.h"

void Bitcrusher::prepare (const juce::dsp::ProcessSpec& spec)
{
    channels.resize (spec.numChannels);
    reset();
}

void Bitcrusher::reset() noexcept
{
    for (auto& c : channels)
        c = ChannelState {};
}

void Bitcrusher::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO: sample & hold com um acumulador de fase por canal (ChannelState):
    //   - quando state.phase >= 1: tira 1 à fase e apanha uma amostra nova, quantizada com o TEU
    //     Quantiser::quantise (exercício 04) para `bits` bits;
    //   - avança a fase 1/factor por amostra;
    //   - saída = (1 − mix)·original + mix·valor segurado.
    // A fase começa em 1 (ver ChannelState), por isso a primeira amostra é logo apanhada.
    juce::ignoreUnused (context);
}
