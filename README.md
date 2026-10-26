# Learn JUCE — aprender JUCE e DSP de áudio a fundo

Plataforma para aprender o que se passa "under the hood" num plugin de áudio: amostras, buffers,
sample rate, bit depth, filtros, delays, modulação, distorção, aliasing, oversampling, reverb, e
depois o plugin real (parâmetros, estado, GUI, VST3), performance e um projeto final.

Duas partes no mesmo repositório:

1. **Exercícios C++/JUCE** (`exercises/`) — 40 exercícios pequenos e progressivos. Cada um tem uma
   lição (`README.md`), código com `// TODO` para completares, testes que verificam o resultado com
   buffers offline e um standalone para ouvires o que escreveste.
2. **Web app de teoria** (`web/`) — uma página interativa por conceito (com som), ligada às lições,
   que mostra o teu progresso.

**Versão do JUCE: 9.0.4** (fixada por tag em `cmake/LearnDependencies.cmake`; ver `PLAN.md`).
Catch2 v3.16.1 para os testes. Tudo o resto em `PLAN.md` e `DECISIONS.md`.

---

## 1. O que instalar (Windows)

| Ferramenta | Versão | Notas |
|---|---|---|
| **Visual Studio 2022** (Community chega) ou **Build Tools for Visual Studio 2022** | 17.10 ou mais recente (testado com 17.14) | Workload **"Desktop development with C++"**. Garante que estão marcados: *MSVC v143*, *Windows 11 SDK* e **"C++ CMake tools for Windows"** (traz o CMake e o **Ninja**). |
| **Git for Windows** | qualquer recente | https://git-scm.com/download/win |
| **CMake** | ≥ 3.25 | Já vem com o Visual Studio (3.31). Se instalares à parte (https://cmake.org/download), também serve (testado com 4.4). |
| **VS Code** | recente | Extensões: **CMake Tools** (`ms-vscode.cmake-tools`) e **C/C++** (`ms-vscode.cpptools`). O repositório recomenda-as automaticamente. |
| **Node.js LTS** | ≥ 20 (testado com 24) | Só para a web app. https://nodejs.org |

Não precisas de abrir o "Developer PowerShell": o comando `learn` encontra o Visual Studio com o
`vswhere` e ativa o ambiente do compilador sozinho.

## 2. Primeiros passos

```powershell
git clone <url-do-teu-repositório> "Learn JUCE"
cd "Learn JUCE"
./learn check 01
```

A primeira execução:
1. configura o CMake com o preset `default` (Ninja Multi-Config + MSVC x64);
2. descarrega o JUCE 9.0.4 e o Catch2 para `build/_deps` (alguns minutos, depende da ligação);
3. compila os módulos do JUCE **uma única vez** numa biblioteca estática (~2–4 min em Debug);
4. compila e corre os testes do exercício 01 — que **falham** até completares os TODOs.

Depois:

```powershell
./learn run 01      # abre o standalone do exercício 01
./learn web         # web app em http://localhost:5173 (instala as dependências na 1.ª vez)
```

## 3. Comandos

| Comando | O que faz |
|---|---|
| `./learn check 03` | compila e corre os testes do exercício 03, mostra o resultado e atualiza `progress.json` |
| `./learn check all` | o mesmo para todos os exercícios, com um resumo no fim |
| `./learn check 03 -v` | mostra também o output completo do Catch2 |
| `./learn run 03` | compila e abre o standalone do exercício 03 |
| `./learn web` | arranca a web app de teoria |
| `./learn list` | currículo com o estado de cada exercício |
| `./learn smoke 03` | abre o standalone ~3 s e verifica que sai som (usado na autoverificação) |

Funciona no PowerShell (`./learn` resolve para `learn.cmd`) e no Git Bash (`./learn`).
O `learn.cmd` chama `scripts/learn.ps1` com `-ExecutionPolicy Bypass` só para esse processo, porque
no Windows a política de execução por omissão bloqueia scripts `.ps1`.

## 4. Fluxo de trabalho num exercício

1. Lê `exercises/NN-nome/README.md` (a lição) e abre a visualização indicada na web app.
2. Abre `Processor.h` / `Processor.cpp` e completa os `// TODO`.
3. `./learn check NN` até tudo passar. Lê as mensagens: dizem o valor obtido e o esperado.
4. `./learn run NN` e ouve. O standalone tem fonte de teste (loop, seno, varrimento, ruído, impulsos,
   ficheiro ou entrada da placa), **Bypass (A/B)**, osciloscópio, espectro (entrada a cinzento, saída a
   azul) e um **limitador de proteção** (corta acima de 0 dBFS e silencia NaN/inf).
5. Encravado? No Claude Code: `/tutor NN` (pistas por níveis, sem dar a solução) ou
   `/explica <conceito>`.

## 5. Abrir no VS Code

1. `File → Open Folder...` e escolhe a pasta do repositório.
2. Aceita instalar as extensões recomendadas.
3. Na barra do CMake Tools escolhe o *configure preset* **"MSVC x64 (Ninja Multi-Config)"**
   (o CMake Tools ativa o ambiente do Visual Studio a partir do preset) e corre **CMake: Configure**.
4. Escolhe o *build preset* `debug` e o alvo, por exemplo `ex03_tests` ou `ex03_app`, e **Build**.
5. Os testes aparecem no painel **Testing** do VS Code (via CTest).
6. Para depurar: seleciona o *launch target* no CMake Tools e usa a configuração
   "Depurar alvo selecionado (CMake)" (F5).

O `./learn` e o VS Code partilham a mesma pasta `build/`.

## 6. Estrutura

```
exercises/NN-nome/
  README.md            a lição (teoria, porquê, armadilhas, "Para saber mais")
  Processor.h/.cpp     o teu código, com // TODO
  tests/               testes Catch2 offline (não precisam de placa de som)
  app/ExerciseApp.cpp  liga o teu processador ao standalone (não precisas de mexer)
shared/testing/        helpers de teste: sinais, medições, deteção de alocações
shared/host/           o standalone partilhado
cmake/                 dependências, biblioteca única do JUCE, learn_add_exercise()
web/                   web app (Vite + TypeScript)
curriculum.json        o currículo (usado pelo learn, pelo CMake e pela web app)
```

Alvos CMake de cada exercício: `exNN_dsp` (o teu código DSP), `exNN_tests` (testes, compilam só o
DSP) e `exNN_app` (standalone). Todos ligam à biblioteca `learn_juce_modules`, por isso o JUCE é
compilado uma vez. Exceção: os exercícios 35 e 40 geram um VST3 com `juce_add_plugin`, que compila o
JUCE com as definições do plugin (ver `DECISIONS.md`).

## 7. Branches e soluções

- `main` — os exercícios com `TODO`. É aqui que trabalhas.
- `solutions` — soluções de referência, nos mesmos ficheiros. **Não está no `main`, nem no histórico.**

Para comparar a tua solução com a de referência de um exercício (só depois de o fazeres!):

```powershell
git diff main solutions -- exercises/03-nyquist-aliasing
```

## 8. Documentos do projeto

| Ficheiro | Conteúdo |
|---|---|
| `PLAN.md` | estrutura, stack, currículo detalhado, fontes por módulo |
| `DECISIONS.md` | decisões tomadas e porquê |
| `PROGRESS.md` | estado da construção da plataforma |
| `SOURCES.md` | todas as fontes usadas, por módulo, e licenças |
| `CLAUDE.md` | regras do modo tutor para o Claude Code |
| `READY.md` | resumo final: como começar, o que foi construído, limitações |

## 9. Curso do tremolo (pré-existente)

As pastas `complete/` e `todo/` são o projeto do curso oficial do JUCE (tremolo), que já estava neste
repositório. Não fazem parte desta plataforma, não são compiladas pelo `CMakeLists.txt` da raiz e
não foram alteradas. O README original está em `TREMOLO_COURSE_README.md`.

## 10. Licenças

- O JUCE é distribuído sob **AGPLv3** ou sob a licença comercial do JUCE 9
  (https://juce.com/legal/juce-9-licence/). Para distribuir plugins de código fechado precisas de
  uma licença adequada — vê https://juce.com/get-juce.
- Catch2: Boost Software License 1.0.
- Fontes e código de terceiros referenciados: ver `SOURCES.md`.
