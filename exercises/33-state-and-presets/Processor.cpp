#include "Processor.h"

void EchoState::save (juce::AudioProcessorValueTreeState& apvts, juce::MemoryBlock& destination)
{
    // TODO(1): apvts.copyState() (uma cópia thread-safe da ValueTree) -> createXml() ->
    //   juce::AudioProcessor::copyXmlToBinary (xml, destination).
    juce::ignoreUnused (apvts, destination);
}

bool EchoState::load (juce::AudioProcessorValueTreeState& apvts, const void* data, int sizeInBytes)
{
    // TODO(2): o inverso de save(), com cuidado:
    //   - dados nulos/vazios, ou que não sejam XML (getXmlFromBinary devolve nullptr): false;
    //   - XML cuja raiz não tem o tipo da ValueTree do APVTS (apvts.state.getType()): false;
    //   - só então: repõe todos os parâmetros no valor por omissão (ver README: porquê?) e
    //     apvts.replaceState (juce::ValueTree::fromXml (*xml)); devolve true.
    juce::ignoreUnused (apvts, data, sizeInBytes);
    return false;
}

const std::vector<Preset>& factoryPresets()
{
    // Criados uma vez, na primeira chamada (na message thread: o host pede os programas daí).
    static const std::vector<Preset> presets {
        { "Default", {} },
        { "Slapback", { { EchoParams::time, 110.0f }, { EchoParams::feedback, 0.1f }, { EchoParams::mix, 0.35f } } },
        { "Ping-pong largo", { { EchoParams::time, 420.0f }, { EchoParams::feedback, 0.55f }, { EchoParams::mix, 0.4f },
                               { EchoParams::mode, 1.0f } } },
        { "Fita escura", { { EchoParams::time, 600.0f }, { EchoParams::feedback, 0.7f }, { EchoParams::tone, 1500.0f },
                           { EchoParams::mix, 0.35f } } },
        { "Quase infinito", { { EchoParams::time, 800.0f }, { EchoParams::feedback, 0.95f }, { EchoParams::tone, 4000.0f },
                              { EchoParams::mix, 0.5f }, { EchoParams::gain, -6.0f } } },
    };

    return presets;
}

void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset)
{
    // TODO(3): todos os parâmetros ao valor por omissão (getDefaultValue() já vem em 0..1); depois,
    //   para cada (id, valor real) do preset: apvts.getParameter (id) e setValueNotifyingHost com o
    //   valor convertido para 0..1 (convertTo0to1). Ignora IDs que não existam.
    juce::ignoreUnused (apvts, preset);
}
