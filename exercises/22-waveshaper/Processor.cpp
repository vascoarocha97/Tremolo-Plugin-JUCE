#include "Processor.h"

#include <cmath>

float Shapers::hardClip (float x) noexcept
{
    // TODO(1)
    return x;
}

float Shapers::softClip (float x) noexcept
{
    // TODO(2): 1.5·(x − x³/3) dentro de [−1, 1]; ±1 fora.
    return x;
}

float Shapers::tanhClip (float x) noexcept
{
    // TODO(3)
    return x;
}

float Shapers::asymmetric (float x) noexcept
{
    // TODO(4): tanh(x) para x ≥ 0; 0.5·tanh(2x) para x < 0.
    return x;
}

float Shapers::apply (Shape shape, float x) noexcept
{
    switch (shape)
    {
        case Shape::hardClip:   return hardClip (x);
        case Shape::softClip:   return softClip (x);
        case Shape::tanh:       return tanhClip (x);
        case Shape::asymmetric: return asymmetric (x);
    }

    return x;
}

void Waveshaper::prepare (const juce::dsp::ProcessSpec& spec)
{
    drive.reset (spec.sampleRate, 0.02);
    output.reset (spec.sampleRate, 0.02);
    reset();
}

void Waveshaper::reset() noexcept
{
    drive.setCurrentAndTargetValue (driveTarget);
    output.setCurrentAndTargetValue (outputTarget);
}

void Waveshaper::setDriveDecibels (float db) noexcept
{
    driveTarget = juce::Decibels::decibelsToGain (db);
    drive.setTargetValue (driveTarget);
}

void Waveshaper::setOutputDecibels (float db) noexcept
{
    outputTarget = juce::Decibels::decibelsToGain (db);
    output.setTargetValue (outputTarget);
}

void Waveshaper::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(5): para cada amostra avança os dois SmoothedValue (drive e saída) uma vez; para cada canal:
    //          amostra = saída · curva (drive · amostra). Shapers::apply escolhe a curva.
    juce::ignoreUnused (context);
}
