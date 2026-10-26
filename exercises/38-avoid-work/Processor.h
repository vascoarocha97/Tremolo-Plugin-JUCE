#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

/** Exercício 38 — Evitar trabalho.

    O código mais rápido é o que não corre. Três técnicas que todos os plugins sérios usam:
      1. recalcular coeficientes só quando os parâmetros mudam (e não a cada bloco);
      2. não deixar o processador cair em números desnormalizados (denormais), que podem ser
         dezenas de vezes mais lentos;
      3. quando a entrada é silêncio e o estado interno já é zero, não processar nada.

    Corre em Release (curriculum.json). */
class EfficientLowpass
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;

    /** Só guardam o valor pedido (podem ser chamadas a cada bloco, com o mesmo valor). */
    void setCutoff (float newCutoffHz) noexcept;
    void setResonance (float newQ) noexcept;

    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    // Para os testes
    int getCoefficientUpdates() const noexcept { return coefficientUpdates; }
    int getSkippedBlocks() const noexcept { return skippedBlocks; }
    /** O estado interno do biquad (2 valores por canal). */
    const std::vector<std::array<float, 2>>& getState() const noexcept { return state; }

    /** Abaixo disto (−160 dBFS) a entrada conta como silêncio. */
    static constexpr float silenceThreshold = 1.0e-8f;

private:
    void updateCoefficientsIfNeeded() noexcept;
    bool canSkip (const juce::dsp::AudioBlock<float>& block) const noexcept;

    double sampleRate = 44100.0;
    float cutoff = 1000.0f, q = 0.7071f;
    bool coefficientsNeedUpdate = true;

    std::array<float, 6> coefficients {}; // b0 b1 b2 a0 a1 a2 (a0 = 1)
    std::vector<std::array<float, 2>> state;

    int coefficientUpdates = 0;
    int skippedBlocks = 0;
};
