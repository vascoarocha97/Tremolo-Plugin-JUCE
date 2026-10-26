// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta com "Impulsos" e o loop: o comb com atraso curto soa metálico (é uma nota!);
// o allpass com atraso curto quase não muda o som contínuo, mas "espalha" os transientes.
// Mudar o atraso reconstrói as delay lines (no wrapper, de forma simplificada).

#include <learn_host/HostApplication.h>

#include "Processor.h"

class CombAllpassExercise final : public learn::ExerciseAudioProcessor
{
public:
    CombAllpassExercise()
    {
        addParameter (mode = new juce::AudioParameterChoice (juce::ParameterID { "mode", 1 }, "Filtro",
                                                             juce::StringArray { "Comb (feedback)", "Allpass" }, 0));
        addParameter (delay = new juce::AudioParameterFloat (
                          juce::ParameterID { "delay", 1 }, "Atraso",
                          juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f, 0.5f), 8.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("ms")));
        addParameter (gain = new juce::AudioParameterFloat (juce::ParameterID { "gain", 1 }, "Feedback / g",
                                                            juce::NormalisableRange<float> (0.0f, 0.97f), 0.8f));
        addParameter (damping = new juce::AudioParameterFloat (juce::ParameterID { "damping", 1 }, "Damping (comb)",
                                                               juce::NormalisableRange<float> (0.0f, 0.95f), 0.2f));
    }

    const juce::String getName() const override { return "Comb e allpass"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        currentRate = sampleRate;
        currentBlock = samplesPerBlock;
        preparedDelay = delay->get();
        update();
        demo.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (! juce::approximatelyEqual (delay->get(), preparedDelay))
            prepareToPlay (currentRate, currentBlock); // atalho de demonstração: aloca (ver exercício 26)

        update();
        juce::dsp::AudioBlock<float> block (buffer);
        demo.process (juce::dsp::ProcessContextReplacing<float> (block));
        buffer.applyGain (0.3f);
    }

private:
    void update()
    {
        demo.setParameters (static_cast<CombAllpassDemo::Mode> (mode->getIndex()), delay->get(), gain->get(), damping->get());
    }

    CombAllpassDemo demo;
    juce::AudioParameterChoice* mode = nullptr;
    juce::AudioParameterFloat* delay = nullptr;
    juce::AudioParameterFloat* gain = nullptr;
    juce::AudioParameterFloat* damping = nullptr;
    double currentRate = 44100.0;
    int currentBlock = 512;
    float preparedDelay = -1.0f;
};

LEARN_EXERCISE_APP (CombAllpassExercise)
