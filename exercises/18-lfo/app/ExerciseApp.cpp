// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// O teu LFO a mover o pan (auto-pan) através do juce::dsp::Panner. Usa auscultadores.
// Com a forma "quadrada" e uma frequência alta ouves o som a saltar de lado.

#include <learn_host/HostApplication.h>
#include <juce_dsp/juce_dsp.h>

#include "Processor.h"

class LfoExercise final : public learn::ExerciseAudioProcessor
{
public:
    LfoExercise()
    {
        addParameter (rate = new juce::AudioParameterFloat (
                          juce::ParameterID { "rate", 1 }, "Velocidade",
                          juce::NormalisableRange<float> (0.05f, 20.0f, 0.0f, 0.4f), 0.5f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (shape = new juce::AudioParameterChoice (
                          juce::ParameterID { "shape", 1 }, "Forma",
                          juce::StringArray { "Seno", juce::String::fromUTF8 ("Triângulo"), "Dente de serra", "Quadrada" }, 0));
        addParameter (depth = new juce::AudioParameterFloat (juce::ParameterID { "depth", 1 }, "Profundidade",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    }

    const juce::String getName() const override { return "LFO (auto-pan)"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        lfo.prepare (sampleRate);
        lfo.reset();
        panner.setRule (juce::dsp::PannerRule::sin3dB);
        panner.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        lfo.setFrequency (rate->get());
        lfo.setShape (static_cast<Lfo::Shape> (shape->getIndex()));

        // O Panner do JUCE muda o pan por bloco; aqui processamos às fatias de 32 amostras para o
        // LFO mexer o pan de forma contínua.
        const auto numSamples = buffer.getNumSamples();

        for (int start = 0; start < numSamples; start += 32)
        {
            const auto num = juce::jmin (32, numSamples - start);
            float value = 0.0f;

            for (int i = 0; i < num; ++i)
                value = lfo.getNextSample();

            panner.setPan (juce::jlimit (-1.0f, 1.0f, value * depth->get()));
            auto block = juce::dsp::AudioBlock<float> (buffer).getSubBlock ((size_t) start, (size_t) num);
            panner.process (juce::dsp::ProcessContextReplacing<float> (block));
        }
    }

private:
    Lfo lfo;
    juce::dsp::Panner<float> panner;
    juce::AudioParameterFloat* rate = nullptr;
    juce::AudioParameterChoice* shape = nullptr;
    juce::AudioParameterFloat* depth = nullptr;
};

LEARN_EXERCISE_APP (LfoExercise)
