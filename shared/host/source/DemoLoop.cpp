#include "DemoLoop.h"

#include <cmath>
#include <numbers>
#include <vector>

namespace learn
{
namespace
{
    constexpr double bpm = 100.0;
    constexpr int numBeats = 8; // 2 compassos de 4/4

    void addKick (juce::AudioBuffer<float>& out, int start, double sampleRate)
    {
        const auto length = static_cast<int> (0.45 * sampleRate);
        double phase = 0.0;

        for (int n = 0; n < length && start + n < out.getNumSamples(); ++n)
        {
            const auto t = n / sampleRate;
            const auto frequency = 45.0 + 75.0 * std::exp (-t * 30.0);   // queda de tom 120 -> 45 Hz
            phase += 2.0 * std::numbers::pi * frequency / sampleRate;
            const auto sample = static_cast<float> (0.9 * std::sin (phase) * std::exp (-t * 7.0));

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample (ch, start + n, sample);
        }
    }

    void addSnare (juce::AudioBuffer<float>& out, int start, double sampleRate, juce::Random& random)
    {
        const auto length = static_cast<int> (0.25 * sampleRate);
        double phase = 0.0;

        for (int n = 0; n < length && start + n < out.getNumSamples(); ++n)
        {
            const auto t = n / sampleRate;
            phase += 2.0 * std::numbers::pi * 185.0 / sampleRate;
            const auto noise = 2.0 * random.nextDouble() - 1.0;
            const auto sample = static_cast<float> (0.35 * noise * std::exp (-t * 18.0)
                                                    + 0.3 * std::sin (phase) * std::exp (-t * 30.0));

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample (ch, start + n, sample);
        }
    }

    void addHat (juce::AudioBuffer<float>& out, int start, double sampleRate, juce::Random& random,
                 float level)
    {
        const auto length = static_cast<int> (0.05 * sampleRate);
        double previous = 0.0;

        for (int n = 0; n < length && start + n < out.getNumSamples(); ++n)
        {
            const auto t = n / sampleRate;
            const auto noise = 2.0 * random.nextDouble() - 1.0;
            const auto highPassed = noise - previous; // diferença = passa-altos simples
            previous = noise;
            const auto sample = static_cast<float> (level * highPassed * std::exp (-t * 90.0));

            out.addSample (0, start + n, 0.7f * sample);

            if (out.getNumChannels() > 1)
                out.addSample (1, start + n, sample);
        }
    }

    /** Karplus-Strong: ruído num delay de comprimento fs/f com uma média de duas amostras
        no feedback (JOS, PASP: "Karplus-Strong Algorithm"). */
    void addPluck (juce::AudioBuffer<float>& out, int start, double sampleRate, double frequency,
                   double durationSeconds, juce::Random& random)
    {
        const auto delayLength = std::max (2, static_cast<int> (std::round (sampleRate / frequency)));
        std::vector<double> line (static_cast<size_t> (delayLength));

        for (auto& s : line)
            s = 2.0 * random.nextDouble() - 1.0;

        const auto length = static_cast<int> (durationSeconds * sampleRate);
        size_t index = 0;

        for (int n = 0; n < length && start + n < out.getNumSamples(); ++n)
        {
            const auto next = (index + 1) % line.size();
            const auto current = line[index];
            line[index] = 0.996 * 0.5 * (current + line[next]);
            index = next;

            const auto fadeOut = n > length - 256 ? (length - n) / 256.0 : 1.0;
            const auto sample = static_cast<float> (0.3 * current * fadeOut);

            out.addSample (0, start + n, sample);

            if (out.getNumChannels() > 1)
                out.addSample (1, start + n, 0.75f * sample);
        }
    }
}

juce::AudioBuffer<float> renderDemoLoop (double sampleRate)
{
    const auto samplesPerBeat = 60.0 / bpm * sampleRate;
    const auto totalSamples = static_cast<int> (numBeats * samplesPerBeat);
    const auto beatStart = [&] (double beat) { return static_cast<int> (beat * samplesPerBeat); };

    juce::AudioBuffer<float> out (2, totalSamples);
    out.clear();

    juce::Random random (42);

    for (int beat = 0; beat < numBeats; ++beat)
    {
        if (beat % 2 == 0)
            addKick (out, beatStart (beat), sampleRate);
        else
            addSnare (out, beatStart (beat), sampleRate, random);

        addHat (out, beatStart (beat), sampleRate, random, 0.25f);
        addHat (out, beatStart (beat + 0.5), sampleRate, random, 0.15f);
    }

    addKick (out, beatStart (6.5), sampleRate);

    // Linha em lá menor (frequências em Hz), em colcheias; 0 = pausa.
    const double riff[] = { 110.0, 0.0, 130.81, 164.81, 0.0, 196.0, 164.81, 0.0,
                            110.0, 0.0, 130.81, 146.83, 0.0, 130.81, 98.0, 0.0 };

    for (int step = 0; step < 16; ++step)
        if (riff[step] > 0.0)
            addPluck (out, beatStart (step * 0.5), sampleRate, riff[step], 0.9, random);

    // Normaliza para pico de 0.9
    const auto peak = out.getMagnitude (0, totalSamples);

    if (peak > 0.0f)
        out.applyGain (0.9f / peak);

    return out;
}

} // namespace learn
