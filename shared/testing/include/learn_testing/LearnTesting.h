#pragma once

// Inclui todos os helpers de teste. Cada ficheiro de testes faz:
//
//     #include <learn_testing/LearnTesting.h>
//
// e fica com Catch2, sinais de teste, medições e deteção de alocações.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "BlockProcessing.h"
#include "Measurements.h"
#include "RealtimeChecks.h"
#include "Signals.h"

using Catch::Approx;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
