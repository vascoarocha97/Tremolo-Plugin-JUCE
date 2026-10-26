#include <learn_testing/LearnTesting.h>

#include "Processor.h"

using namespace learn::test;

TEST_CASE ("reset() define a duração da rampa e não cria rampa", "[08]")
{
    LinearSmoother s;
    s.setCurrentAndTargetValue (0.3f);
    s.reset (48000.0, 0.01);

    REQUIRE (s.getRampLengthInSamples() == 480);
    REQUIRE_FALSE (s.isSmoothing());
    REQUIRE (s.getNextValue() == 0.3f);
}

TEST_CASE ("Uma rampa de 4 amostras de 0 para 1: 0.25, 0.5, 0.75, 1", "[08]")
{
    LinearSmoother s;
    s.reset (4.0, 1.0); // 4 amostras
    s.setCurrentAndTargetValue (0.0f);
    s.setTargetValue (1.0f);

    REQUIRE (s.isSmoothing());
    REQUIRE_THAT (s.getNextValue(), WithinAbs (0.25, 1.0e-6));
    REQUIRE_THAT (s.getNextValue(), WithinAbs (0.5, 1.0e-6));
    REQUIRE_THAT (s.getNextValue(), WithinAbs (0.75, 1.0e-6));
    REQUIRE (s.getNextValue() == 1.0f);
    REQUIRE_FALSE (s.isSmoothing());
    REQUIRE (s.getNextValue() == 1.0f);
}

TEST_CASE ("Chega EXATAMENTE ao alvo ao fim de rampLength amostras", "[08]")
{
    LinearSmoother s;
    s.reset (48000.0, 0.05); // 2400 amostras
    s.setCurrentAndTargetValue (1.0f);
    s.setTargetValue (0.1234567f);

    for (int i = 0; i < 2399; ++i)
    {
        s.getNextValue();
        REQUIRE (s.isSmoothing());
    }

    REQUIRE (s.getNextValue() == 0.1234567f);
    REQUIRE_FALSE (s.isSmoothing());
}

TEST_CASE ("Mudar o alvo a meio começa uma rampa nova a partir do valor atual", "[08]")
{
    LinearSmoother s;
    s.reset (10.0, 1.0); // 10 amostras
    s.setCurrentAndTargetValue (0.0f);
    s.setTargetValue (1.0f);

    for (int i = 0; i < 5; ++i)
        s.getNextValue();

    REQUIRE_THAT (s.getCurrentValue(), WithinAbs (0.5, 1.0e-6));

    s.setTargetValue (0.0f); // volta para trás: 10 amostras de 0.5 até 0
    REQUIRE_THAT (s.getNextValue(), WithinAbs (0.45, 1.0e-6));

    for (int i = 0; i < 8; ++i)
        s.getNextValue();

    REQUIRE (s.getNextValue() == 0.0f);
}

TEST_CASE ("Alvo igual não reinicia a rampa; rampa de 0 amostras salta", "[08]")
{
    LinearSmoother s;
    s.reset (10.0, 1.0);
    s.setCurrentAndTargetValue (0.0f);
    s.setTargetValue (1.0f);
    s.getNextValue();
    s.getNextValue();
    s.setTargetValue (1.0f); // o mesmo alvo
    REQUIRE_THAT (s.getNextValue(), WithinAbs (0.3, 1.0e-6));

    LinearSmoother instant;
    instant.reset (48000.0, 0.0);
    instant.setTargetValue (0.7f);
    REQUIRE_FALSE (instant.isSmoothing());
    REQUIRE (instant.getNextValue() == 0.7f);
}

TEST_CASE ("skip(n) é igual a n chamadas de getNextValue()", "[08]")
{
    LinearSmoother a, b;
    a.reset (1000.0, 0.1);
    b.reset (1000.0, 0.1);
    a.setCurrentAndTargetValue (-1.0f);
    b.setCurrentAndTargetValue (-1.0f);
    a.setTargetValue (2.0f);
    b.setTargetValue (2.0f);

    a.skip (37);

    for (int i = 0; i < 37; ++i)
        b.getNextValue();

    REQUIRE_THAT (a.getCurrentValue(), WithinAbs (b.getCurrentValue(), 1.0e-5));

    a.skip (1000); // passa do fim
    REQUIRE (a.getCurrentValue() == 2.0f);
    REQUIRE_FALSE (a.isSmoothing());
}

TEST_CASE ("Comporta-se como juce::SmoothedValue<float, Linear>", "[08]")
{
    LinearSmoother mine;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> juceSmoother;

    mine.reset (48000.0, 0.02);
    juceSmoother.reset (48000.0, 0.02);
    mine.setCurrentAndTargetValue (0.5f);
    juceSmoother.setCurrentAndTargetValue (0.5f);

    juce::Random random (7);

    for (int i = 0; i < 20000; ++i)
    {
        if (i % 700 == 0)
        {
            const auto newTarget = random.nextFloat();
            mine.setTargetValue (newTarget);
            juceSmoother.setTargetValue (newTarget);
        }

        INFO ("amostra " << i);
        REQUIRE_THAT (mine.getNextValue(), WithinAbs (juceSmoother.getNextValue(), 1.0e-5));
    }
}

TEST_CASE ("SmoothedGainProcessor: um degrau de ganho vira uma rampa sem clicks", "[08]")
{
    SmoothedGainProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto buffer = makeConstant (2, 4800, 1.0f);
    processor.setGain (0.0f);
    processInBlocks (processor, buffer, awkwardBlockSizes());

    // 2400 amostras de rampa: cada passo é 1/2400. Sem smoothing, o salto seria 1.0.
    REQUIRE (maxSampleJump (buffer, 0) < 1.0f / 2400.0f + 1.0e-5f);
    REQUIRE_THAT (buffer.getSample (0, 1199), WithinAbs (0.5, 1.0e-3)); // a meio da rampa
    REQUIRE (buffer.getSample (0, 2399) == 0.0f);                       // fim da rampa
    REQUIRE (peak (buffer, 0, 2400) == 0.0f);
}

TEST_CASE ("SmoothedGainProcessor: todos os canais recebem o mesmo ganho em cada amostra", "[08]")
{
    SmoothedGainProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    processor.setGain (0.25f);

    auto buffer = makeConstant (3, 512, 1.0f);
    processor.processBlock (buffer);

    for (int n = 0; n < 512; ++n)
    {
        INFO ("amostra " << n);
        REQUIRE (buffer.getSample (1, n) == buffer.getSample (0, n));
        REQUIRE (buffer.getSample (2, n) == buffer.getSample (0, n));
    }

    // A velocidade da rampa não depende do número de canais
    REQUIRE_THAT (buffer.getSample (0, 239), WithinAbs (1.0 - 0.75 * 240.0 / 2400.0, 1.0e-4));
}

TEST_CASE ("SmoothedGainProcessor não aloca", "[08]")
{
    SmoothedGainProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    auto buffer = makeNoise (2, 512);

    REQUIRE (countAllocations ([&]
    {
        for (int i = 0; i < 20; ++i)
        {
            processor.setGain (i % 2 == 0 ? 0.2f : 0.9f);
            processor.processBlock (buffer);
        }
    }) == 0);
}
