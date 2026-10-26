#include "Processor.h"

#include <cmath>

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    // TODO(1): cria e devolve os 7 parâmetros (ver a tabela no README). Para cada um:
    //   - ParameterID { EchoParams::xxx, EchoParams::version }  — ID estável + versão;
    //   - nome para o host ("Ganho", "Tempo", ...), gama (NormalisableRange), valor por omissão;
    //   - unidades com AudioParameterFloatAttributes().withLabel ("dB").
    //   O tempo e o tom precisam de setSkewForCentre (250 ms e 3000 Hz): o meio do curso do botão
    //   fica nesse valor (o ouvido é logarítmico).
    //   Tipos: AudioParameterFloat (5), AudioParameterChoice ("Mono", "Ping-pong"), AudioParameterBool.
    //   layout.add (std::make_unique<...> (...), std::make_unique<...> (...), ...);
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    return layout;
}

//==============================================================================
void EchoEngine::prepare (const juce::dsp::ProcessSpec& spec, const Settings& initial)
{
    sampleRate = spec.sampleRate;

    delay.setMaximumDelayInSamples (static_cast<int> (std::ceil (maxTimeMs * 0.001 * sampleRate)) + 2);
    delay.prepare ({ spec.sampleRate, spec.maximumBlockSize, 2 });

    for (auto& t : tone)
    {
        t.prepare ({ spec.sampleRate, spec.maximumBlockSize, 1 });
        t.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
    }

    gain.reset (sampleRate, 0.02);
    timeSamples.reset (sampleRate, 0.1);   // mudar o tempo depressa demais soa a "pitch bend"
    feedback.reset (sampleRate, 0.02);
    mix.reset (sampleRate, 0.02);

    gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (initial.gainDecibels));
    timeSamples.setCurrentAndTargetValue (initial.timeMs * 0.001f * static_cast<float> (sampleRate));
    feedback.setCurrentAndTargetValue (initial.feedback);
    mix.setCurrentAndTargetValue (initial.mix);

    reset();
}

void EchoEngine::reset() noexcept
{
    delay.reset();

    for (auto& t : tone)
        t.reset();
}

void EchoEngine::process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept
{
    const auto numChannels = juce::jmin (buffer.getNumChannels(), 2);

    if (numChannels == 0)
        return;

    gain.setTargetValue (juce::Decibels::decibelsToGain (s.gainDecibels));
    timeSamples.setTargetValue (juce::jlimit (1.0f, maxTimeMs, s.timeMs) * 0.001f * static_cast<float> (sampleRate));
    feedback.setTargetValue (juce::jlimit (0.0f, 0.95f, s.feedback));
    mix.setTargetValue (juce::jlimit (0.0f, 1.0f, s.mix));

    for (auto& t : tone)
        t.setCutoffFrequency (juce::jlimit (20.0f, 0.45f * static_cast<float> (sampleRate), s.toneHz));

    auto* left = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        const auto d = timeSamples.getNextValue();
        const auto fb = feedback.getNextValue();
        const auto m = mix.getNextValue();
        const auto g = gain.getNextValue();

        const auto inL = left[n];
        const auto inR = right != nullptr ? right[n] : inL;

        // pop antes de push: atraso de exatamente d amostras (exercício 13)
        const auto echoL = tone[0].processSample (0, delay.popSample (0, d));
        const auto echoR = tone[1].processSample (0, delay.popSample (1, d));

        if (s.pingPong)
        {
            delay.pushSample (0, 0.5f * (inL + inR) + fb * echoR);
            delay.pushSample (1, fb * echoL);
        }
        else
        {
            delay.pushSample (0, inL + fb * echoL);
            delay.pushSample (1, inR + fb * echoR);
        }

        left[n] = g * ((1.0f - m) * inL + m * echoL);

        if (right != nullptr)
            right[n] = g * ((1.0f - m) * inR + m * echoR);
    }
}

//==============================================================================
EchoProcessor::EchoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // TODO(2): guarda os ponteiros para os valores dos parâmetros (membros gain, time, ...):
    //   - parâmetros float: apvts.getRawParameterValue (id)  -> std::atomic<float>* (valor REAL, p. ex. −6 dB);
    //   - mode e bypass: apvts.getParameter (id) com dynamic_cast para AudioParameterChoice / AudioParameterBool.
    //   Faz isto aqui, uma vez: procurar por ID (strings) no processBlock seria lento.

    jassert (gain != nullptr && time != nullptr && feedback != nullptr && tone != nullptr
             && mix != nullptr && mode != nullptr && bypass != nullptr);
}

bool EchoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Por agora só estéreo → estéreo. O exercício 35 alarga isto.
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void EchoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 }, currentSettings());
}

EchoEngine::Settings EchoProcessor::currentSettings() const noexcept
{
    EchoEngine::Settings s;

    // TODO(3): preenche `s` com os valores atuais dos parâmetros, lendo os atomics com .load()
    //   (o host e a GUI podem estar a escrevê-los noutra thread). pingPong: mode->getIndex() == 1.

    return s;
}

void EchoProcessor::processEcho (juce::AudioBuffer<float>& buffer) noexcept
{
    engine.process (buffer, currentSettings());
}

void EchoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Canais de saída sem entrada correspondente: limpa-os (como no template do Projucer).
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // TODO(4): se o parâmetro bypass estiver ligado, deixa o buffer como está (sai já daqui).

    processEcho (buffer);
}
