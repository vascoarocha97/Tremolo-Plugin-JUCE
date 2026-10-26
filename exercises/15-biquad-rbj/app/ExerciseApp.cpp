// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta com "Ruído branco" e olha para a forma do espectro de cada tipo de filtro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class BiquadExercise final : public learn::ExerciseAudioProcessor
{
public:
    BiquadExercise()
    {
        addParameter (type = new juce::AudioParameterChoice (
                          juce::ParameterID { "type", 1 }, "Tipo",
                          juce::StringArray { "Passa-baixo", "Passa-alto", "Passa-banda", "Notch", "Peaking",
                                              "Low shelf", "High shelf" },
                          0));
        addParameter (frequency = new juce::AudioParameterFloat (
                          juce::ParameterID { "frequency", 1 }, juce::String::fromUTF8 ("Frequência"),
                          juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 1000.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (q = new juce::AudioParameterFloat (juce::ParameterID { "q", 1 }, "Q",
                                                         juce::NormalisableRange<float> (0.1f, 18.0f, 0.0f, 0.35f), 0.7071f));
        addParameter (gain = new juce::AudioParameterFloat (
                          juce::ParameterID { "gain", 1 }, "Ganho (peaking/shelf)",
                          juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "Biquad (Audio EQ Cookbook)"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        updateFilter();
        filter.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        updateFilter();
        juce::dsp::AudioBlock<float> block (buffer);
        filter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void updateFilter()
    {
        filter.setParameters (static_cast<BiquadFilter::Type> (type->getIndex()), frequency->get(), q->get(), gain->get());
    }

    BiquadFilter filter;
    juce::AudioParameterChoice* type = nullptr;
    juce::AudioParameterFloat* frequency = nullptr;
    juce::AudioParameterFloat* q = nullptr;
    juce::AudioParameterFloat* gain = nullptr;
};

LEARN_EXERCISE_APP (BiquadExercise)
