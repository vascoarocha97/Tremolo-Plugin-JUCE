// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Um passa-baixo que evita trabalho. Escolhe a fonte "Silêncio" ou "Impulsos" e vê o contador de
// "Blocos saltados" a subir; mexe no corte e vê os "Recálculos". Usa ./learn run 38 (Release).

#include <learn_host/HostApplication.h>

#include "Processor.h"

class AvoidWorkExercise final : public learn::ExerciseAudioProcessor
{
public:
    AvoidWorkExercise()
    {
        addParameter (cutoff = new juce::AudioParameterFloat (juce::ParameterID { "cutoff", 1 }, "Corte",
                                                              juce::NormalisableRange<float> (50.0f, 15000.0f, 1.0f, 0.3f), 1200.0f,
                                                              juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (resonance = new juce::AudioParameterFloat (juce::ParameterID { "q", 1 }, "Q",
                                                                 juce::NormalisableRange<float> (0.3f, 10.0f, 0.01f, 0.5f), 0.7071f));
        addParameter (skipped = new juce::AudioParameterInt (juce::ParameterID { "skipped", 1 }, "Blocos saltados", 0, 1000000, 0));
        addParameter (updates = new juce::AudioParameterInt (juce::ParameterID { "updates", 1 }, juce::String::fromUTF8 ("Recálculos"), 0, 1000000, 0));
    }

    const juce::String getName() const override { return "Evitar trabalho"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        filter.setCutoff (cutoff->get());
        filter.setResonance (resonance->get());
        filter.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        filter.setCutoff (cutoff->get());
        filter.setResonance (resonance->get());

        juce::dsp::AudioBlock<float> block (buffer);
        filter.process (juce::dsp::ProcessContextReplacing<float> (block));

        // Mostrar contadores como "parâmetros" é só um atalho deste wrapper.
        *skipped = filter.getSkippedBlocks();
        *updates = filter.getCoefficientUpdates();
    }

private:
    EfficientLowpass filter;
    juce::AudioParameterFloat* cutoff = nullptr;
    juce::AudioParameterFloat* resonance = nullptr;
    juce::AudioParameterInt* skipped = nullptr;
    juce::AudioParameterInt* updates = nullptr;
};

LEARN_EXERCISE_APP (AvoidWorkExercise)
