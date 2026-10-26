#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace learn
{

/** Gera (offline) um loop de 2 compassos a 100 BPM: bombo, tarola, hi-hat e uma linha
    de "guitarra" em Karplus-Strong. Tudo sintetizado, sem samples externos (sem licenças).

    Bom para ouvir delays e reverbs (transientes), distorção (cordas) e filtros (pratos). */
juce::AudioBuffer<float> renderDemoLoop (double sampleRate);

} // namespace learn
