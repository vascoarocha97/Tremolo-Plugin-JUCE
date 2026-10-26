// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta: Q alto (8–15), "Modulação" de 2–3 oitavas e LFO rápido — o filtro "canta" e não rebenta.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class SvfExercise final : public learn::ExerciseAudioProcessor
{
public:
    SvfExercise()
    {
        addParameter (mode = new juce::AudioParameterChoice (juce::ParameterID { "mode", 1 }, juce::String::fromUTF8 ("Saída"),
                                                             juce::StringArray { "Passa-baixo", "Passa-banda", "Passa-alto" }, 0));
        addParameter (cutoff = new juce::AudioParameterFloat (
                          juce::ParameterID { "cutoff", 1 }, "Corte",
                          juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 800.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (q = new juce::AudioParameterFloat (juce::ParameterID { "q", 1 }, juce::String::fromUTF8 ("Ressonância (Q)"),
                                                         juce::NormalisableRange<float> (0.3f, 20.0f, 0.0f, 0.4f), 2.0f));
        addParameter (octaves = new juce::AudioParameterFloat (juce::ParameterID { "mod", 1 }, juce::String::fromUTF8 ("Modulação (oitavas)"),
                                                               juce::NormalisableRange<float> (0.0f, 4.0f), 1.5f));
        addParameter (rate = new juce::AudioParameterFloat (
                          juce::ParameterID { "rate", 1 }, "LFO",
                          juce::NormalisableRange<float> (0.05f, 20.0f, 0.0f, 0.4f), 0.5f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    }

    const juce::String getName() const override { return "State Variable Filter (TPT)"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        filter.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        filter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        filter.setParameters (static_cast<SvfProcessor::Mode> (mode->getIndex()), cutoff->get(), q->get(),
                              octaves->get(), rate->get());
    }

    SvfProcessor filter;
    juce::AudioParameterChoice* mode = nullptr;
    juce::AudioParameterFloat* cutoff = nullptr;
    juce::AudioParameterFloat* q = nullptr;
    juce::AudioParameterFloat* octaves = nullptr;
    juce::AudioParameterFloat* rate = nullptr;
};

LEARN_EXERCISE_APP (SvfExercise)
