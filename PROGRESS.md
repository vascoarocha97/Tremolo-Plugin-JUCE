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

- **Fase 2** ✔ — web app (`web/`, Vite 8.3.5 + TypeScript 7.0.2, sem framework): página inicial com
  progresso (lê `/progress.json` a cada 5 s via middleware do Vite) e 11 páginas dos módulos 1–3
  (`buffers`, `sampling`, `nyquist-aliasing`, `bit-depth`, `realtime`, `decibels`, `pan-laws`,
  `smoothing`, `delay-line`, `fractional-delay`, `feedback-delay`). Verificado: `npm run build`
  (tsc + vite) sem erros; todas as páginas abrem sem erros de consola; todos os botões ▶ geram e
  tocam áudio; o progresso atualiza ao vivo. Capturas: Edge headless.

- **Fases 3–5** ✔ — módulos 4 (filtros, 14–17), 5 (modulação, 18–21) e 6 (distorção, 22–26), com as páginas
  `one-pole`, `biquad`, `svf`, `lfo`, `chorus-flanger`, `waveshaping`, `bitcrush`, `fft`, `oversampling`.
  - `learn_add_exercise(... DEPENDS NN)`: exercícios que usam o código do aluno de outro exercício
    (19–21 usam o LFO do 18; 23 e 26 as curvas do 22; 24 o quantizador do 04). No `main`, estes só
    passam depois de o exercício de que dependem estar feito — testar as soluções sempre no worktree `solutions`.
  - Verificado no `solutions`: 26/26 exercícios passam; smoke test dos standalones 1–26 OK.

- **Fase 6** ✔ — módulo 7 (reverb, 27–31: comb/allpass, Schroeder, FDN, difusor + FDN, shimmer) e
  páginas `comb-allpass`, `reverb`, `fdn`, `shimmer` (blocos DSP em `web/src/lib/reverb.ts`).
  Verificado no `solutions`: 31/31; smoke 27–31 OK.

- **Fase 7** ✔ — módulo 8 (plugin real, 32–35: APVTS, estado/presets, editor com attachments, VST3) e
  página `plugin-architecture`. `learn_add_plugin` (VST3 + Standalone do JUCE, fora do ALL); host com
  estado persistente e caixa de presets. Verificado no `solutions`: 35/35; smoke 32–35 OK (o VST3
  compila e o JUCE carrega-o para gerar o `moduleinfo.json`; o Standalone abre).

- **Fase 8** ✔ — módulo 9 (performance, 36–38, Release: medir CPU, SIMD, evitar trabalho) e página
  `performance`. Verificado no `solutions` em Release; smoke OK.

- **Fase 9** ✔ — módulo 10 (39 glitch, 40 pedal escrito do zero com especificação + testes de caixa
  preta, VST3) e página `pedal`. Verificado no `solutions`: **40/40** exercícios passam; smoke 1–40 OK.

- **Documentação** ✔ — `SOURCES.md` completo (114 fontes, todas verificadas a 2026-10-26), desvios
  em `PLAN.md` (secção 10), `DECISIONS.md` 20–25, README com as regras dos exercícios especiais.

- **Fase final** ✔ — revisão independente por três subagentes (lições 1–5, lições 6–10,
  infraestrutura); achados corrigidos (commit "Correções da revisão independente") ou registados
  (`DECISIONS.md` 26–29, `READY.md`). Teste de clone limpo OK. `READY.md` escrito.

## Falta

Nada da construção. Melhorias possíveis estão em `READY.md` e `DECISIONS.md` 26.

## Notas de trabalho (para quem retoma)

- Ambiente de desenvolvimento usado: VS 2022 17.14 Professional, CMake 4.4.3, Ninja 1.13.2, Node 24.14.
- Soluções em preparação ficam em `C:\Vasco\Learn\staging\<pasta>\` antes de irem para o branch `solutions`.
- Clone de referência do JUCE 9.0.4 para ler headers: `C:\Vasco\Learn\refs\JUCE`.
- Fluxo por exercício: escrever solução + testes no `main` (sem commit) → validar → copiar solução
  para staging → escrever versão com TODO → `./learn check NN` tem de falhar → commit no `main` →
  no worktree `solutions`: `git merge main`, copiar a solução, `./learn check NN` passa → commit.
- Depois de mudar ficheiros com TODO: `./learn update-starter-hashes`.
