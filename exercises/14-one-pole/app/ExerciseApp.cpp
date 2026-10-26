// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta com a fonte "Ruído branco" e olha para o espectro: a inclinação é −6 dB por oitava.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class OnePoleExercise final : public learn::ExerciseAudioProcessor
{
public:
    OnePoleExercise()
    {
        addParameter (cutoff = new juce::AudioParameterFloat (
                          juce::ParameterID { "cutoff", 1 }, "Corte",
                          juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 1000.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (type = new juce::AudioParameterChoice (juce::ParameterID { "type", 1 }, "Tipo",
                                                             juce::StringArray { "Passa-baixo", "Passa-alto" }, 0));
    }

    const juce::String getName() const override { return "Filtro de um polo"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        filter.setCutoff (cutoff->get());
        filter.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        filter.setCutoff (cutoff->get());
        filter.setType (type->getIndex() == 0 ? OnePoleFilter::Type::lowPass : OnePoleFilter::Type::highPass);
        juce::dsp::AudioBlock<float> block (buffer);
        filter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    OnePoleProcessor filter;
    juce::AudioParameterFloat* cutoff = nullptr;
    juce::AudioParameterChoice* type = nullptr;
};

LEARN_EXERCISE_APP (OnePoleExercise)
