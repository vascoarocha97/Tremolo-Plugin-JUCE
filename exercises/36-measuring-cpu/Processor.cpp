#include "Processor.h"

#include <algorithm>
#include <cmath>

//==============================================================================
void CpuLoadMeter::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    reset();
}

void CpuLoadMeter::reset() noexcept
{
    hasMeasurement = false;
    load = 0.0f;
    peak = 0.0f;
    overruns = 0;
}

void CpuLoadMeter::addMeasurement (double secondsTaken, int numSamples) noexcept
{
    // TODO(1): (ver o comentário no Processor.h)
    //   - ignora blocos com numSamples <= 0;
    //   - orçamento = numSamples / sampleRate (segundos); carga deste bloco = secondsTaken / orçamento;
    //   - a 1.ª medição (hasMeasurement == false) define `load`; as seguintes: load += 0.1 · (nova − load);
    //   - peak = máximo das cargas; overruns conta os blocos com carga > 1.
    juce::ignoreUnused (secondsTaken, numSamples);
}

CpuLoadMeter::ScopedMeasurement::ScopedMeasurement (CpuLoadMeter& meterToUse, int numSamplesInBlock) noexcept
    : meter (meterToUse), numSamples (numSamplesInBlock)
{
    // TODO(2): guarda o instante de início em startTicks (juce::Time::getHighResolutionTicks()).
}

CpuLoadMeter::ScopedMeasurement::~ScopedMeasurement() noexcept
{
    // TODO(2): tempo decorrido em ticks -> segundos (juce::Time::highResolutionTicksToSeconds) e
    //   meter.addMeasurement (segundos, numSamples).
}

//==============================================================================
void NaiveDriveFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    state.assign (spec.numChannels, 0.0f);
}

void NaiveDriveFilter::reset() noexcept
{
    std::fill (state.begin(), state.end(), 0.0f);
}

void NaiveDriveFilter::process (const juce::dsp::ProcessContextReplacing<float>& context,
                                const DriveFilterSettings& settings) noexcept
{
    auto& block = context.getOutputBlock();
    const auto numChannels = std::min (block.getNumChannels(), state.size());

    // Amostra a amostra, canal a canal, recalculando tudo sempre. Funciona... devagar.
    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const auto drive = static_cast<float> (std::pow (10.0, settings.driveDb / 20.0));
            const auto a = static_cast<float> (std::exp (-juce::MathConstants<double>::twoPi * settings.cutoffHz / sampleRate));
            const auto gain = static_cast<float> (std::pow (10.0, settings.gainDb / 20.0));

            const auto x = block.getSample (static_cast<int> (ch), static_cast<int> (n)) * drive;
            state[ch] = (1.0f - a) * x + a * state[ch];
            block.setSample (static_cast<int> (ch), static_cast<int> (n), softClip (state[ch]) * gain);
        }
    }
}

//==============================================================================
void FastDriveFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    state.assign (spec.numChannels, 0.0f);
}

void FastDriveFilter::reset() noexcept
{
    std::fill (state.begin(), state.end(), 0.0f);
}

void FastDriveFilter::process (const juce::dsp::ProcessContextReplacing<float>& context,
                               const DriveFilterSettings& settings) noexcept
{
    // TODO(3): isto é uma cópia da versão ingénua: dá o resultado certo mas é lenta. Torna-a pelo
    //   menos 3× mais rápida SEM mudar o resultado. Mede antes de mudar (./learn check 36 mostra os
    //   tempos) e pergunta-te, linha a linha: "isto precisa MESMO de ser feito para cada amostra?"
    auto& block = context.getOutputBlock();
    const auto numChannels = std::min (block.getNumChannels(), state.size());

    for (size_t n = 0; n < block.getNumSamples(); ++n)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const auto drive = static_cast<float> (std::pow (10.0, settings.driveDb / 20.0));
            const auto a = static_cast<float> (std::exp (-juce::MathConstants<double>::twoPi * settings.cutoffHz / sampleRate));
            const auto gain = static_cast<float> (std::pow (10.0, settings.gainDb / 20.0));

            const auto x = block.getSample (static_cast<int> (ch), static_cast<int> (n)) * drive;
            state[ch] = (1.0f - a) * x + a * state[ch];
            block.setSample (static_cast<int> (ch), static_cast<int> (n), softClip (state[ch]) * gain);
        }
    }
}
