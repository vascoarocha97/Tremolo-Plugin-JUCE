// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// Repara: o wrapper NÃO chama setGainDecibels no processBlock. Um juce::AudioProcessorParameter::Listener
// recebe a mudança na thread que a fez (a GUI, ou a thread de automação do host) e passa-a ao
// SmoothedGain, que a guarda num std::atomic.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class SmoothedGainExercise final : public learn::ExerciseAudioProcessor,
                                   private juce::AudioProcessorParameter::Listener
{
public:
    SmoothedGainExercise()
    {
        addParameter (gainDb = new juce::AudioParameterFloat (
                          juce::ParameterID { "gain", 1 }, "Ganho",
                          juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
        gainDb->addListener (this);
    }

    ~SmoothedGainExercise() override { gainDb->removeListener (this); }

    const juce::String getName() const override { return "Ganho suave e thread-safe"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        gain.setGainDecibels (gainDb->get());
        gain.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock),
                        static_cast<juce::uint32> (getTotalNumOutputChannels()) });
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::dsp::AudioBlock<float> block (buffer);
        gain.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

private:
    void parameterValueChanged (int, float) override  { gain.setGainDecibels (gainDb->get()); }
    void parameterGestureChanged (int, bool) override {}

    SmoothedGain gain;
    juce::AudioParameterFloat* gainDb = nullptr;
};

LEARN_EXERCISE_APP (SmoothedGainExercise)
