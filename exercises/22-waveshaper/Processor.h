#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 22 — Waveshaper.

    Distorção "sem memória": cada amostra de saída depende só da amostra de entrada,
    y = f(ganho · x). A forma da curva f decide que harmónicos aparecem.

    Os exercícios 23 (fuzz) e 26 (oversampling) usam estas funções. */
namespace Shapers
{
    enum class Shape
    {
        hardClip,     // corta em ±1
        softClip,     // cúbica: suave até ±1, depois plana
        tanh,         // tangente hiperbólica
        asymmetric    // satura mais cedo do lado negativo (gera harmónicos pares)
    };

    /** min(max(x, −1), 1) */
    float hardClip (float x) noexcept;

    /** 1.5·(x − x³/3) para |x| ≤ 1, e ±1 fora. Contínua e com derivada contínua em ±1. */
    float softClip (float x) noexcept;

    /** std::tanh (x) */
    float tanhClip (float x) noexcept;

    /** x ≥ 0: tanh(x);  x < 0: 0.5·tanh(2x).  Inclinação 1 em 0, satura em +1 e em −0.5. */
    float asymmetric (float x) noexcept;

    float apply (Shape shape, float x) noexcept;
}

/** Ganho de entrada (drive) -> curva -> ganho de saída (convenção juce::dsp). */
class Waveshaper
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept;

    /** Chamar na thread de áudio, entre blocos. */
    void setDriveDecibels (float db) noexcept;
    void setOutputDecibels (float db) noexcept;
    void setShape (Shapers::Shape newShape) noexcept { shape = newShape; }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> drive { 1.0f }, output { 1.0f };
    Shapers::Shape shape = Shapers::Shape::tanh;
    float driveTarget = 1.0f, outputTarget = 1.0f;
};
