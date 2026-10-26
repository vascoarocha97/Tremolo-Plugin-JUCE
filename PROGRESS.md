# PROGRESS — estado da construção da plataforma

> Atualizado no fim de cada fase. Para retomar: ler este ficheiro, `PLAN.md` e `DECISIONS.md`.

## Feito

- **Fase 0** (em curso)
  - Pesquisa: JUCE 9.0.4 (estável mais recente), Catch2 v3.16.1, padrão de biblioteca estática única.
  - `PLAN.md`, `DECISIONS.md`, `CLAUDE.md`, skills `/tutor` e `/explica`.
  - CMake: `LearnDependencies`, `LearnJuceModules`, `LearnExercise`, presets.
  - `shared/testing` (sinais, medições, deteção de alocações + autotestes).
  - `shared/host` (standalone partilhado com fonte de teste, análise, limitador, `--smoke-test`).
  - `learn` (`check`, `run`, `web`, `list`, `smoke`, `update-starter-hashes`).
  - Exercício 01 completo (lição, TODO, testes, standalone).

## Falta

- Fase 1: exercícios 02–13.
- Fase 2: web app (Vite + TS) com páginas dos módulos 1–3 + progresso.
- Fases 3–9: módulos 4–10.
- Revisão independente, teste de clone limpo, `READY.md`.

## Próximo passo

Commit da fase 0 no `main`; criar o branch `solutions` (worktree em `C:\Vasco\Learn\sol`).

## Notas de trabalho (para quem retoma)

- Ambiente de desenvolvimento usado: VS 2022 17.14 Professional, CMake 4.4.3, Ninja 1.13.2, Node 24.14.
- Soluções em preparação ficam em `C:\Vasco\Learn\staging\<pasta>\` antes de irem para o branch `solutions`.
- Clone de referência do JUCE 9.0.4 para ler headers: `C:\Vasco\Learn\refs\JUCE`.
- Fluxo por exercício: escrever solução + testes no `main` (sem commit) → validar → copiar solução
  para staging → escrever versão com TODO → `./learn check NN` tem de falhar → commit no `main` →
  no worktree `solutions`: `git merge main`, copiar a solução, `./learn check NN` passa → commit.
- Depois de mudar ficheiros com TODO: `./learn update-starter-hashes`.
