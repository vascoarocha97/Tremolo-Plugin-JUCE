#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Exercício 01 — O primeiro buffer.

    Um processador mínimo com a mesma "forma" de um juce::AudioProcessor:
      prepareToPlay()    -> chamado antes de o áudio começar (aqui podes alocar memória)
      processBlock()     -> chamado na thread de áudio, bloco a bloco
      releaseResources() -> chamado quando o áudio pára

    Lê a lição em README.md antes de começar. */
class FirstBufferProcessor
{
public:
    /** O ganho fixo aplicado por processBlock(): 0.5 ≈ -6 dB. */
    static constexpr float gain = 0.5f;

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();

    /** Multiplica todas as amostras de todos os canais do bloco por `gain`. */
    void processBlock (juce::AudioBuffer<float>& buffer);

    /** Devolve o maior valor absoluto |x| encontrado em todos os canais do buffer. */
    static float findPeak (const juce::AudioBuffer<float>& buffer);
};
