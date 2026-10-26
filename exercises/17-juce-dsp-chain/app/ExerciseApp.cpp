// Wrapper do standalone. Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class DspChainExercise final : public learn::ExerciseAudioProcessor
{
public:
    DspChainExercise()
    {
        addParameter (lowCut = new juce::AudioParameterFloat (
                          juce::ParameterID { "lowcut", 1 }, "Corte de graves",
                          juce::NormalisableRange<float> (20.0f, 2000.0f, 0.0f, 0.3f), 120.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (highCut = new juce::AudioParameterFloat (
                          juce::ParameterID { "highcut", 1 }, "Corte de agudos",
                          juce::NormalisableRange<float> (1000.0f, 20000.0f, 0.0f, 0.3f), 8000.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("Hz")));
        addParameter (gain = new juce::AudioParameterFloat (
                          juce::ParameterID { "gain", 1 }, "Ganho",
                          juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "Cadeias juce::dsp"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        update();
        chain.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        update();
        juce::dsp::AudioBlock<float> block (buffer);
        chain.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void update()
    {
        chain.setLowCut (lowCut->get());
        chain.setHighCut (highCut->get());
        chain.setGainDecibels (gain->get());
    }

    BandFilterChain chain;
    juce::AudioParameterFloat* lowCut = nullptr;
    juce::AudioParameterFloat* highCut = nullptr;
    juce::AudioParameterFloat* gain = nullptr;
};

LEARN_EXERCISE_APP (DspChainExercise)
