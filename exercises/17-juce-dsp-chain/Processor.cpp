#include "Processor.h"

void BandFilterChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // TODO(1): por esta ordem:
    //   - calcula os coeficientes (força coefficientsNeedUpdate e chama updateCoefficients());
    //   - configura o juce::dsp::Gain da cadeia (ganho atual e rampa de 20 ms) — lê o README sobre
    //     o valor inicial do Gain;
    //   - prepara a cadeia inteira com chain.prepare (spec).
}

void BandFilterChain::reset() noexcept
{
    chain.reset();
}

void BandFilterChain::setLowCut (float frequencyHz) noexcept
{
    if (frequencyHz != lowCut)
    {
        lowCut = frequencyHz;
        coefficientsNeedUpdate = true;
    }
}

void BandFilterChain::setHighCut (float frequencyHz) noexcept
{
    if (frequencyHz != highCut)
    {
        highCut = frequencyHz;
        coefficientsNeedUpdate = true;
    }
}

void BandFilterChain::setGainDecibels (float gainDb) noexcept
{
    gainDecibels = gainDb;
    chain.get<gainIndex>().setGainDecibels (gainDb);
}

void BandFilterChain::updateCoefficients() noexcept
{
    if (! coefficientsNeedUpdate)
        return;

    // TODO(2): atualiza os coeficientes dos dois filtros SEM alocar memória:
    //   - limita os cortes a [10 Hz, 0.49·fs];
    //   - o "state" de cada ProcessorDuplicator (chain.get<índice>().state) é um ponteiro para os
    //     Coefficients partilhados por todos os canais;
    //   - procura em juce::dsp::IIR a forma de obter os coeficientes de um passa-alto/passa-baixo
    //     como std::array (e não como um objeto novo no heap) e atribui-os a *state.
    //   No fim, coefficientsNeedUpdate = false.
}

void BandFilterChain::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(3): atualiza os coeficientes (se for preciso) e processa a cadeia.
    juce::ignoreUnused (context);
}

const juce::dsp::IIR::Coefficients<float>& BandFilterChain::getLowCutCoefficients() const noexcept
{
    return *chain.get<lowCutIndex>().state;
}

const juce::dsp::IIR::Coefficients<float>& BandFilterChain::getHighCutCoefficients() const noexcept
{
    return *chain.get<highCutIndex>().state;
}
