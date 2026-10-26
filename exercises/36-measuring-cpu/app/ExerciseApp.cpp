// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Olha para o "CPU" na linha de estado do standalone (medido pelo próprio JUCE) e para o parâmetro
// "Carga medida" (medido pelo TEU CpuLoadMeter). Aumenta as "Instâncias" até a versão ingénua
// passar dos 100% (ouve-se: cliques e buracos), depois muda para a tua versão.
// Usa ./learn run 36: compila em Release.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class CpuExercise final : public learn::ExerciseAudioProcessor
{
public:
    CpuExercise()
    {
        addParameter (implementation = new juce::AudioParameterChoice (juce::ParameterID { "implementation", 1 }, juce::String::fromUTF8 ("Implementação"),
                                                                       juce::StringArray { juce::String::fromUTF8 ("Ingénua (referência)"), "A tua (Fast)" }, 0));
        addParameter (instances = new juce::AudioParameterInt (juce::ParameterID { "instances", 1 }, juce::String::fromUTF8 ("Instâncias"), 1, 400, 1));
        addParameter (drive = new juce::AudioParameterFloat (juce::ParameterID { "drive", 1 }, "Drive",
                                                             juce::NormalisableRange<float> (0.0f, 30.0f, 0.1f), 12.0f,
                                                             juce::AudioParameterFloatAttributes().withLabel ("dB")));
        addParameter (cutoff = new juce::AudioParameterFloat (juce::ParameterID { "cutoff", 1 }, "Corte",
                                                              juce::NormalisableRange<float> (200.0f, 15000.0f, 1.0f, 0.3f), 3000.0f,
                                                              juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (measured = new juce::AudioParameterFloat (juce::ParameterID { "measured", 1 }, "Carga medida",
                                                                juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 0.0f,
                                                                juce::AudioParameterFloatAttributes().withLabel ("%")));
    }

    const juce::String getName() const override { return "Medir CPU"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        const juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
        naive.prepare (spec);
        fast.prepare (spec);
        meter.prepare (sampleRate);
        scratch.setSize (2, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        const DriveFilterSettings settings { drive->get(), cutoff->get(), -6.0f };
        const auto useFast = implementation->getIndex() == 1;
        const auto numSamples = juce::jmin (buffer.getNumSamples(), scratch.getNumSamples());

        {
            const CpuLoadMeter::ScopedMeasurement measure (meter, buffer.getNumSamples());

            // As instâncias extra processam uma cópia (só para gastar CPU); a última, o sinal real.
            for (int i = 1; i < instances->get(); ++i)
            {
                for (int ch = 0; ch < 2; ++ch)
                    scratch.copyFrom (ch, 0, buffer, juce::jmin (ch, buffer.getNumChannels() - 1), 0, numSamples);

                juce::dsp::AudioBlock<float> block (scratch);
                auto sub = block.getSubBlock (0, static_cast<size_t> (numSamples));
                process (sub, settings, useFast);
            }

            juce::dsp::AudioBlock<float> block (buffer);
            process (block, settings, useFast);
        }

        // Mostrar um valor medido num "parâmetro" é um atalho deste wrapper (num plugin real a GUI
        // leria meter.getLoad() num Timer).
        measured->setValueNotifyingHost (measured->convertTo0to1 (juce::jmin (200.0f, 100.0f * meter.getLoad())));
    }

private:
    void process (juce::dsp::AudioBlock<float>& block, const DriveFilterSettings& settings, bool useFast) noexcept
    {
        const juce::dsp::ProcessContextReplacing<float> context (block);
        if (useFast)
            fast.process (context, settings);
        else
            naive.process (context, settings);
    }

    NaiveDriveFilter naive;
    FastDriveFilter fast;
    CpuLoadMeter meter;
    juce::AudioBuffer<float> scratch;

    juce::AudioParameterChoice* implementation = nullptr;
    juce::AudioParameterInt* instances = nullptr;
    juce::AudioParameterFloat* drive = nullptr;
    juce::AudioParameterFloat* cutoff = nullptr;
    juce::AudioParameterFloat* measured = nullptr;
};

LEARN_EXERCISE_APP (CpuExercise)
