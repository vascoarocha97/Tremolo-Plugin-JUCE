// Wrapper do standalone. Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class TremoloExercise final : public learn::ExerciseAudioProcessor
{
public:
    TremoloExercise()
    {
        addParameter (rate = new juce::AudioParameterFloat (
                          juce::ParameterID { "rate", 1 }, "Velocidade",
                          juce::NormalisableRange<float> (0.1f, 20.0f, 0.0f, 0.4f), 5.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (depth = new juce::AudioParameterFloat (juce::ParameterID { "depth", 1 }, "Profundidade",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));
        addParameter (shape = new juce::AudioParameterChoice (
                          juce::ParameterID { "shape", 1 }, "Forma",
                          juce::StringArray { "Seno", juce::String::fromUTF8 ("Triângulo"), "Dente de serra", "Quadrada" }, 0));
        addParameter (stereo = new juce::AudioParameterFloat (
                          juce::ParameterID { "stereo", 1 }, juce::String::fromUTF8 ("Fase estéreo"),
                          juce::NormalisableRange<float> (0.0f, 180.0f, 1.0f), 0.0f,
                          juce::AudioParameterFloatAttributes().withLabel (juce::String::fromUTF8 ("°"))));
    }

    const juce::String getName() const override { return "Tremolo"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        tremolo.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        tremolo.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        tremolo.setRate (rate->get());
        tremolo.setDepth (depth->get());
        tremolo.setShape (static_cast<Lfo::Shape> (shape->getIndex()));
        tremolo.setStereoPhase (stereo->get());
    }

    Tremolo tremolo;
    juce::AudioParameterFloat* rate = nullptr;
    juce::AudioParameterFloat* depth = nullptr;
    juce::AudioParameterChoice* shape = nullptr;
    juce::AudioParameterFloat* stereo = nullptr;
};

LEARN_EXERCISE_APP (TremoloExercise)
