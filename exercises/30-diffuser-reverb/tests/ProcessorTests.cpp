#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;

    double energy (const Frame& v)
    {
        double s = 0.0;
        for (const auto x : v) s += static_cast<double> (x) * x;
        return s;
    }
}

TEST_CASE ("Hadamard normalizada: [1,0,...,0] -> todos 1/√8, energia preservada, H·H = I", "[30]")
{
    Frame unit {};
    unit[0] = 1.0f;
    Mixing::hadamard (unit);

    for (const auto x : unit)
        REQUIRE_THAT (x, WithinAbs (1.0 / std::sqrt (8.0), 1.0e-6));

    Frame second {};
    second[1] = 1.0f;
    Mixing::hadamard (second);
    REQUIRE_THAT (second[0], WithinAbs (1.0 / std::sqrt (8.0), 1.0e-6));
    REQUIRE_THAT (second[1], WithinAbs (-1.0 / std::sqrt (8.0), 1.0e-6)); // sinais diferentes

    juce::Random random (9);

    for (int trial = 0; trial < 100; ++trial)
    {
        Frame v;
        for (auto& x : v) x = 2.0f * random.nextFloat() - 1.0f;
        const auto original = v;

        Mixing::hadamard (v);
        REQUIRE_THAT (energy (v), WithinRel (energy (original), 1.0e-5));
        Mixing::hadamard (v);

        for (size_t i = 0; i < v.size(); ++i)
            REQUIRE_THAT (v[i], WithinAbs (original[i], 1.0e-5));
    }
}

TEST_CASE ("DiffusionStep: cada canal num segmento diferente do intervalo de atrasos", "[30]")
{
    DiffusionStep step;
    step.prepare (fs, 80.0f, 42);

    const auto segment = 80.0 * 0.001 * fs / 8.0; // 480 amostras
    int flipped = 0;

    for (int ch = 0; ch < 8; ++ch)
    {
        INFO ("canal " << ch);
        REQUIRE (step.getDelaySamples (ch) >= static_cast<int> (ch * segment));
        REQUIRE (step.getDelaySamples (ch) <= static_cast<int> ((ch + 1) * segment));
        flipped += step.isFlipped (ch) ? 1 : 0;
    }

    REQUIRE ((flipped > 0 && flipped < 8)); // alguns invertidos, não todos

    DiffusionStep same;
    same.prepare (fs, 80.0f, 42);
    REQUIRE (same.getDelaySamples (3) == step.getDelaySamples (3)); // a seed torna-o reprodutível
}

TEST_CASE ("Um passo de difusão não ganha nem perde energia", "[30]")
{
    DiffusionStep step;
    step.prepare (fs, 40.0f, 7);

    Frame impulse {};
    impulse[2] = 1.0f;
    double total = 0.0;

    for (int n = 0; n < 4000; ++n) // mais do que o atraso máximo (1920 amostras)
        total += energy (step.process (n == 0 ? impulse : Frame {}));

    REQUIRE_THAT (total, WithinAbs (1.0, 1.0e-5));
}

TEST_CASE ("4 passos transformam um impulso numa nuvem de ecos", "[30]")
{
    DiffuserReverb reverb;
    reverb.prepare ({ fs, 512, 2 });

    Frame impulse {};
    impulse[0] = 1.0f;
    int nonZero = 0;
    double total = 0.0;

    for (int n = 0; n < 24000; ++n) // 0.5 s (os 4 passos somam até 300 ms)
    {
        const auto out = reverb.diffuseOnly (n == 0 ? impulse : Frame {});
        if (std::abs (out[0]) > 1.0e-6f)
            ++nonZero;
        total += energy (out);
    }

    REQUIRE (nonZero > 300);                         // centenas de ecos distintos num só canal
    REQUIRE_THAT (total, WithinAbs (1.0, 1.0e-4));   // e a energia total é a do impulso
}

TEST_CASE ("Reverb completa: denso desde o início, decai, finita e sem alocar", "[30]")
{
    DiffuserReverb reverb;
    reverb.setDecaySeconds (2.0f);
    reverb.setMix (1.0f);
    reverb.prepare ({ fs, 512, 2 });

    auto buffer = makeImpulse (2, 3 * 48000, 0);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (reverb, buffer, sizes); }) == 0);
    REQUIRE (allFinite (buffer));

    // A mesma resposta só com a FDN do exercício 29, para comparar
    FdnReverb plain;
    plain.setDecaySeconds (2.0f);
    plain.setMix (1.0f);
    plain.prepare ({ fs, 512, 2 });
    auto plainBuffer = makeImpulse (2, 3 * 48000, 0);
    processInBlocks (plain, plainBuffer, sizes);

    auto countNonZero = [] (const juce::AudioBuffer<float>& b)
    {
        int count = 0;
        for (int n = 4800; n < 14400; ++n) // 100–300 ms
            if (std::abs (b.getSample (0, n)) > 1.0e-7f)
                ++count;
        return count;
    };

    const auto diffused = countNonZero (buffer);
    const auto sparse = countNonZero (plainBuffer);
    INFO ("amostras com som entre 100 e 300 ms: com difusor " << diffused << ", só FDN " << sparse << " (de 9600)");

    REQUIRE (diffused > 3000);       // uma "nuvem" logo nos primeiros 300 ms...
    REQUIRE (diffused > 20 * sparse); // ...onde a FDN sozinha ainda só tem ecos isolados
    REQUIRE (rms (buffer, 0, 2 * 48000, 4800) < 0.1 * rms (buffer, 0, 4800, 9600)); // e decaiu
}
