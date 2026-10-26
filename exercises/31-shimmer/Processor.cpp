#include "Processor.h"

#include <algorithm>
#include <cmath>

//==============================================================================
void PitchShifter::prepare (double sampleRate, float windowMs)
{
    windowSamples = windowMs * 0.001f * static_cast<float> (sampleRate);
    // + margem para o atraso mínimo de 1 amostra e para a interpolação
    buffer.assign (static_cast<size_t> (windowSamples) + 4, 0.0f);
    reset();
}

void PitchShifter::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
    phase = 0.0f;
}

void PitchShifter::setSemitones (float semitones) noexcept
{
    // TODO(1): razão de frequências para `semitones` (12 semitons = uma oitava = ×2).
    juce::ignoreUnused (semitones);
}

float PitchShifter::crossfadeGain (float p) noexcept
{
    // TODO(2): sin²(π·p)
    juce::ignoreUnused (p);
    return 0.0f;
}

float PitchShifter::readDelayed (float delaySamples) const noexcept
{
    // interpolação linear, como no exercício 11
    const auto size = static_cast<int> (buffer.size());
    auto readPos = static_cast<float> (writeIndex) - delaySamples;

    if (readPos < 0.0f)
        readPos += static_cast<float> (size);

    const auto i0 = static_cast<int> (readPos);
    const auto frac = readPos - static_cast<float> (i0);
    const auto i1 = (i0 + 1) % size;

    return buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);
}

float PitchShifter::processSample (float input) noexcept
{
    buffer[(size_t) writeIndex] = input;
    float out = 0.0f;

    // TODO(3): duas leituras desfasadas de meia janela (ver o diagrama no README):
    //   - fase da 2.ª leitura = phase + 0.5, de volta a [0, 1);
    //   - atraso de cada leitura = 1 + fase · windowSamples (usa readDelayed);
    //   - out = soma das duas leituras, cada uma pesada por crossfadeGain (a sua fase);
    //   - a fase avança (1 − ratio) / windowSamples por amostra (pensa: se a leitura anda à velocidade
    //     `ratio` e a escrita à velocidade 1, quanto muda o atraso por amostra?); mantém-na em [0, 1)
    //     — atenção que, para ratio > 1, ela anda para TRÁS.

    if (++writeIndex >= static_cast<int> (buffer.size()))
        writeIndex = 0;

    return out;
}

//==============================================================================
void ShimmerReverb::prepare (const juce::dsp::ProcessSpec& spec)
{
    fdn.prepare (spec.sampleRate);
    shifter.prepare (spec.sampleRate);

    // passa-baixo de um polo a ~6 kHz no ciclo (exercício 12): cada oitava acima perde mais,
    // por isso a subida acaba por se apagar em vez de ir até Nyquist.
    lowpassCoeff = std::exp (-juce::MathConstants<float>::twoPi * 6000.0f / static_cast<float> (spec.sampleRate));
    reset();
}

void ShimmerReverb::reset() noexcept
{
    fdn.reset();
    shifter.reset();
    lastWet = 0.0f;
    lowpassState = 0.0f;
}

void ShimmerReverb::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& block = context.getOutputBlock();

    if (block.getNumChannels() == 0)
        return;

    auto* left = block.getChannelPointer (0);
    auto* right = block.getNumChannels() > 1 ? block.getChannelPointer (1) : nullptr;

    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        const auto dryL = left[n];
        const auto dryR = right != nullptr ? right[n] : dryL;

        // TODO(4): o ciclo do shimmer (ver o diagrama no README):
        //   1. passa `lastWet` (a saída da reverb na amostra anterior) pelo `shifter`;
        //   2. filtra com o passa-baixo de um polo: lowpassState = x + lowpassCoeff · (lowpassState − x);
        //   3. feedback = shimmer · tanh (lowpassState)   — o tanh garante que o ciclo nunca explode;
        //   4. a FDN recebe a entrada mono (média de L e R) + feedback;
        //   5. guarda em lastWet a média das duas saídas da FDN.
        float wetL = 0.0f, wetR = 0.0f;

        left[n] = (1.0f - mix) * dryL + mix * wetL;

        if (right != nullptr)
            right[n] = (1.0f - mix) * dryR + mix * wetR;
    }
}
