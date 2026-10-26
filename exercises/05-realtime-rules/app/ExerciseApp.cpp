// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// O medidor do exercício é lido pela thread da GUI, como num plugin real: este wrapper cria um
// editor simples que mostra getPeak() e um gráfico com readHistory().

#include <learn_host/HostApplication.h>

#include "Processor.h"

class RealtimeRulesExercise;

class MeterEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    MeterEditor (juce::AudioProcessor& p, PeakMeterProcessor& m)
        : AudioProcessorEditor (p), meter (m)
    {
        setSize (380, 260);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff15181d));
        auto area = getLocalBounds().reduced (10);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String::fromUTF8 ("getPeak() por canal (lido na thread da GUI)"),
                    area.removeFromTop (18), juce::Justification::left);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto bar = area.removeFromTop (18).reduced (0, 3).toFloat();
            g.setColour (juce::Colour (0xff2a2f38));
            g.fillRect (bar);
            g.setColour (juce::Colour (0xff38bdf8));
            g.fillRect (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, meter.getPeak (ch))));
        }

        area.removeFromTop (10);
        g.setColour (juce::Colours::lightgrey);
        g.drawText (juce::String::fromUTF8 ("readHistory(): pico de cada bloco"), area.removeFromTop (18),
                    juce::Justification::left);

        g.setColour (juce::Colour (0xff2a2f38));
        g.fillRect (area);

        juce::Path path;
        const auto w = static_cast<float> (area.getWidth()) / static_cast<float> (history.size());

        for (size_t i = 0; i < history.size(); ++i)
        {
            const auto x = static_cast<float> (area.getX()) + w * static_cast<float> (i);
            const auto y = static_cast<float> (area.getBottom()) - history[(i + writeIndex) % history.size()] * static_cast<float> (area.getHeight());
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }

        g.setColour (juce::Colour (0xff38bdf8));
        g.strokePath (path, juce::PathStrokeType (1.5f));
    }

private:
    void timerCallback() override
    {
        float values[256];
        const auto num = meter.readHistory (values, 256);

        for (int i = 0; i < num; ++i)
        {
            history[writeIndex] = values[i];
            writeIndex = (writeIndex + 1) % history.size();
        }

        repaint();
    }

    PeakMeterProcessor& meter;
    std::array<float, 300> history {};
    size_t writeIndex = 0;
};

class RealtimeRulesExercise final : public learn::ExerciseAudioProcessor
{
public:
    const juce::String getName() const override { return "Regras de tempo-real"; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        meter.prepareToPlay (sampleRate, samplesPerBlock);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        meter.processBlock (buffer);
    }

    bool hasEditor() const override { return true; }

private:
    juce::AudioProcessorEditor* createEditor() override { return new MeterEditor (*this, meter); }

    PeakMeterProcessor meter;
};

LEARN_EXERCISE_APP (RealtimeRulesExercise)
