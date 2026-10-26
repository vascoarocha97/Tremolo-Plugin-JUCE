#include "Processor.h"

void OversampledShaper::prepare (const juce::dsp::ProcessSpec& spec)
{
    // TODO(1):
    //   - cria o juce::dsp::Oversampling<float> (std::make_unique) com spec.numChannels canais, o fator
    //     factorLog2, filtros filterHalfBandPolyphaseIIR, máxima qualidade, sem latência inteira;
    //   - initProcessing (spec.maximumBlockSize): reserva os buffers internos AQUI;
    //   - o smoothing do drive corre ao sample rate INTERNO (spec.sampleRate · fator), rampa de 20 ms;
    //   - reset().
    juce::ignoreUnused (spec);
}

void OversampledShaper::reset() noexcept
{
    if (oversampling != nullptr)
        oversampling->reset();

    drive.setCurrentAndTargetValue (driveTarget);
}

void OversampledShaper::setDriveDecibels (float db) noexcept
{
    driveTarget = juce::Decibels::decibelsToGain (db);
    drive.setTargetValue (driveTarget);
}

float OversampledShaper::getLatencyInSamples() const noexcept
{
    return oversampling != nullptr ? oversampling->getLatencyInSamples() : 0.0f;
}

void OversampledShaper::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO(2): os três passos (ver README):
    //   1. auto upsampled = oversampling->processSamplesUp (bloco)  — um AudioBlock N vezes maior;
    //   2. aplica a curva (Shapers::apply com o drive suavizado) a todas as amostras de `upsampled`;
    //   3. oversampling->processSamplesDown (bloco)                  — filtra e escreve no bloco original.
    juce::ignoreUnused (context);
}
