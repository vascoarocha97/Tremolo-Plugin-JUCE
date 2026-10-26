# PROGRESS — estado da construção da plataforma

> Atualizado no fim de cada fase. Para retomar: ler este ficheiro, `PLAN.md` e `DECISIONS.md`.

## Feito

- **Fase 0** ✔ (commit 39c56c4)
  - Pesquisa: JUCE 9.0.4 (estável mais recente), Catch2 v3.16.1, padrão de biblioteca estática única.
  - `PLAN.md`, `DECISIONS.md`, `CLAUDE.md`, skills `/tutor` e `/explica`.
  - CMake: `LearnDependencies`, `LearnJuceModules`, `LearnExercise`, presets.
  - `shared/testing` (sinais, medições, deteção de alocações + autotestes).
  - `shared/host` (standalone partilhado com fonte de teste, análise, limitador, `--smoke-test`).
  - `learn` (`check`, `run`, `web`, `list`, `smoke`, `update-starter-hashes`).
  - Exercício 01 completo (lição, TODO, testes, standalone).

- **Fase 1** ✔ — exercícios 02–13 (módulos 1–3): lição, TODO, testes, standalone, solução no
  branch `solutions`. Verificado: soluções passam 13/13; esqueletos compilam e falham; smoke test dos
  13 standalones OK (abrem e produzem som a 48 kHz).
  - Melhorias ao `learn`: reconfigura sozinho com exercícios novos; deteta crashes dos testes
    (sem janelas de erro do MSVC: `shared/testing/source/TestEnvironment.cpp`).

## Falta

- Fase 2: web app (Vite + TS) com páginas dos módulos 1–3 + progresso.
- Fases 3–9: módulos 4–10.
- Revisão independente, teste de clone limpo, `READY.md`.

## Próximo passo

Fase 2: web app (Vite 8 + TypeScript 7, sem framework) com as páginas dos módulos 1–3
(`buffers`, `sampling`, `nyquist-aliasing`, `bit-depth`, `realtime`, `decibels`, `pan-laws`,
`smoothing`, `delay-line`, `fractional-delay`, `feedback-delay`) e a página inicial com progresso.

## Notas de trabalho (para quem retoma)

- Ambiente de desenvolvimento usado: VS 2022 17.14 Professional, CMake 4.4.3, Ninja 1.13.2, Node 24.14.
- Soluções em preparação ficam em `C:\Vasco\Learn\staging\<pasta>\` antes de irem para o branch `solutions`.
- Clone de referência do JUCE 9.0.4 para ler headers: `C:\Vasco\Learn\refs\JUCE`.
- Fluxo por exercício: escrever solução + testes no `main` (sem commit) → validar → copiar solução
  para staging → escrever versão com TODO → `./learn check NN` tem de falhar → commit no `main` →
  no worktree `solutions`: `git merge main`, copiar a solução, `./learn check NN` passa → commit.
- Depois de mudar ficheiros com TODO: `./learn update-starter-hashes`.
