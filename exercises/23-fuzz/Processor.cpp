#include "Processor.h"

#include <cmath>
#include <numbers>

void DcBlocker::prepare (double sampleRate, float cutoffHz) noexcept
{
    // TODO(1): R = 1 − 2π·fc/fs, e reset().
    juce::ignoreUnused (sampleRate, cutoffHz);
    reset();
}

float DcBlocker::processSample (float x) noexcept
{
    // TODO(2): y[n] = x[n] − x[n−1] + R·y[n−1]  (e atualiza x1 e y1)
    return x;
}

float Fuzz::toneToFrequency (float t) noexcept
{
    return 500.0f * std::pow (20.0f, juce::jlimit (0.0f, 1.0f, t)); // 500 Hz .. 10 kHz
}

void Fuzz::prepare (const juce::dsp::ProcessSpec& spec)
{
    inputHighPass.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
    inputHighPass.setCutoffFrequency (100.0f);
    inputHighPass.prepare (spec);

    toneLowPass.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
    toneLowPass.setCutoffFrequency (toneToFrequency (tone));
    toneLowPass.prepare (spec);

    dcBlockers.resize (spec.numChannels);

    for (auto& d : dcBlockers)
        d.prepare (spec.sampleRate);

    gain.reset (spec.sampleRate, 0.02);
    output.reset (spec.sampleRate, 0.02);
    reset();
}

void Fuzz::reset() noexcept
{
    inputHighPass.reset();
    toneLowPass.reset();

    for (auto& d : dcBlockers)
        d.reset();

    gain.setCurrentAndTargetValue (gainTarget);
    output.setCurrentAndTargetValue (outputTarget);
}

void Fuzz::setGainDecibels (float db) noexcept
{
    gainTarget = juce::Decibels::decibelsToGain (juce::jlimit (0.0f, 50.0f, db));
    gain.setTargetValue (gainTarget);
}

void Fuzz::setTone (float newTone) noexcept
{
    tone = juce::jlimit (0.0f, 1.0f, newTone);
    toneLowPass.setCutoffFrequency (toneToFrequency (tone));
}

void Fuzz::setOutputDecibels (float db) noexcept
{
    outputTarget = juce::Decibels::decibelsToGain (db);
    output.setTargetValue (outputTarget);
}

void Fuzz::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(3): a cadeia, por esta ordem (ver README):
    //   1. inputHighPass.process (context)                      — tira os graves antes de distorcer
    //   2. por amostra: ganho (SmoothedValue) -> Shapers::asymmetric (exercício 22) -> DC blocker do canal
    //   3. toneLowPass.process (context)                         — o tom
    //   4. por amostra: volume de saída (SmoothedValue)
    juce::ignoreUnused (context);
}
