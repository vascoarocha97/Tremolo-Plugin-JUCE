// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta: 4–6 bits com o loop de demonstração; depois baixa o "Nível" da fonte para -40 dB
// e compara dither ligado/desligado.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class BitDepthExercise final : public learn::ExerciseAudioProcessor
{
public:
    BitDepthExercise()
    {
        addParameter (bits = new juce::AudioParameterInt (juce::ParameterID { "bits", 1 }, "Bits", 1, 16, 8));
        addParameter (dither = new juce::AudioParameterBool (juce::ParameterID { "dither", 1 }, "Dither TPDF", false));
    }

    const juce::String getName() const override { return juce::String::fromUTF8 ("Bit depth e quantização"); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        reducer.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        reducer.setBits (bits->get());
        reducer.setDitherEnabled (dither->get());
        reducer.processBlock (buffer);
    }

private:
    BitDepthReducer reducer;
    juce::AudioParameterInt* bits = nullptr;
    juce::AudioParameterBool* dither = nullptr;
};

LEARN_EXERCISE_APP (BitDepthExercise)
