#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/** Exercício 32 — Parâmetros com AudioProcessorValueTreeState (APVTS).

    Módulo 8: vamos construir um plugin de eco completo, peça a peça:
      32  parâmetros (este)          33  estado e presets
      34  editor com attachments     35  plugin VST3 (junta tudo)

    O DSP do eco (EchoEngine) já está feito — é matéria do módulo 3. Aqui o foco é a ponte entre o
    host (e a GUI) e o código de áudio: os parâmetros. */

/** IDs dos parâmetros: strings que ficam gravadas nos projetos dos utilizadores.
    Nunca mudes um ID depois de publicares o plugin. */
namespace EchoParams
{
    inline constexpr auto gain     = "gain";
    inline constexpr auto time     = "time";
    inline constexpr auto feedback = "feedback";
    inline constexpr auto tone     = "tone";
    inline constexpr auto mix      = "mix";
    inline constexpr auto mode     = "mode";
    inline constexpr auto bypass   = "bypass";

    /** Versão dos parâmetros (ParameterID{ id, version }). Parâmetros acrescentados numa versão nova
        do plugin levam um número maior — o formato Audio Unit usa-o para manter a ordem. */
    inline constexpr int version = 1;
}

/** Cria todos os parâmetros do eco. É o argumento do construtor do APVTS. */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

//==============================================================================
/** O DSP: eco com feedback, filtro passa-baixo no ciclo ("tom") e modo ping-pong. Já feito. */
class EchoEngine
{
public:
    struct Settings
    {
        float gainDecibels = 0.0f;
        float timeMs = 350.0f;
        float feedback = 0.4f;
        float toneHz = 6000.0f;
        float mix = 0.3f;
        bool pingPong = false;
    };

    static constexpr float maxTimeMs = 1000.0f;

    void prepare (const juce::dsp::ProcessSpec& spec, const Settings& initial);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept;

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
    std::array<juce::dsp::FirstOrderTPTFilter<float>, 2> tone;
    juce::SmoothedValue<float> gain, timeSamples, feedback, mix;
    double sampleRate = 44100.0;
};

//==============================================================================
class EchoProcessor : public juce::AudioProcessor
{
public:
    EchoProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    const juce::String getName() const override { return "Eco"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }   // exercício 35

    int getNumPrograms() override { return 1; }                      // exercício 33
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}        // exercício 33
    void setStateInformation (const void*, int) override {}

    bool hasEditor() const override { return false; }                // exercício 34

    /** Os parâmetros e o seu estado. Público para o editor (34) e para os testes. */
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMETERS", createParameterLayout() };

protected:
    /** Lê os parâmetros e corre o eco (sem olhar para o bypass). O exercício 35 reutiliza-o. */
    void processEcho (juce::AudioBuffer<float>& buffer) noexcept;

    /** Os valores atuais dos parâmetros, lidos dos atomics. */
    EchoEngine::Settings currentSettings() const noexcept;

    // Ponteiros para os valores dos parâmetros, obtidos UMA vez no construtor.
    std::atomic<float>* gain = nullptr;
    std::atomic<float>* time = nullptr;
    std::atomic<float>* feedback = nullptr;
    std::atomic<float>* tone = nullptr;
    std::atomic<float>* mix = nullptr;
    juce::AudioParameterChoice* mode = nullptr;
    juce::AudioParameterBool* bypass = nullptr;

private:
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    EchoEngine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoProcessor)
};
