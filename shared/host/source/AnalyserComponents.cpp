#include "AnalyserComponents.h"

#include <cmath>

namespace learn
{
namespace colours
{
    const juce::Colour background { 0xff15181d };
    const juce::Colour grid       { 0xff2a2f38 };
    const juce::Colour gridText   { 0xff7a8494 };
    const juce::Colour input      { 0xff6b7280 };
    const juce::Colour output     { 0xff38bdf8 };
}

//==============================================================================
void SampleHistory::append (const float* samples, int num)
{
    for (int n = 0; n < num; ++n)
    {
        data[(size_t) writeIndex] = samples[n];
        writeIndex = (writeIndex + 1) % size();
    }
}

void SampleHistory::copyLatest (float* destination, int num) const
{
    num = juce::jmin (num, size());
    auto readIndex = (writeIndex - num + size()) % size();

    for (int n = 0; n < num; ++n)
    {
        destination[n] = data[(size_t) readIndex];
        readIndex = (readIndex + 1) % size();
    }
}

//==============================================================================
ScopeComponent::ScopeComponent()
{
    setOpaque (true);
}

void ScopeComponent::update (const SampleHistory& input, const SampleHistory& output, double rate)
{
    sampleRate = rate > 0.0 ? rate : 44100.0;

    // Mostra 20 ms; procura um trigger (zero ascendente) na janela anterior para a imagem ficar parada
    const auto visible = static_cast<int> (sampleRate * 0.02);
    const auto searchLength = juce::jmin (output.size() - visible, visible * 2);

    std::vector<float> latestOut ((size_t) (visible + searchLength)), latestIn ((size_t) (visible + searchLength));
    output.copyLatest (latestOut.data(), visible + searchLength);
    input.copyLatest (latestIn.data(), visible + searchLength);

    int trigger = 0;

    for (int n = searchLength - 1; n > 0; --n)
    {
        if (latestOut[(size_t) n - 1] < 0.0f && latestOut[(size_t) n] >= 0.0f)
        {
            trigger = n;
            break;
        }
    }

    outputSnapshot.assign (latestOut.begin() + trigger, latestOut.begin() + trigger + visible);
    inputSnapshot.assign (latestIn.begin() + trigger, latestIn.begin() + trigger + visible);
    repaint();
}

void ScopeComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (colours::background);

    g.setColour (colours::grid);
    g.drawHorizontalLine (getHeight() / 2, 0.0f, bounds.getWidth());
    g.drawHorizontalLine (getHeight() / 4, 0.0f, bounds.getWidth());
    g.drawHorizontalLine (getHeight() * 3 / 4, 0.0f, bounds.getWidth());

    auto drawTrace = [&] (const std::vector<float>& samples, juce::Colour colour)
    {
        if (samples.size() < 2)
            return;

        juce::Path path;
        const auto xScale = bounds.getWidth() / static_cast<float> (samples.size() - 1);

        for (size_t n = 0; n < samples.size(); ++n)
        {
            const auto x = static_cast<float> (n) * xScale;
            const auto y = bounds.getCentreY() - juce::jlimit (-1.1f, 1.1f, samples[n]) * bounds.getHeight() * 0.45f;

            if (n == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.5f));
    };

    drawTrace (inputSnapshot, colours::input);
    drawTrace (outputSnapshot, colours::output);

    g.setColour (colours::gridText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (juce::String::fromUTF8 ("Osciloscópio · 20 ms · cinzento = entrada, azul = saída"),
                getLocalBounds().reduced (6, 4), juce::Justification::topLeft);
}

//==============================================================================
SpectrumComponent::SpectrumComponent()
    : fftData ((size_t) fftSize * 2, 0.0f),
      inputDb ((size_t) fftSize / 2, -120.0f),
      outputDb ((size_t) fftSize / 2, -120.0f)
{
    setOpaque (true);
}

void SpectrumComponent::computeSpectrum (const SampleHistory& history, std::vector<float>& smoothedDb)
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    history.copyLatest (fftData.data(), fftSize);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    // Um seno de amplitude A dá |X| = A * N/2 * 0.5 (ganho coerente da janela de Hann) = A * N/4
    const auto normalisation = 4.0f / fftSize;

    for (size_t bin = 0; bin < smoothedDb.size(); ++bin)
    {
        const auto db = juce::Decibels::gainToDecibels (fftData[bin] * normalisation, -120.0f);
        // Sobe imediatamente, desce devagar (mais fácil de ler)
        smoothedDb[bin] = juce::jmax (db, smoothedDb[bin] - 1.5f);
    }
}

void SpectrumComponent::update (const SampleHistory& input, const SampleHistory& output, double rate)
{
    sampleRate = rate > 0.0 ? rate : 44100.0;
    computeSpectrum (input, inputDb);
    computeSpectrum (output, outputDb);
    repaint();
}

float SpectrumComponent::frequencyToX (float frequency, float width) const
{
    const auto minF = 20.0f, maxF = static_cast<float> (sampleRate * 0.5);
    return width * std::log (frequency / minF) / std::log (maxF / minF);
}

void SpectrumComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto width = bounds.getWidth(), height = bounds.getHeight();
    constexpr float minDb = -100.0f, maxDb = 6.0f;
    const auto dbToY = [&] (float db) { return juce::jmap (juce::jlimit (minDb, maxDb, db), maxDb, minDb, 0.0f, height); };

    g.fillAll (colours::background);
    g.setFont (juce::FontOptions (11.0f));

    for (const auto f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        if (f >= sampleRate * 0.5)
            continue;

        const auto x = frequencyToX (f, width);
        g.setColour (colours::grid);
        g.drawVerticalLine (juce::roundToInt (x), 0.0f, height);
        g.setColour (colours::gridText);
        g.drawText (f >= 1000.0f ? juce::String (f / 1000.0f) + "k" : juce::String (f),
                    juce::Rectangle<float> (x + 2.0f, height - 14.0f, 40.0f, 12.0f),
                    juce::Justification::left);
    }

    for (const auto db : { 0.0f, -20.0f, -40.0f, -60.0f, -80.0f })
    {
        const auto y = dbToY (db);
        g.setColour (colours::grid);
        g.drawHorizontalLine (juce::roundToInt (y), 0.0f, width);
        g.setColour (colours::gridText);
        g.drawText (juce::String (db, 0) + " dB", juce::Rectangle<float> (2.0f, y + 1.0f, 50.0f, 12.0f),
                    juce::Justification::left);
    }

    auto drawSpectrum = [&] (const std::vector<float>& db, juce::Colour colour)
    {
        juce::Path path;
        bool started = false;

        for (size_t bin = 1; bin < db.size(); ++bin)
        {
            const auto frequency = static_cast<float> (bin * sampleRate / fftSize);

            if (frequency < 20.0f)
                continue;

            const auto x = frequencyToX (frequency, width);
            const auto y = dbToY (db[bin]);

            if (! started)
            {
                path.startNewSubPath (x, y);
                started = true;
            }
            else
            {
                path.lineTo (x, y);
            }
        }

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.2f));
    };

    drawSpectrum (inputDb, colours::input);
    drawSpectrum (outputDb, colours::output);

    g.setColour (colours::gridText);
    g.drawText (juce::String::fromUTF8 ("Espectro (FFT 4096, Hann) · cinzento = entrada, azul = saída"),
                getLocalBounds().reduced (60, 4), juce::Justification::topLeft);
}

//==============================================================================
void LevelMeter::setLevels (float left, float right)
{
    const float newLevels[2] { left, right };

    for (int ch = 0; ch < 2; ++ch)
    {
        levels[ch] = juce::jmax (newLevels[ch], levels[ch] * 0.85f);

        if (newLevels[ch] >= holds[ch])
        {
            holds[ch] = newLevels[ch];
            holdCounters[ch] = 45;
        }
        else if (--holdCounters[ch] <= 0)
        {
            holds[ch] *= 0.9f;
        }
    }

    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
    auto area = getLocalBounds().reduced (3);
    const auto barWidth = (area.getWidth() - 3) / 2;
    constexpr float minDb = -60.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto bar = area.removeFromLeft (barWidth).toFloat();
        area.removeFromLeft (3);

        const auto proportion = [&] (float level)
        {
            return juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (level, minDb) - minDb) / -minDb);
        };

        const auto levelHeight = bar.getHeight() * proportion (levels[ch]);
        g.setColour (colours::grid);
        g.fillRect (bar);
        g.setColour (levels[ch] >= 0.999f ? juce::Colours::red : colours::output);
        g.fillRect (bar.withTop (bar.getBottom() - levelHeight));

        const auto holdY = bar.getBottom() - bar.getHeight() * proportion (holds[ch]);
        g.setColour (holds[ch] >= 0.999f ? juce::Colours::red : juce::Colours::white);
        g.drawHorizontalLine (juce::roundToInt (holdY), bar.getX(), bar.getRight());
    }
}

} // namespace learn
