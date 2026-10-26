// Ponto de entrada do plugin: o wrapper VST3/Standalone do JUCE chama esta função para criar o
// processador. Não precisas de editar este ficheiro.

#include "Processor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PedalProcessor();
}
