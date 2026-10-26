// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta: põe o pan no centro e troca de lei. Depois move o pan de um lado para o outro com
// cada lei e ouve se a fonte "afunda" no meio. Usa auscultadores.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class PanLawsExercise final : public learn::ExerciseAudioProcessor
{
public:
    PanLawsExercise()
    {
        addParameter (pan = new juce::AudioParameterFloat (juce::ParameterID { "pan", 1 }, "Pan",
                                                           juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
        addParameter (law = new juce::AudioParameterChoice (
                          juce::ParameterID { "law", 1 }, "Lei",
                          juce::StringArray { "Linear (-6 dB)", juce::String::fromUTF8 ("Potência constante (-3 dB)"),
                                              juce::String::fromUTF8 ("Compromisso (-4.5 dB)") },
                          1));
    }

    const juce::String getName() const override { return "Leis de pan"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        panner.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        panner.setPan (pan->get());
        panner.setLaw (static_cast<PanLaw> (law->getIndex()));
        panner.processBlock (buffer);
    }

private:
    StereoPanner panner;
    juce::AudioParameterFloat* pan = nullptr;
    juce::AudioParameterChoice* law = nullptr;
};

LEARN_EXERCISE_APP (PanLawsExercise)
