// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Experimenta: fonte "Seno" a 3–5 kHz, hard clip, drive alto. Alterna "Oversampling" entre 1× e 8×
// e olha para o espectro: as riscas que não são harmónicos (aliasing) desaparecem.
// Mudar o fator obriga a preparar de novo (o wrapper faz isso no processBlock — ver o comentário lá).

#include <learn_host/HostApplication.h>

#include "Processor.h"

class OversamplingExercise final : public learn::ExerciseAudioProcessor
{
public:
    OversamplingExercise()
    {
        addParameter (factor = new juce::AudioParameterChoice (juce::ParameterID { "factor", 1 }, "Oversampling",
                                                               juce::StringArray { "1x", "2x", "4x", "8x", "16x" }, 2));
        addParameter (shape = new juce::AudioParameterChoice (juce::ParameterID { "shape", 1 }, "Curva",
                                                              juce::StringArray { "Hard clip", juce::String::fromUTF8 ("Soft clip (cúbica)"), "tanh",
                                                                                  juce::String::fromUTF8 ("Assimétrica") }, 0));
        addParameter (drive = new juce::AudioParameterFloat (
                          juce::ParameterID { "drive", 1 }, "Drive",
                          juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f), 18.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "Oversampling"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;
        preparedFactor = factor->getIndex();
        shaper.setOversamplingFactorLog2 (preparedFactor);
        shaper.setShape (static_cast<Shapers::Shape> (shape->getIndex()));
        shaper.setDriveDecibels (drive->get());
        shaper.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });

        // Num plugin real: o host compensa este atraso nas outras pistas.
        setLatencySamples (juce::roundToInt (shaper.getLatencyInSamples()));
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (factor->getIndex() != preparedFactor)
        {
            // Atalho de demonstração: preparar aloca memória e não devia acontecer na thread de áudio.
            // Num plugin real, muda-se o fator noutra thread (com suspendProcessing) ou têm-se vários
            // objetos Oversampling já preparados.
            prepareToPlay (currentSampleRate, currentBlockSize);
        }

        shaper.setShape (static_cast<Shapers::Shape> (shape->getIndex()));
        shaper.setDriveDecibels (drive->get());
        juce::dsp::AudioBlock<float> block (buffer);
        shaper.process (juce::dsp::ProcessContextReplacing<float> (block));
        buffer.applyGain (0.5f);
    }

private:
    OversampledShaper shaper;
    juce::AudioParameterChoice* factor = nullptr;
    juce::AudioParameterChoice* shape = nullptr;
    juce::AudioParameterFloat* drive = nullptr;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    int preparedFactor = -1;
};

LEARN_EXERCISE_APP (OversamplingExercise)
