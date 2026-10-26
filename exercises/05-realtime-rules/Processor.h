#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <mutex>
#include <vector>

/** Exercício 05 — Regras de tempo-real.

    Um medidor de pico: deixa o áudio passar inalterado e mede o pico de cada canal em cada bloco.
    A GUI (outra thread) lê:
      - getPeak (ch): o pico do último bloco, para desenhar o medidor;
      - readHistory(): os picos de cada bloco ainda não lidos, para desenhar um gráfico.

    Há duas threads a mexer nos mesmos dados: a thread de áudio escreve, a GUI lê.

    ⚠ Esta implementação FUNCIONA, mas quebra as regras de tempo-real. O teu trabalho é corrigi-la
      sem mudar a interface pública (ver README). Podes mudar tudo o que está em `private`. */
class PeakMeterProcessor
{
public:
    static constexpr int maxChannels = 8;
    static constexpr int historyCapacity = 1024;

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();

    /** Thread de áudio. Não altera o áudio. */
    void processBlock (juce::AudioBuffer<float>& buffer);

    /** Qualquer thread. Pico (|x| máximo) do último bloco no canal dado; 0 se o canal não existir. */
    float getPeak (int channel) const noexcept;

    /** Thread da GUI (um único leitor). Copia para `destination` até `maxValues` picos de bloco
        (o máximo entre canais de cada bloco), do mais antigo para o mais recente, e retira-os.
        Devolve quantos copiou. Se a GUI não ler, guardam-se no máximo `historyCapacity` valores. */
    int readHistory (float* destination, int maxValues) noexcept;

private:
    // TODO: estes três membros são o problema. Ver README e os TODO em Processor.cpp.
    mutable std::mutex mutex;
    std::vector<float> peaks;
    std::vector<float> history;
};
