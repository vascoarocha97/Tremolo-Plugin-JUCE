// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// O áudio passa por um hard clip com drive. O editor recolhe a saída (FIFO lock-free, como no
// exercício 05) e, na thread da GUI, usa as TUAS funções Spectrum::magnitudeDb/aliasingRatioDb
// para mostrar o aliasing medido. Usa a fonte "Seno" e sobe a frequência e o drive.

#include <learn_host/HostApplication.h>

#include "Processor.h"

#include <atomic>

class AliasingMeterExercise;

class AliasingEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    AliasingEditor (juce::AudioProcessor& p, juce::AbstractFifo& f, std::vector<float>& d, std::atomic<double>& rate)
        : AudioProcessorEditor (p), fifo (f), fifoData (d), sampleRate (rate)
    {
        setSize (380, 140);
        startTimerHz (4);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff15181d));
        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (juce::String::fromUTF8 ("Medido com as tuas funções (FFT de 16384 pontos):"),
                    getLocalBounds().reduced (10).removeFromTop (20), juce::Justification::left);
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff38bdf8));
        g.drawMultiLineText (text, 10, 62, getWidth() - 20);
    }

private:
    void timerCallback() override
    {
        // Lê da FIFO para uma história circular de 16384 amostras
        const auto scope = fifo.read (fifo.getNumReady());
        auto copy = [&] (int start, int size)
        {
            for (int i = 0; i < size; ++i)
            {
                history[(size_t) writeIndex] = fifoData[(size_t) (start + i)];
                writeIndex = (writeIndex + 1) % (int) history.size();
            }
        };
        copy (scope.startIndex1, scope.blockSize1);
        copy (scope.startIndex2, scope.blockSize2);

        std::vector<float> ordered (history.size());
        for (size_t i = 0; i < history.size(); ++i)
            ordered[i] = history[(i + (size_t) writeIndex) % history.size()];

        // f0 estimado pelo bin mais forte
        const auto db = Spectrum::magnitudeDb (ordered.data(), 14);
        const auto peakBin = (int) std::distance (db.begin(), std::max_element (db.begin() + 1, db.end()));
        const auto rate = sampleRate.load();
        const auto f0 = Spectrum::binFrequency (peakBin, 14, rate);

        if (db[(size_t) peakBin] < -60.0f || rate <= 0.0)
            text = juce::String::fromUTF8 ("(sem sinal)");
        else
            text = "f0 = " + juce::String (f0, 0) + " Hz\naliasing = "
                   + juce::String (Spectrum::aliasingRatioDb (ordered.data(), 14, f0, rate), 1) + " dB";

        repaint();
    }

    juce::AbstractFifo& fifo;
    std::vector<float>& fifoData;
    std::atomic<double>& sampleRate;
    std::vector<float> history = std::vector<float> (16384, 0.0f);
    int writeIndex = 0;
    juce::String text = "...";
};

class AliasingMeterExercise final : public learn::ExerciseAudioProcessor
{
public:
    AliasingMeterExercise()
    {
        addParameter (drive = new juce::AudioParameterFloat (
                          juce::ParameterID { "drive", 1 }, "Drive (hard clip)",
                          juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f), 12.0f,
                          juce::AudioParameterFloatAttributes().withLabel ("dB")));
    }

    const juce::String getName() const override { return "Medir aliasing com FFT"; }
    bool hasEditor() const override { return true; }

    void prepareToPlay (double rate, int) override { sampleRate.store (rate); }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        const auto g = juce::Decibels::decibelsToGain (drive->get());

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                d[n] = 0.5f * juce::jlimit (-1.0f, 1.0f, g * d[n]);
        }

        const auto scope = fifo.write (buffer.getNumSamples());
        const auto* left = buffer.getReadPointer (0);
        for (int i = 0; i < scope.blockSize1; ++i) fifoData[(size_t) (scope.startIndex1 + i)] = left[i];
        for (int i = 0; i < scope.blockSize2; ++i) fifoData[(size_t) (scope.startIndex2 + i)] = left[scope.blockSize1 + i];
    }

private:
    juce::AudioProcessorEditor* createEditor() override { return new AliasingEditor (*this, fifo, fifoData, sampleRate); }

    juce::AudioParameterFloat* drive = nullptr;
    juce::AbstractFifo fifo { 65536 };
    std::vector<float> fifoData = std::vector<float> (65536, 0.0f);
    std::atomic<double> sampleRate { 0.0 };
};

LEARN_EXERCISE_APP (AliasingMeterExercise)
