// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Usa o "Loop de demonstração", carrega em "Repetir" e muda a "Fatia" (só conta quando volta a
// ligar). Experimenta "Crossfade" no mínimo para ouvir os cliques que estás a evitar.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class GlitchExercise final : public learn::ExerciseAudioProcessor
{
public:
    GlitchExercise()
    {
        addParameter (repeat = new juce::AudioParameterBool (juce::ParameterID { "repeat", 1 }, "Repetir", false));
        addParameter (sliceMs = new juce::AudioParameterFloat (juce::ParameterID { "slice", 1 }, "Fatia",
                                                               juce::NormalisableRange<float> (20.0f, Glitch::maxSliceMs, 1.0f, 0.5f), 150.0f,
                                                               juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (fadeMs = new juce::AudioParameterFloat (juce::ParameterID { "fade", 1 }, "Crossfade",
                                                              juce::NormalisableRange<float> (0.1f, Glitch::maxFadeMs, 0.1f), 4.0f,
                                                              juce::AudioParameterFloatAttributes().withLabel ("ms")));
    }

    const juce::String getName() const override { return "Glitch"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        glitch.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        glitch.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        glitch.setSliceMs (sliceMs->get());
        glitch.setFadeMs (fadeMs->get());
        glitch.setActive (repeat->get());
    }

    Glitch glitch;
    juce::AudioParameterBool* repeat = nullptr;
    juce::AudioParameterFloat* sliceMs = nullptr;
    juce::AudioParameterFloat* fadeMs = nullptr;
};

LEARN_EXERCISE_APP (GlitchExercise)
