#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <vector>

/** Exercício 36 — Medir CPU (e otimizar o que se mediu).

    Num callback de áudio de N amostras a fs Hz há N/fs segundos para fazer TUDO (o "orçamento").
    A carga é tempo gasto / orçamento. Acima de 1 (100%) o áudio falha: cliques e buracos.

    Este exercício corre em Release (curriculum.json): medir performance em Debug não diz nada. */

//==============================================================================
/** Medidor de carga: mede quanto tempo um bloco demorou e compara com o orçamento. As leituras
    (getLoad, getPeak) são atómicas: a GUI pode lê-las enquanto o áudio escreve. */
class CpuLoadMeter
{
public:
    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;

    /** Regista um bloco de `numSamples` amostras que demorou `secondsTaken` segundos a processar.
        carga = secondsTaken / (numSamples / sampleRate). A primeira medição define a carga; as
        seguintes suavizam: carga += 0.1 · (nova − carga). Também atualiza o pico e conta os blocos
        acima de 100% (overruns). */
    void addMeasurement (double secondsTaken, int numSamples) noexcept;

    float getLoad() const noexcept { return load.load(); }
    float getPeak() const noexcept { return peak.load(); }
    int getOverruns() const noexcept { return overruns.load(); }

    /** RAII: mede o tempo entre a construção e a destruição e chama addMeasurement. */
    class ScopedMeasurement
    {
    public:
        ScopedMeasurement (CpuLoadMeter& meterToUse, int numSamplesInBlock) noexcept;
        ~ScopedMeasurement() noexcept;

    private:
        CpuLoadMeter& meter;
        int numSamples;
        juce::int64 startTicks = 0;

        JUCE_DECLARE_NON_COPYABLE (ScopedMeasurement)
    };

private:
    double sampleRate = 44100.0;
    bool hasMeasurement = false;
    std::atomic<float> load { 0.0f }, peak { 0.0f };
    std::atomic<int> overruns { 0 };
};

//==============================================================================
/** Ganho de drive → passa-baixo de um polo → saturação cúbica → ganho de saída. */
struct DriveFilterSettings
{
    float driveDb = 12.0f;
    float cutoffHz = 3000.0f;
    float gainDb = -6.0f;
};

/** Saturação cúbica suave: x − (4/27)·x³ em [−1.5, 1.5], ±1 fora. Contínua e com derivada contínua. */
inline float softClip (float x) noexcept
{
    x = juce::jlimit (-1.5f, 1.5f, x);
    return x - (4.0f / 27.0f) * x * x * x;
}

/** A versão "ingénua", lenta, que serve de referência. NÃO EDITES: os testes comparam com ela. */
class NaiveDriveFilter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context, const DriveFilterSettings& settings) noexcept;

private:
    std::vector<float> state;
    double sampleRate = 44100.0;
};

/** A tua versão: o MESMO resultado (até à última casa decimal razoável), pelo menos 3× mais rápida. */
class FastDriveFilter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<float>& context, const DriveFilterSettings& settings) noexcept;

private:
    std::vector<float> state;
    double sampleRate = 44100.0;
};
