#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <vector>

namespace learn::test
{

/** Processadores com a interface do juce::AudioProcessor: processBlock (AudioBuffer&). */
template <typename P>
concept BufferProcessor = requires (P& p, juce::AudioBuffer<float>& b) { p.processBlock (b); };

/** Processadores com a convenção juce::dsp: process (ProcessContextReplacing<float>). */
template <typename P>
concept DspProcessor = requires (P& p, const juce::dsp::ProcessContextReplacing<float>& c) { p.process (c); };

/** Processa as amostras [start, start + num) do buffer, sem copiar nem alocar.

    Assim os testes simulam o que um host faz: chamar o processador muitas vezes com blocos
    consecutivos de tamanhos diferentes. */
template <typename Processor>
void processRange (Processor& processor, juce::AudioBuffer<float>& buffer, int start, int num)
{
    if constexpr (DspProcessor<Processor>)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto subBlock = block.getSubBlock (static_cast<size_t> (start), static_cast<size_t> (num));
        juce::dsp::ProcessContextReplacing<float> context (subBlock);
        processor.process (context);
    }
    else
    {
        static_assert (BufferProcessor<Processor>, "O processador precisa de processBlock() ou process()");

        // Este construtor do AudioBuffer *referencia* os dados existentes (não copia, não aloca
        // para até 32 canais).
        juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), buffer.getNumChannels(),
                                       start, num);
        processor.processBlock (view);
    }
}

/** Processa o buffer inteiro num único bloco. */
template <typename Processor>
void processWhole (Processor& processor, juce::AudioBuffer<float>& buffer)
{
    processRange (processor, buffer, 0, buffer.getNumSamples());
}

/** Processa o buffer em blocos consecutivos, percorrendo `blockSizes` em ciclo. */
template <typename Processor>
void processInBlocks (Processor& processor, juce::AudioBuffer<float>& buffer,
                      const std::vector<int>& blockSizes)
{
    jassert (! blockSizes.empty());

    int position = 0;
    size_t index = 0;

    while (position < buffer.getNumSamples())
    {
        const auto num = std::min (blockSizes[index++ % blockSizes.size()],
                                   buffer.getNumSamples() - position);
        processRange (processor, buffer, position, num);
        position += num;
    }
}

/** Tamanhos de bloco "desagradáveis" (todos ≤ 512): 1 amostra, primos, potências de 2... */
inline std::vector<int> awkwardBlockSizes()
{
    return { 1, 7, 64, 13, 512, 100, 2, 333, 256, 31 };
}

} // namespace learn::test
