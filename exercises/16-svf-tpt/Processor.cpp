#include "Processor.h"

#include <cmath>
#include <numbers>

void StateVariableFilter::setParameters (float cutoffHz, float q, double sampleRate) noexcept
{
    // TODO(1): limita fc a [10 Hz, 0.49·fs] e Q a >= 0.1, e calcula:
    //   g  = tan(π·fc/fs)          (pré-distorção do bilinear transform)
    //   R2 = 1/Q
    //   h  = 1 / (1 + R2·g + g²)
    juce::ignoreUnused (cutoffHz, q, sampleRate);
}

StateVariableFilter::Outputs StateVariableFilter::processSample (float x) noexcept
{
    // TODO(2): as três saídas, pela ordem HP -> BP -> LP (ver README e o diagrama do Zavalishin):
    //   HP = h · (x − s1·(g + R2) − s2)
    //   cada integrador trapezoidal, com entrada v e estado s:   saída = g·v + s;   s = g·v + saída
    //   BP é a saída do 1.º integrador (entrada HP, estado s1)
    //   LP é a saída do 2.º integrador (entrada BP, estado s2)
    juce::ignoreUnused (x);
    return {};
}

void SvfProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filters.resize (spec.numChannels);

    for (auto& f : filters)
        f.setParameters (cutoff, resonance, sampleRate);

    reset();
}

void SvfProcessor::reset() noexcept
{
    for (auto& f : filters)
        f.reset();

    lfoPhase = 0.0;
}

void SvfProcessor::setParameters (Mode newMode, float cutoffHz, float q, float modulationOctaves, float lfoHz) noexcept
{
    mode = newMode;
    cutoff = cutoffHz;
    resonance = q;
    octaves = modulationOctaves;
    lfoRate = lfoHz;
}

void SvfProcessor::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = juce::jmin (block.getNumChannels(), filters.size());
    const auto lfoIncrement = 2.0 * std::numbers::pi * lfoRate / sampleRate;

    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        // O corte muda em CADA amostra (oitavas acima/abaixo do corte base): é aqui que a TPT brilha.
        const auto modulatedCutoff = cutoff * std::exp2 (octaves * static_cast<float> (std::sin (lfoPhase)));
        lfoPhase = std::fmod (lfoPhase + lfoIncrement, 2.0 * std::numbers::pi);

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto& f = filters[channel];
            f.setParameters (modulatedCutoff, resonance, sampleRate);

            auto* samples = block.getChannelPointer (channel);
            const auto out = f.processSample (samples[n]);

            samples[n] = mode == Mode::lowPass ? out.lowPass : mode == Mode::bandPass ? out.bandPass : out.highPass;
        }
    }
}
