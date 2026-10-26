#include "Processor.h"

#include <cmath>

void Glitch::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    const auto numChannels = static_cast<int> (spec.numChannels);
    const auto maxSlice = static_cast<int> (std::ceil (maxSliceMs * 0.001 * sampleRate));
    const auto maxFade = static_cast<int> (std::ceil (maxFadeMs * 0.001 * sampleRate));

    // Histórico com espaço para fatia + pré-rolo + um bloco (o bloco atual já foi escrito)
    history.setSize (numChannels, maxSlice + maxFade + static_cast<int> (spec.maximumBlockSize) + 1);
    slice.setSize (numChannels, maxSlice + maxFade);
    reset();
}

void Glitch::reset() noexcept
{
    history.clear();
    slice.clear();
    writePos = 0;
    readPos = 0;
    repeating = false;
    loopGain.reset (sampleRate, fadeMs * 0.001);
    loopGain.setCurrentAndTargetValue (0.0f);
}

void Glitch::captureSlice() noexcept
{
    sliceLength = juce::jmax (1, juce::roundToInt (sliceMs * 0.001 * sampleRate));
    fadeLength = juce::jlimit (1, sliceLength, juce::roundToInt (fadeMs * 0.001 * sampleRate));

    // TODO(1): copia para `slice` (em cada canal) as últimas fadeLength + sliceLength amostras do
    //   histórico, das mais antigas para as mais recentes: [pré-rolo][fatia].
    //   `writePos` é a posição onde vai ser escrita a PRÓXIMA amostra; o histórico é circular
    //   (cuidado com índices negativos: soma o tamanho antes do %).

    readPos = 0;
}

float Glitch::readLoop (int channel) const noexcept
{
    // Posição p na fatia: slice[fadeLength + p]. Nas últimas fadeLength amostras de cada repetição,
    // cruza com o pré-rolo, que desemboca exatamente em slice[fadeLength + 0] (o início da fatia).
    // TODO(2): a amostra da fatia na posição readPos é slice[fadeLength + readPos]. Nas últimas
    //   fadeLength posições de cada repetição, faz um crossfade linear dessa amostra com o pré-rolo
    //   (slice[0 .. fadeLength-1]), de forma a que, quando readPos volta a 0, o som continue sem
    //   degrau. Peso do pré-rolo na k-ésima amostra da junção: (k + 1) / (fadeLength + 1).
    juce::ignoreUnused (channel);
    return 0.0f;
}

void Glitch::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = juce::jmin (static_cast<int> (block.getNumChannels()), history.getNumChannels());
    const auto numSamples = static_cast<int> (block.getNumSamples());
    const auto size = history.getNumSamples();

    if (numChannels == 0 || size == 0)
        return;

    // TODO(3): transições sem clique (ver README):
    //   - ao ligar (active && ! repeating): captura a fatia, repeating = true, e prepara loopGain
    //     para uma rampa de 0 até 1 com a duração fadeMs (reset + setCurrentAndTargetValue + setTargetValue);
    //   - ao desligar (! active, ainda repeating, a rampa ainda a subir ou em 1): rampa até 0;
    //   - ao religar DURANTE a rampa de descida (active, repeating, alvo em 0): rampa de volta até 1
    //     (senão o efeito fica "ligado" mas mudo).
    //   (No fim do process, quando a rampa de descida acaba, repeating passa a false — já feito.)

    for (int n = 0; n < numSamples; ++n)
    {
        const auto g = loopGain.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = block.getChannelPointer (static_cast<size_t> (ch));
            const auto dry = samples[n];
            history.setSample (ch, writePos, dry); // o histórico grava sempre a entrada

            if (repeating)
                samples[n] = (1.0f - g) * dry + g * readLoop (ch);
        }

        if (++writePos >= size)
            writePos = 0;

        if (repeating && ++readPos >= sliceLength)
            readPos = 0;
    }

    if (repeating && ! active && ! loopGain.isSmoothing())
        repeating = false;
}
