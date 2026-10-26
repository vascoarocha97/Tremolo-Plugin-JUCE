#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../22-waveshaper/Processor.h" // as TUAS curvas do exercício 22

#include <memory>

/** Exercício 26 — Oversampling.

    A distorção cria harmónicos acima de Nyquist, que dobram para baixo (aliasing). A solução:
      1. subir o sample rate (upsample + filtro), 2. distorcer lá em cima, onde há espaço para os
      harmónicos, 3. filtrar tudo o que passa do Nyquist original e voltar a descer.
    O juce::dsp::Oversampling faz os passos 1 e 3. */
class OversampledShaper
{
public:
    /** factorLog2: 0 = sem oversampling, 1 = 2×, 2 = 4×, 3 = 8×. Aplica-se no próximo prepare(). */
    void setOversamplingFactorLog2 (int newFactorLog2) noexcept { factorLog2 = juce::jlimit (0, 4, newFactorLog2); }

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setDriveDecibels (float db) noexcept;
    void setShape (Shapers::Shape newShape) noexcept { shape = newShape; }

    /** Atraso (em amostras, ao sample rate original) introduzido pelos filtros do oversampling.
        Um plugin tem de o comunicar ao host (setLatencySamples) para o host compensar. */
    float getLatencyInSamples() const noexcept;

    int getOversamplingFactor() const noexcept { return 1 << factorLog2; }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> drive { 1.0f };
    Shapers::Shape shape = Shapers::Shape::hardClip;
    int factorLog2 = 2;
    float driveTarget = 1.0f;
};
