#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 39 — Glitch (stutter / beat-repeat).

    Ao ligar, o efeito "congela" os últimos `slice` milissegundos e repete-os em ciclo até ser
    desligado. Os três sítios onde nascem cliques — a junção de cada repetição, o momento em que liga
    e o momento em que desliga — são tratados com crossfades. */
class Glitch
{
public:
    static constexpr float maxSliceMs = 1000.0f;
    static constexpr float maxFadeMs = 20.0f;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;

    /** Comprimento da fatia repetida. Só conta no momento em que o glitch liga. */
    void setSliceMs (float ms) noexcept { sliceMs = juce::jlimit (5.0f, maxSliceMs, ms); }

    /** Duração dos crossfades (junção e liga/desliga). */
    void setFadeMs (float ms) noexcept { fadeMs = juce::jlimit (0.1f, maxFadeMs, ms); }

    /** Liga/desliga. Ao ligar, a fatia é capturada no início do próximo process(). */
    void setActive (bool shouldBeActive) noexcept { active = shouldBeActive; }

    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    // Para os testes
    bool isRepeating() const noexcept { return repeating; }
    int getSliceLengthSamples() const noexcept { return sliceLength; }
    int getFadeLengthSamples() const noexcept { return fadeLength; }

private:
    void captureSlice() noexcept;
    float readLoop (int channel) const noexcept;

    double sampleRate = 44100.0;
    float sliceMs = 125.0f, fadeMs = 4.0f;
    bool active = false, repeating = false;

    // Histórico: o que entrou recentemente (buffer circular), sempre a gravar.
    juce::AudioBuffer<float> history;
    int writePos = 0;

    // A fatia capturada: `fadeLength` amostras de "pré-rolo" seguidas de `sliceLength` amostras.
    juce::AudioBuffer<float> slice;
    int sliceLength = 0, fadeLength = 0, readPos = 0;

    // 0 = seco, 1 = repetição. Suavizado: liga e desliga com crossfade.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> loopGain;
};
