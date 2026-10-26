// Wrapper do standalone. Não precisas de editar este ficheiro.
//
// O mesmo eco do exercício 32, agora com estado e presets. O standalone guarda o estado ao fechar
// (getStateInformation) e repõe-no ao abrir (setStateInformation): fecha e volta a abrir para ver.

#include <learn_host/HostApplication.h>

#include "Processor.h"

LEARN_EXERCISE_APP (StatefulEchoProcessor)
