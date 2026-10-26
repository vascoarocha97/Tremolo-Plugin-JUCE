#include "TestSignalSource.h"
#include "DemoLoop.h"

#include <cmath>
#include <numbers>

namespace learn
{

TestSignalSource::TestSignalSource() = default;
TestSignalSource::~TestSignalSource() = default;

juce::StringArray TestSignalSource::getSourceNames()
{
    // Pela ordem do enum SourceType
    return { juce::String::fromUTF8 ("Silêncio"),
             juce::String::fromUTF8 ("Loop de demonstração"),
             juce::String::fromUTF8 ("Seno"),
             juce::String::fromUTF8 ("Dente de serra ingénuo (com aliasing)"),
             juce::String::fromUTF8 ("Varrimento 20 Hz → 20 kHz"),
             juce::String::fromUTF8 ("Ruído branco"),
             juce::String::fromUTF8 ("Impulsos (2 por segundo)"),
             juce::String::fromUTF8 ("Ficheiro de áudio"),
             juce::String::fromUTF8 ("Entrada (microfone / interface)") };
}

void TestSignalSource::prepare (double newSampleRate, int)
{
    sampleRate = newSampleRate;
    phase = 0.0;
    sweepTime = 0.0;
    clickCounter = 0;

    smoothedLevel.reset (sampleRate, 0.02);
    smoothedLevel.setCurrentAndTargetValue (levelGain.load());

    if (demoLoopRate != sampleRate)
    {
        demoLoop = renderDemoLoop (sampleRate);
        demoLoopRate = sampleRate;
        demoLoopPosition = 0;
    }

    resampleFileIfNeeded();
}

void TestSignalSource::render (juce::AudioBuffer<float>& buffer, const float* const* inputs,
                               int numInputs, int inputOffset) noexcept
{
    const auto numSamples = buffer.getNumSamples();
    buffer.clear();

    switch (type.load())
    {
        case SourceType::silence:
            return;

        case SourceType::demoLoop:
            renderLooped (&demoLoop, demoLoopPosition, buffer);
            break;

        case SourceType::file:
        {
            const juce::SpinLock::ScopedTryLockType lock (fileLock);

            if (lock.isLocked())
                renderLooped (filePlayback.get(), filePosition, buffer);

            break;
        }

        case SourceType::liveInput:
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                // Entrada mono => copiada para os dois canais
                const auto inputChannel = juce::jmin (ch, numInputs - 1);

                if (inputChannel >= 0 && inputs[inputChannel] != nullptr)
                    buffer.copyFrom (ch, 0, inputs[inputChannel] + inputOffset, numSamples);
            }

            break;
        }

        case SourceType::sine:
        case SourceType::naiveSaw:
        case SourceType::sweep:
        case SourceType::noise:
        case SourceType::clicks:
            renderGenerator (buffer);
            break;
    }

    // Nível da fonte (com smoothing para não dar clicks ao mexer no slider)
    smoothedLevel.setTargetValue (levelGain.load());

    for (int n = 0; n < numSamples; ++n)
    {
        const auto gain = smoothedLevel.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer (ch)[n] *= gain;
    }
}

void TestSignalSource::renderGenerator (juce::AudioBuffer<float>& buffer) noexcept
{
    constexpr auto twoPi = 2.0 * std::numbers::pi;
    const auto numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);
    const auto currentType = type.load();
    const auto hz = static_cast<double> (frequency.load());

    for (int n = 0; n < numSamples; ++n)
    {
        float sample = 0.0f;

        switch (currentType)
        {
            case SourceType::sine:
                sample = static_cast<float> (std::sin (phase));
                phase = std::fmod (phase + twoPi * hz / sampleRate, twoPi);
                break;

            case SourceType::naiveSaw:
                // Ingénuo: rampa de -1 a 1 sem limitar a banda => harmónicos acima de Nyquist dobram
                sample = static_cast<float> (phase / std::numbers::pi - 1.0);
                phase = std::fmod (phase + twoPi * hz / sampleRate, twoPi);
                break;

            case SourceType::sweep:
            {
                // Varrimento logarítmico de 20 Hz a 20 kHz em 10 s, depois recomeça
                constexpr double duration = 10.0, f0 = 20.0, f1 = 20000.0;
                const auto f = f0 * std::pow (f1 / f0, sweepTime / duration);
                sample = f < sampleRate * 0.5 ? static_cast<float> (std::sin (phase)) : 0.0f;
                phase = std::fmod (phase + twoPi * f / sampleRate, twoPi);
                sweepTime += 1.0 / sampleRate;

                if (sweepTime >= duration)
                    sweepTime = 0.0;

                break;
            }

            case SourceType::noise:
                sample = 2.0f * random.nextFloat() - 1.0f;
                break;

            case SourceType::clicks:
                sample = clickCounter == 0 ? 1.0f : 0.0f;

                if (++clickCounter >= static_cast<int> (sampleRate * 0.5))
                    clickCounter = 0;

                break;

            case SourceType::silence:
            case SourceType::demoLoop:
            case SourceType::file:
            case SourceType::liveInput:
                break;
        }

        left[n] = sample;
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

void TestSignalSource::renderLooped (const juce::AudioBuffer<float>* loop, int& position,
                                     juce::AudioBuffer<float>& buffer) noexcept
{
    if (loop == nullptr || loop->getNumSamples() == 0)
        return;

    const auto loopLength = loop->getNumSamples();
    int written = 0;

    while (written < buffer.getNumSamples())
    {
        position %= loopLength;
        const auto chunk = juce::jmin (buffer.getNumSamples() - written, loopLength - position);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto sourceChannel = juce::jmin (ch, loop->getNumChannels() - 1);
            buffer.copyFrom (ch, written, *loop, sourceChannel, position, chunk);
        }

        written += chunk;
        position += chunk;
    }
}

bool TestSignalSource::loadFile (const juce::File& file, juce::String& errorMessage)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
    {
        errorMessage = juce::String::fromUTF8 ("Formato não suportado: ") + file.getFileName();
        return false;
    }

    // Máximo de 60 s para não encher a memória
    const auto length = static_cast<int> (juce::jmin<juce::int64> (reader->lengthInSamples,
                                                                   static_cast<juce::int64> (reader->sampleRate * 60.0)));
    auto newBuffer = std::make_unique<juce::AudioBuffer<float>> (
        static_cast<int> (juce::jmin (reader->numChannels, 2u)), length);
    reader->read (newBuffer.get(), 0, length, 0, true, newBuffer->getNumChannels() > 1);

    {
        const juce::SpinLock::ScopedLockType lock (fileLock);
        std::swap (fileOriginal, newBuffer);
        fileOriginalRate = reader->sampleRate;
        filePlayback.reset();
        filePosition = 0;
        fileName = file.getFileName();
    }

    resampleFileIfNeeded();
    return true;
}

juce::String TestSignalSource::getLoadedFileName() const
{
    return fileName;
}

void TestSignalSource::resampleFileIfNeeded()
{
    std::unique_ptr<juce::AudioBuffer<float>> original, resampled;
    double originalRate = 0.0;

    {
        const juce::SpinLock::ScopedLockType lock (fileLock);

        if (fileOriginal == nullptr)
            return;

        original = std::make_unique<juce::AudioBuffer<float>> (*fileOriginal);
        originalRate = fileOriginalRate;
    }

    // Conversão de sample rate offline (interpolação de Lagrange, 4 pontos)
    const auto ratio = originalRate / sampleRate;
    // (-4: o interpolador de Lagrange precisa de algumas amostras de margem no fim)
    const auto outLength = juce::jmax (0, static_cast<int> (original->getNumSamples() / ratio) - 4);
    resampled = std::make_unique<juce::AudioBuffer<float>> (original->getNumChannels(), outLength);

    for (int ch = 0; ch < original->getNumChannels(); ++ch)
    {
        juce::LagrangeInterpolator interpolator;
        interpolator.process (ratio, original->getReadPointer (ch), resampled->getWritePointer (ch),
                              outLength);
    }

    {
        const juce::SpinLock::ScopedLockType lock (fileLock);
        std::swap (filePlayback, resampled);
        filePosition = 0;
    }
    // o buffer antigo é libertado aqui, fora do lock
}

} // namespace learn
