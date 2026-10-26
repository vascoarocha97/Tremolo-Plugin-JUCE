#include "Processor.h"

#include <cmath>
#include <numbers>

StereoGains Pan::computeGains (float pan, PanLaw law) noexcept
{
    const auto p = juce::jlimit (-1.0f, 1.0f, pan);

    switch (law)
    {
        case PanLaw::linear:
            // TODO(1): L e R variam em linha reta entre 0 e 1, com L + R = 1.
            return { 1.0f, 1.0f };

        case PanLaw::constantPower:
            // TODO(2): pensa em p como um ângulo entre 0 (só esquerda) e π/2 (só direita):
            //          L² + R² = 1 para qualquer ângulo (que par de funções tem esta propriedade?).
            //          Os extremos têm de dar exatamente 0 e 1 (ver "Armadilhas" no README).
            return { 1.0f, 1.0f };

        case PanLaw::compromise4p5dB:
            // TODO(3): os ganhos da lei de potência constante elevados a 1.5.
            return { 1.0f, 1.0f };
    }

    juce::ignoreUnused (p);
    return { 1.0f, 1.0f };
}

void StereoPanner::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    juce::ignoreUnused (sampleRate, maximumExpectedSamplesPerBlock);
}

void StereoPanner::processBlock (juce::AudioBuffer<float>& buffer)
{
    // TODO(4): se houver pelo menos 2 canais, para cada amostra:
    //            mono = média de L e R;  L = ganhoL · mono;  R = ganhoR · mono.
    //          Calcula os ganhos uma vez por bloco, não por amostra.
    juce::ignoreUnused (buffer);
}
