#pragma once

#include "../32-apvts-parameters/Processor.h" // o TEU EchoProcessor do exercício 32

#include <utility>
#include <vector>

/** Exercício 33 — Estado e presets.

    O host guarda o estado do plugin no projeto (getStateInformation) e devolve-o ao abrir o projeto
    (setStateInformation). São só bytes: o formato é decisão nossa. Com o APVTS, o estado natural é
    a ValueTree dos parâmetros, guardada como XML. */
namespace EchoState
{
    /** Escreve o estado atual dos parâmetros em `destination` (XML em binário). */
    void save (juce::AudioProcessorValueTreeState& apvts, juce::MemoryBlock& destination);

    /** Lê um estado guardado por save(). Devolve false (sem mexer em nada) se os dados não forem
        um estado válido deste plugin. Parâmetros que não constem do estado (p. ex. estado gravado
        por uma versão antiga do plugin) voltam ao valor por omissão. */
    bool load (juce::AudioProcessorValueTreeState& apvts, const void* data, int sizeInBytes);
}

/** Um preset: um nome e os valores REAIS (não normalizados) de alguns parâmetros. Os parâmetros que
    não aparecem ficam no valor por omissão. */
struct Preset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};

/** Os presets de fábrica. */
const std::vector<Preset>& factoryPresets();

/** Aplica um preset como se fosse o host a mexer nos parâmetros (o host e a GUI são notificados). */
void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset);

//==============================================================================
/** O EchoProcessor do 32 com estado e programas (presets). Já está feito: liga as funções acima aos
    métodos do juce::AudioProcessor que o host chama. */
class StatefulEchoProcessor : public EchoProcessor
{
public:
    void getStateInformation (juce::MemoryBlock& destination) override { EchoState::save (apvts, destination); }
    void setStateInformation (const void* data, int size) override    { EchoState::load (apvts, data, size); }

    int getNumPrograms() override { return static_cast<int> (factoryPresets().size()); }
    int getCurrentProgram() override { return currentProgram; }

    void setCurrentProgram (int index) override
    {
        if (juce::isPositiveAndBelow (index, getNumPrograms()))
        {
            currentProgram = index;
            applyPreset (apvts, factoryPresets()[(size_t) index]);
        }
    }

    const juce::String getProgramName (int index) override
    {
        return juce::isPositiveAndBelow (index, getNumPrograms()) ? factoryPresets()[(size_t) index].name : juce::String();
    }

private:
    int currentProgram = 0;
};
