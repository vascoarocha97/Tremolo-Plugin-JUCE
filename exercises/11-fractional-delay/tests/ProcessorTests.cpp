#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>
#include <numbers>

using namespace learn::test;

namespace
{
    /** Passa o sinal por uma FractionalDelayLine com atraso constante. */
    std::vector<float> delayConstant (const std::vector<float>& input, float delay, int maxDelay = 64)
    {
        FractionalDelayLine line;
        line.prepare (maxDelay);
        std::vector<float> output;

        for (const auto x : input)
        {
            line.push (x);
            output.push_back (line.readLinear (delay));
        }

        return output;
    }
}

TEST_CASE ("readLinear com atraso inteiro é igual a read", "[11]")
{
    FractionalDelayLine line;
    line.prepare (32);

    for (int i = 1; i <= 50; ++i)
    {
        line.push (static_cast<float> (i * i));

        for (int d = 0; d <= 32; ++d)
        {
            INFO ("push " << i << ", atraso " << d);
            REQUIRE (line.readLinear (static_cast<float> (d)) == line.read (d));
        }
    }
}

TEST_CASE ("Um impulso atrasado 2.25 amostras divide-se em 0.75 e 0.25", "[11]")
{
    std::vector<float> impulse (10, 0.0f);
    impulse[0] = 1.0f;

    const auto out = delayConstant (impulse, 2.25f);

    REQUIRE_THAT (out[1], WithinAbs (0.0, 1.0e-7));
    REQUIRE_THAT (out[2], WithinAbs (0.75, 1.0e-6));
    REQUIRE_THAT (out[3], WithinAbs (0.25, 1.0e-6));
    REQUIRE_THAT (out[4], WithinAbs (0.0, 1.0e-7));
}

TEST_CASE ("Uma rampa atrasada 0.5 amostras fica a meio caminho", "[11]")
{
    std::vector<float> ramp;

    for (int i = 0; i < 20; ++i)
        ramp.push_back (static_cast<float> (i));

    const auto out = delayConstant (ramp, 3.5f);

    for (int n = 4; n < 20; ++n)
        REQUIRE_THAT (out[(size_t) n], WithinAbs (n - 3.5, 1.0e-5));
}

TEST_CASE ("Um seno sai com a fase certa (atraso de 10.5 amostras)", "[11]")
{
    constexpr double fs = 48000.0, f = 1000.0;
    std::vector<float> sine;

    for (int n = 0; n < 2000; ++n)
        sine.push_back (static_cast<float> (std::sin (2.0 * std::numbers::pi * f * n / fs)));

    const auto out = delayConstant (sine, 10.5f);

    // A interpolação linear atenua um pouco (ver o teste seguinte): |H| = cos(π·f/fs) para meia amostra.
    const auto gain = std::cos (std::numbers::pi * f / fs);

    for (int n = 100; n < 2000; ++n)
    {
        const auto expected = gain * std::sin (2.0 * std::numbers::pi * f * (n - 10.5) / fs);
        INFO ("amostra " << n);
        REQUIRE_THAT (out[(size_t) n], WithinAbs (expected, 1.0e-4));
    }
}

TEST_CASE ("A interpolação linear é um passa-baixo (máximo a meia amostra)", "[11]")
{
    constexpr double fs = 48000.0;

    for (const double f : { 1000.0, 10000.0, 20000.0 })
    {
        std::vector<float> sine;

        for (int n = 0; n < 4800; ++n)
            sine.push_back (static_cast<float> (std::sin (2.0 * std::numbers::pi * f * n / fs)));

        const auto out = delayConstant (sine, 0.5f);
        const auto measured = amplitudeAt (out.data() + 100, 4700, f, fs);
        const auto expected = std::cos (std::numbers::pi * f / fs); // ≈ 0.998, 0.793, 0.259

        INFO ("f = " << f << " Hz");
        REQUIRE_THAT (measured, WithinAbs (expected, 0.01));
    }
}

TEST_CASE ("FractionalDelay: 10 ms a 48 kHz são 480 amostras", "[11]")
{
    FractionalDelay delay;
    delay.setDelayMilliseconds (10.0f);
    delay.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeImpulse (2, 1024, 0);
    processInBlocks (delay, buffer, awkwardBlockSizes());

    for (int ch = 0; ch < 2; ++ch)
    {
        REQUIRE_THAT (buffer.getSample (ch, 480), WithinAbs (1.0, 1.0e-3));
        REQUIRE (firstSampleAbove (buffer, ch, 0.01f) == 480);
    }
}

TEST_CASE ("FractionalDelay: mudar o atraso desliza (sem saltos na forma de onda)", "[11]")
{
    FractionalDelay delay;
    delay.setDelayMilliseconds (10.0f);
    delay.prepare ({ 48000.0, 512, 1 });

    auto buffer = makeSine (1, 48000, 200.0, 48000.0, 0.5f);

    for (int block = 0; block < 48000 / 480; ++block)
    {
        // Muda de 10 ms para 30 ms e volta, várias vezes
        delay.setDelayMilliseconds ((block / 10) % 2 == 0 ? 10.0f : 30.0f);
        processRange (delay, buffer, block * 480, 480);
    }

    // Um seno de 200 Hz com amplitude 0.5 varia no máximo ~0.013 por amostra. Com o atraso a
    // deslizar, a velocidade muda um pouco (efeito Doppler), mas nunca há um salto.
    // Sem smoothing, um salto de 20 ms no atraso daria saltos até ~1.
    REQUIRE (maxSampleJump (buffer, 0, 600) < 0.03f);
}

TEST_CASE ("FractionalDelay::process não aloca", "[11]")
{
    FractionalDelay delay;
    delay.prepare ({ 48000.0, 512, 2 });
    delay.setDelayMilliseconds (123.4f);

    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (delay, buffer, sizes); }) == 0);
}
