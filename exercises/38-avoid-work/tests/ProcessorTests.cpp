#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    /** Referência: o mesmo biquad, sem atalhos (processa sempre tudo), com os denormais desligados. */
    struct ReferenceLowpass
    {
        void setup (float cutoff, float q) { c = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (fs, cutoff, q); }

        void process (juce::AudioBuffer<float>& buffer, int start, int num)
        {
            const juce::ScopedNoDenormals noDenormals;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* x = buffer.getWritePointer (ch, start);
                auto& s = state[(size_t) ch];
                for (int n = 0; n < num; ++n)
                {
                    const auto in = x[n];
                    const auto y = c[0] * in + s[0];
                    s[0] = c[1] * in - c[4] * y + s[1];
                    s[1] = c[2] * in - c[5] * y;
                    x[n] = y;
                }
            }
        }

        std::array<float, 6> c {};
        std::array<std::array<float, 2>, 2> state {};
    };

    bool anySubnormal (const EfficientLowpass& f)
    {
        for (const auto& s : f.getState())
            for (const auto v : s)
                if (std::fpclassify (v) == FP_SUBNORMAL)
                    return true;
        return false;
    }
}

TEST_CASE ("Coeficientes: recalculados só quando o corte ou o Q mudam", "[38]")
{
    EfficientLowpass f;
    f.prepare ({ fs, 512, 2 });
    REQUIRE (f.getCoefficientUpdates() == 1); // o prepare calcula-os uma vez

    auto buffer = makeNoise (2, 512, 0.5f);
    juce::dsp::AudioBlock<float> block (buffer);

    for (int i = 0; i < 100; ++i) // o host chama os setters em todos os blocos, com o mesmo valor
    {
        f.setCutoff (1000.0f);
        f.setResonance (0.7071f);
        f.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
    REQUIRE (f.getCoefficientUpdates() == 1);

    for (int i = 0; i < 50; ++i) // automação: um valor novo por bloco
    {
        f.setCutoff (1000.0f + 10.0f * static_cast<float> (i + 1));
        f.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
    REQUIRE (f.getCoefficientUpdates() == 51);

    // Vários setters entre dois blocos = um só recálculo, no bloco seguinte
    f.setCutoff (300.0f);
    f.setCutoff (400.0f);
    f.setResonance (2.0f);
    REQUIRE (f.getCoefficientUpdates() == 51);
    f.process (juce::dsp::ProcessContextReplacing<float> (block));
    REQUIRE (f.getCoefficientUpdates() == 52);
}

TEST_CASE ("Continua a ser um passa-baixo: 100 Hz passa, 10 kHz cai ~40 dB", "[38]")
{
    EfficientLowpass f;
    f.setCutoff (1000.0f);
    f.prepare ({ fs, 512, 1 });

    for (const auto& [frequency, minDb, maxDb] : { std::tuple { 100.0, -0.5, 0.5 }, std::tuple { 10000.0, -45.0, -35.0 } })
    {
        INFO (frequency << " Hz");
        f.reset();
        const auto input = makeSine (1, 24000, frequency, fs, 0.5f);
        auto output = copyOf (input);
        processInBlocks (f, output, { 512 });
        const auto db = gainDb (input, output, 0, 4800);
        REQUIRE (db > minDb);
        REQUIRE (db < maxDb);
    }
}

TEST_CASE ("Depois de um impulso, o estado nunca fica desnormalizado e chega a zero", "[38]")
{
    EfficientLowpass f;
    f.setCutoff (200.0f);
    f.setResonance (4.0f); // ressonante: a cauda decai devagar
    f.prepare ({ fs, 512, 2 });

    auto buffer = makeImpulse (2, 512 * 400, 0); // ~4 s
    int blocksWithSubnormals = 0;

    for (int start = 0; start < buffer.getNumSamples(); start += 512)
    {
        processRange (f, buffer, start, 512);
        blocksWithSubnormals += anySubnormal (f) ? 1 : 0;
    }

    REQUIRE (blocksWithSubnormals == 0);
    for (const auto& s : f.getState())
    {
        REQUIRE (s[0] == 0.0f);
        REQUIRE (s[1] == 0.0f);
    }
}

TEST_CASE ("Silêncio: blocos saltados, e o resultado é igual ao de um filtro que processa tudo", "[38]")
{
    EfficientLowpass f;
    f.setCutoff (2000.0f);
    f.prepare ({ fs, 512, 2 });

    ReferenceLowpass reference;
    reference.setup (2000.0f, 0.7071f);

    // 0.5 s de ruído, ~4.5 s de silêncio, 0.5 s de ruído
    auto input = makeNoise (2, 512 * 516, 0.5f);
    for (int ch = 0; ch < 2; ++ch)
        input.clear (ch, 512 * 47, 512 * 422);

    auto actual = copyOf (input);
    auto expected = copyOf (input);

    for (int b = 0; b < 516; ++b)
    {
        if (b == 300) // o utilizador mexe no corte durante o silêncio
        {
            f.setCutoff (500.0f);
            reference.setup (500.0f, 0.7071f);
        }

        processRange (f, actual, b * 512, 512);
        reference.process (expected, b * 512, 512);
    }

    INFO ("blocos saltados: " << f.getSkippedBlocks() << " de 422 blocos de silêncio");
    REQUIRE (f.getSkippedBlocks() >= 380);
    REQUIRE (f.getSkippedBlocks() <= 422);
    REQUIRE (maxAbsDifference (expected, actual) < 1.0e-6f);
    REQUIRE (rms (actual, 0, 512 * 470) > 0.01); // o som voltou, com o corte novo
}

TEST_CASE ("Nada aloca", "[38]")
{
    EfficientLowpass f;
    f.prepare ({ fs, 512, 2 });
    auto buffer = makeNoise (2, 512 * 8, 0.5f);
    const std::vector<int> sizes { 512 }; // criado fora: o próprio vector alocaria

    REQUIRE (countAllocations ([&] {
        f.setCutoff (3000.0f);
        processInBlocks (f, buffer, sizes);
    }) == 0);
}

TEST_CASE ("(Demonstração) quanto custam os denormais nesta máquina", "[38][demo]")
{
    // Um passa-baixo de um polo muito lento (a = 0.9999) depois de um impulso: o estado passa
    // quase todo o tempo na zona desnormalizada (e acaba preso no menor desnormal: 0.9999 · s
    // arredonda para s). Medimos o mesmo cálculo com e sem FTZ/DAZ.
    auto run = [] (bool flushToZero)
    {
        std::unique_ptr<juce::ScopedNoDenormals> guard;
        if (flushToZero)
            guard = std::make_unique<juce::ScopedNoDenormals>();

        volatile float input = 0.0f;
        float s = 1.0e-37f; // já perto da zona desnormalizada (abaixo de ~1.2e-38)
        const auto start = juce::Time::getHighResolutionTicks();
        for (int n = 0; n < 48000 * 4; ++n)
            s = 0.0001f * input + 0.9999f * s;
        const auto seconds = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - start);
        return std::pair { seconds, s };
    };

    const auto [withDenormals, s1] = run (false);
    const auto [withoutDenormals, s2] = run (true);

    WARN ("4 s de cauda de um filtro de um polo: com denormais " << withDenormals * 1000.0 << " ms, com FTZ/DAZ "
          << withoutDenormals * 1000.0 << " ms (" << withDenormals / std::max (withoutDenormals, 1.0e-9) << "x)");
    REQUIRE (std::isfinite (s1 + s2));
}
