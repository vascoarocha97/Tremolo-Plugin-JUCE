// Wrapper do standalone. Não precisas de editar este ficheiro.
// Experimenta a fonte "Seno" a 200 Hz: no espectro vês os harmónicos de cada curva.
// Depois sobe a frequência para 3–5 kHz com drive alto: as riscas que não são harmónicos são aliasing.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class WaveshaperExercise final : public learn::ExerciseAudioProcessor
{
public:
    WaveshaperExercise()
    {
        addParameter (shape = new juce::AudioParameterChoice (juce::ParameterID { "shape", 1 }, "Curva",
                                                              juce::StringArray { "Hard clip", juce::String::fromUTF8 ("Soft clip (cúbica)"), "tanh",
                                                                                  juce::String::fromUTF8 ("Assimétrica") }, 2));
        addParameter (drive = new juce::AudioParameterFloat (
                          juce::ParameterID { "drive", 1 }, "Drive",
                          juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f), 12.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
        addParameter (output = new juce::AudioParameterFloat (
                          juce::ParameterID { "output", 1 }, juce::String::fromUTF8 ("Saída"),
                          juce::NormalisableRange<float> (-30.0f, 6.0f, 0.1f), -6.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "Waveshaper"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        shaper.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        shaper.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        shaper.setShape (static_cast<Shapers::Shape> (shape->getIndex()));
        shaper.setDriveDecibels (drive->get());
        shaper.setOutputDecibels (output->get());
    }

    Waveshaper shaper;
    juce::AudioParameterChoice* shape = nullptr;
    juce::AudioParameterFloat* drive = nullptr;
    juce::AudioParameterFloat* output = nullptr;
};

LEARN_EXERCISE_APP (WaveshaperExercise)
