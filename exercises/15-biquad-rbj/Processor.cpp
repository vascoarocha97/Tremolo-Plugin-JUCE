#include "Processor.h"

#include <cmath>
#include <complex>
#include <numbers>

namespace
{
    // Variáveis intermédias do cookbook (w0, cos(w0), alpha, A).
    struct Intermediates
    {
        double w0, cosW0, alpha, A;
    };

    [[maybe_unused]] Intermediates intermediates (double sampleRate, double frequency, double q, double gainDb = 0.0)
    {
        // TODO(1): w0 = 2π·f0/Fs,  alpha = sin(w0)/(2Q),  A = 10^(dBgain/40)   (ver o cookbook)
        juce::ignoreUnused (sampleRate, frequency, q, gainDb);
        return { 0.0, 1.0, 0.0, 1.0 };
    }

    /** Divide tudo por a0 (o biquad assume a0 = 1). */
    [[maybe_unused]] BiquadCoefficients normalise (double b0, double b1, double b2, double a0, double a1, double a2)
    {
        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }
}

// TODO(2): as sete fórmulas do Audio EQ Cookbook. Cada uma: calcula as intermédias, escreve
//          b0, b1, b2, a0, a1, a2 como no cookbook e devolve normalise (...).
//          Dica de C++: const auto [w0, c, alpha, A] = intermediates (...); (structured bindings)

BiquadCoefficients Rbj::lowPass (double sampleRate, double frequency, double q)
{
    juce::ignoreUnused (sampleRate, frequency, q);
    return {};
}

BiquadCoefficients Rbj::highPass (double sampleRate, double frequency, double q)
{
    juce::ignoreUnused (sampleRate, frequency, q);
    return {};
}

BiquadCoefficients Rbj::bandPass (double sampleRate, double frequency, double q)
{
    juce::ignoreUnused (sampleRate, frequency, q);
    return {};
}

BiquadCoefficients Rbj::notch (double sampleRate, double frequency, double q)
{
    juce::ignoreUnused (sampleRate, frequency, q);
    return {};
}

BiquadCoefficients Rbj::peaking (double sampleRate, double frequency, double q, double gainDb)
{
    juce::ignoreUnused (sampleRate, frequency, q, gainDb);
    return {};
}

BiquadCoefficients Rbj::lowShelf (double sampleRate, double frequency, double q, double gainDb)
{
    juce::ignoreUnused (sampleRate, frequency, q, gainDb);
    return {};
}

BiquadCoefficients Rbj::highShelf (double sampleRate, double frequency, double q, double gainDb)
{
    juce::ignoreUnused (sampleRate, frequency, q, gainDb);
    return {};
}

double Rbj::magnitudeAt (const BiquadCoefficients& c, double frequency, double sampleRate)
{
    // TODO(3): |H(e^jω)| com ω = 2π·f/fs. Usa std::complex<double>: z⁻¹ = std::polar (1.0, -ω),
    //          z⁻² = std::polar (1.0, -2ω); avalia numerador e denominador e devolve std::abs (num / den).
    juce::ignoreUnused (c, frequency, sampleRate);
    return 1.0;
}

void Biquad::setCoefficients (const BiquadCoefficients& c) noexcept
{
    b0 = static_cast<float> (c.b0);
    b1 = static_cast<float> (c.b1);
    b2 = static_cast<float> (c.b2);
    a1 = static_cast<float> (c.a1);
    a2 = static_cast<float> (c.a2);
}

float Biquad::processSample (float x) noexcept
{
    // TODO(4): Transposed Direct Form II (ver o diagrama no README): calcula y a partir de x e s1,
    //          depois atualiza s1 e s2.
    return x;
}

void BiquadFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filters.resize (spec.numChannels);
    updateCoefficients();
    reset();
}

void BiquadFilter::reset() noexcept
{
    for (auto& f : filters)
        f.reset();
}

void BiquadFilter::setParameters (Type newType, float newFrequency, float newQ, float newGainDb) noexcept
{
    if (newType == type && newFrequency == frequency && newQ == q && newGainDb == gainDb)
        return; // nada mudou: não recalcula (cos, sin, pow são caros)

    type = newType;
    frequency = newFrequency;
    q = newQ;
    gainDb = newGainDb;
    updateCoefficients();
}

void BiquadFilter::updateCoefficients() noexcept
{
    const auto f = juce::jlimit (10.0, 0.49 * sampleRate, static_cast<double> (frequency));
    const auto qq = juce::jmax (0.05, static_cast<double> (q));

    switch (type)
    {
        case Type::lowPass:   coefficients = Rbj::lowPass (sampleRate, f, qq); break;
        case Type::highPass:  coefficients = Rbj::highPass (sampleRate, f, qq); break;
        case Type::bandPass:  coefficients = Rbj::bandPass (sampleRate, f, qq); break;
        case Type::notch:     coefficients = Rbj::notch (sampleRate, f, qq); break;
        case Type::peaking:   coefficients = Rbj::peaking (sampleRate, f, qq, gainDb); break;
        case Type::lowShelf:  coefficients = Rbj::lowShelf (sampleRate, f, qq, gainDb); break;
        case Type::highShelf: coefficients = Rbj::highShelf (sampleRate, f, qq, gainDb); break;
    }

    for (auto& filter : filters)
        filter.setCoefficients (coefficients);
}

void BiquadFilter::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = juce::jmin (block.getNumChannels(), filters.size());

    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* samples = block.getChannelPointer (channel);

        for (size_t n = 0; n < block.getNumSamples(); ++n)
            samples[n] = filters[channel].processSample (samples[n]);
    }
}
