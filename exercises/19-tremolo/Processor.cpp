#include "Processor.h"

void Tremolo::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (auto& lfo : lfos)
        lfo.prepare (sampleRate);

    depth.reset (sampleRate, 0.02);
    reset();
}

void Tremolo::reset() noexcept
{
    for (auto& lfo : lfos)
        lfo.reset();

    setStereoPhase (stereoPhaseDegrees);
    depth.setCurrentAndTargetValue (depthTarget);
}

void Tremolo::setRate (float hz) noexcept
{
    for (auto& lfo : lfos)
        lfo.setFrequency (hz);
}

void Tremolo::setDepth (float newDepth) noexcept
{
    depthTarget = juce::jlimit (0.0f, 1.0f, newDepth);
    depth.setTargetValue (depthTarget);
}

void Tremolo::setShape (Lfo::Shape shape) noexcept
{
    for (auto& lfo : lfos)
        lfo.setShape (shape);
}

void Tremolo::setStereoPhase (float degrees) noexcept
{
    stereoPhaseDegrees = juce::jlimit (0.0f, 180.0f, degrees);
    lfos[0].setPhaseOffset (0.0f);
    lfos[1].setPhaseOffset (stereoPhaseDegrees / 360.0f);
}

void Tremolo::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    // TODO: para cada amostra (avança o smoothing da profundidade UMA vez) e cada canal (até 2):
    //          ganho = 1 − profundidade · unipolar (LFO desse canal)
    //       e multiplica a amostra pelo ganho.
    //
    //       Se o bloco for mono, avança também o LFO do canal direito, para os dois continuarem
    //       com o desfasamento certo quando voltar a haver dois canais.
    juce::ignoreUnused (context);
}
