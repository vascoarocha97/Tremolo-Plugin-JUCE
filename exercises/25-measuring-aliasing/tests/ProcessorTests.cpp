#include <learn_testing/LearnTesting.h>

#include "Processor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace learn::test;

namespace
{
    constexpr double fs = 48000.0;
    constexpr int order = 14; // 16384 pontos

    // Frequências que caem exatamente no centro de um bin (e os harmónicos também): sem "fugas"
    // da janela, o que sobra fora dos harmónicos é mesmo aliasing.
    // (calculadas aqui, e não com a tua binFrequency, para um erro nela não estragar os outros testes)
    constexpr double binWidth = fs / (1 << order);
    constexpr double f1k = 341 * binWidth;   // ≈ 999 Hz
    constexpr double f5k = 1707 * binWidth;  // ≈ 5001 Hz
    constexpr double f100 = 34 * binWidth;   // ≈ 99.6 Hz

    std::vector<float> clippedSine (double f0, float drive, bool hard)
    {
        std::vector<float> s ((size_t) 1 << order);

        for (size_t n = 0; n < s.size(); ++n)
        {
            const auto x = drive * static_cast<float> (std::sin (2.0 * std::numbers::pi * f0 * static_cast<double> (n) / fs));
            s[n] = hard ? juce::jlimit (-1.0f, 1.0f, x) : std::tanh (x);
        }

        return s;
    }
}

TEST_CASE ("binFrequency: k · fs / N", "[25]")
{
    REQUIRE_THAT (Spectrum::binFrequency (0, order, fs), WithinAbs (0.0, 1.0e-9));
    REQUIRE_THAT (Spectrum::binFrequency (1, order, fs), WithinAbs (fs / 16384.0, 1.0e-9));
    REQUIRE_THAT (Spectrum::binFrequency (8192, order, fs), WithinAbs (fs / 2.0, 1.0e-6));
}

TEST_CASE ("magnitudeDb: um seno de amplitude 1 no centro de um bin dá 0 dB nesse bin", "[25]")
{
    const auto bin = 1024; // 3 kHz exatos com 16384 pontos a 48 kHz
    const auto f = Spectrum::binFrequency (bin, order, fs);
    const auto sine = makeSine (1, 1 << order, f, fs, 1.0f);

    const auto db = Spectrum::magnitudeDb (sine.getReadPointer (0), order);
    REQUIRE (db.size() == 8192);
    REQUIRE_THAT (db[(size_t) bin], WithinAbs (0.0, 0.05));

    // Metade da amplitude: −6 dB
    const auto half = makeSine (1, 1 << order, f, fs, 0.5f);
    REQUIRE_THAT (Spectrum::magnitudeDb (half.getReadPointer (0), order)[(size_t) bin], WithinAbs (-6.02, 0.05));

    // O pico é mesmo nesse bin, e longe dele não há quase nada (é para isto que serve a janela)
    const auto maxIt = std::max_element (db.begin(), db.end());
    REQUIRE (std::distance (db.begin(), maxIt) == bin);
    REQUIRE (db[(size_t) bin + 50] < -90.0f);
}

TEST_CASE ("aliasingRatioDb: um seno puro quase não tem energia fora dos harmónicos", "[25]")
{
    const auto sine = makeSine (1, 1 << order, f1k, fs);
    REQUIRE (Spectrum::aliasingRatioDb (sine.getReadPointer (0), order, f1k, fs) < -90.0);
}

TEST_CASE ("aliasingRatioDb: harmónicos abaixo de Nyquist não contam como aliasing", "[25]")
{
    // Uma "onda quadrada" com banda limitada (só harmónicos ímpares abaixo de Nyquist)
    std::vector<float> square ((size_t) 1 << order, 0.0f);
    const double f0 = f1k;

    for (int k = 1; k * f0 < fs / 2; k += 2)
        for (size_t n = 0; n < square.size(); ++n)
            square[n] += static_cast<float> (4.0 / (std::numbers::pi * k) * std::sin (2.0 * std::numbers::pi * k * f0 * static_cast<double> (n) / fs));

    REQUIRE (Spectrum::aliasingRatioDb (square.data(), order, f0, fs) < -80.0);
}

TEST_CASE ("aliasingRatioDb: hard clip de um seno de 5 kHz tem muito aliasing; tanh menos", "[25]")
{
    const auto hard = clippedSine (f5k, 8.0f, true);
    const auto soft = clippedSine (f5k, 8.0f, false);

    const auto hardRatio = Spectrum::aliasingRatioDb (hard.data(), order, f5k, fs);
    const auto softRatio = Spectrum::aliasingRatioDb (soft.data(), order, f5k, fs);

    REQUIRE (hardRatio > -40.0);
    REQUIRE (softRatio < hardRatio);

    // E com um seno grave (100 Hz) os harmónicos que dobram já são fracos: menos aliasing
    const auto lowHard = clippedSine (f100, 8.0f, true);
    REQUIRE (Spectrum::aliasingRatioDb (lowHard.data(), order, f100, fs) < hardRatio - 10.0);
}
