#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <cmath>

using namespace learn::test;
using Shapers::Shape;

namespace
{
    /** Seno de 1 kHz (amplitude 1) por uma curva com drive; devolve as amplitudes dos harmónicos 1..5 e o DC. */
    struct Harmonics
    {
        double h[6] {};
        double dc = 0.0;
    };

    Harmonics analyse (Shape shape, float driveDb)
    {
        Waveshaper shaper;
        shaper.setShape (shape);
        shaper.setDriveDecibels (driveDb);
        shaper.prepare ({ 48000.0, 4800, 1 });

        auto buffer = makeSine (1, 48000, 1000.0, 48000.0);
        processInBlocks (shaper, buffer, { 4800 });

        Harmonics result;

        for (int k = 1; k <= 5; ++k)
            result.h[k] = amplitudeAt (buffer, 0, 1000.0 * k, 48000.0);

        result.dc = mean (buffer, 0);
        return result;
    }
}

TEST_CASE ("hardClip: corta em ±1 e não mexe no resto", "[22]")
{
    REQUIRE (Shapers::hardClip (0.5f) == 0.5f);
    REQUIRE (Shapers::hardClip (-0.3f) == -0.3f);
    REQUIRE (Shapers::hardClip (2.0f) == 1.0f);
    REQUIRE (Shapers::hardClip (-7.0f) == -1.0f);
}

TEST_CASE ("softClip: cúbica até ±1, plana depois, sem saltos", "[22]")
{
    REQUIRE_THAT (Shapers::softClip (0.0f), WithinAbs (0.0, 1.0e-7));
    REQUIRE_THAT (Shapers::softClip (0.5f), WithinAbs (1.5 * (0.5 - 0.125 / 3.0), 1.0e-6));
    REQUIRE_THAT (Shapers::softClip (1.0f), WithinAbs (1.0, 1.0e-6));
    REQUIRE_THAT (Shapers::softClip (0.999f), WithinAbs (1.0, 1.0e-5)); // contínua em 1
    REQUIRE (Shapers::softClip (3.0f) == 1.0f);
    REQUIRE (Shapers::softClip (-3.0f) == -1.0f);

    // Para sinais pequenos é quase linear (inclinação 1.5 em 0)
    REQUIRE_THAT (Shapers::softClip (0.001f), WithinAbs (0.0015, 1.0e-6));
}

TEST_CASE ("tanh e assimétrica", "[22]")
{
    REQUIRE_THAT (Shapers::tanhClip (0.7f), WithinAbs (std::tanh (0.7), 1.0e-6));
    REQUIRE_THAT (Shapers::asymmetric (2.0f), WithinAbs (std::tanh (2.0), 1.0e-6));
    REQUIRE_THAT (Shapers::asymmetric (-0.1f), WithinAbs (0.5 * std::tanh (-0.2), 1.0e-6));
    REQUIRE_THAT (Shapers::asymmetric (-10.0f), WithinAbs (-0.5, 1.0e-4));
    REQUIRE_THAT (Shapers::asymmetric (1.0e-4f), WithinAbs (1.0e-4, 1.0e-7)); // inclinação 1 em 0
}

TEST_CASE ("As curvas simétricas são ímpares: f(−x) = −f(x)", "[22]")
{
    for (auto shape : { Shape::hardClip, Shape::softClip, Shape::tanh })
    {
        for (float x = -3.0f; x <= 3.0f; x += 0.137f)
        {
            INFO ("curva " << static_cast<int> (shape) << ", x = " << x);
            REQUIRE_THAT (Shapers::apply (shape, -x), WithinAbs (-Shapers::apply (shape, x), 1.0e-6));
        }
    }
}

TEST_CASE ("Simétrica => só harmónicos ímpares; assimétrica => também pares (e DC)", "[22]")
{
    const auto sym = analyse (Shape::tanh, 12.0f);
    REQUIRE (toDb (sym.h[3]) > -20.0);  // 3.º harmónico bem presente
    REQUIRE (toDb (sym.h[2]) < -80.0);  // 2.º praticamente inexistente
    REQUIRE (toDb (sym.h[4]) < -80.0);
    REQUIRE (std::abs (sym.dc) < 1.0e-4);

    const auto asym = analyse (Shape::asymmetric, 12.0f);
    REQUIRE (toDb (asym.h[2]) > -30.0); // agora há 2.º harmónico
    REQUIRE (asym.dc > 0.05);           // e um desvio DC: o lado positivo chega a +1, o negativo só a −0.5
}

TEST_CASE ("Hard clip tem mais harmónicos altos do que o tanh (é mais \"áspero\")", "[22]")
{
    const auto hard = analyse (Shape::hardClip, 12.0f);
    const auto soft = analyse (Shape::tanh, 12.0f);
    REQUIRE (hard.h[5] > soft.h[5]);
}

TEST_CASE ("Waveshaper: drive e saída em dB, saída limitada para qualquer entrada", "[22]")
{
    Waveshaper shaper;
    shaper.setShape (Shape::hardClip);
    shaper.setDriveDecibels (6.0206f);  // ×2
    shaper.setOutputDecibels (-6.0206f); // ×0.5
    shaper.prepare ({ 48000.0, 512, 2 });

    auto buffer = makeConstant (2, 512, 0.25f);
    processWhole (shaper, buffer);
    REQUIRE_THAT (buffer.getSample (0, 100), WithinAbs (0.25, 1.0e-4)); // 0.5·hard(2·0.25)

    auto loud = makeConstant (2, 512, 1.0e6f);
    processWhole (shaper, loud);
    REQUIRE (peak (loud, 1) <= 0.5f + 1.0e-5f);
}

TEST_CASE ("Waveshaper::process não aloca", "[22]")
{
    Waveshaper shaper;
    shaper.prepare ({ 48000.0, 512, 2 });
    shaper.setDriveDecibels (20.0f);
    auto buffer = makeNoise (2, 4096);
    const auto sizes = awkwardBlockSizes();
    REQUIRE (countAllocations ([&] { processInBlocks (shaper, buffer, sizes); }) == 0);
}
