// Wrapper do standalone: liga o teu FirstBufferProcessor a um juce::AudioProcessor real.
// Não precisas de editar este ficheiro.

#include <learn_host/HostApplication.h>

#include "Processor.h"

class FirstBufferExercise final : public learn::ExerciseAudioProcessor
{
public:
    const juce::String getName() const override { return juce::String::fromUTF8 ("O primeiro buffer"); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        dsp.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void releaseResources() override
    {
        dsp.releaseResources();
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        dsp.processBlock (buffer);
    }

private:
    FirstBufferProcessor dsp;
};

LEARN_EXERCISE_APP (FirstBufferExercise)
