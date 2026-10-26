#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 17 — Cadeias juce::dsp.

    Um "filtro de banda" como num channel strip: corte de graves (passa-alto) + corte de agudos
    (passa-baixo) + ganho, montado com as peças do JUCE:
      - juce::dsp::IIR::Filter (um canal) duplicado para N canais com juce::dsp::ProcessorDuplicator;
      - juce::dsp::Gain;
      - tudo encadeado num juce::dsp::ProcessorChain.

    Os parâmetros podem mudar em cada bloco: os coeficientes são atualizados sem alocar memória. */
class BandFilterChain
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setLowCut (float frequencyHz) noexcept;
    void setHighCut (float frequencyHz) noexcept;
    void setGainDecibels (float gainDb) noexcept;

    /** Os coeficientes atuais (para os testes). */
    const juce::dsp::IIR::Coefficients<float>& getLowCutCoefficients() const noexcept;
    const juce::dsp::IIR::Coefficients<float>& getHighCutCoefficients() const noexcept;

private:
    void updateCoefficients() noexcept;

    using MultiChannelIIR = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                           juce::dsp::IIR::Coefficients<float>>;

    enum ChainIndex
    {
        lowCutIndex,
        highCutIndex,
        gainIndex
    };

    juce::dsp::ProcessorChain<MultiChannelIIR, MultiChannelIIR, juce::dsp::Gain<float>> chain;

    double sampleRate = 44100.0;
    float lowCut = 80.0f, highCut = 12000.0f, gainDecibels = 0.0f;
    bool coefficientsNeedUpdate = true;
};
