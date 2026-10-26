# READY — a plataforma está pronta

## Os primeiros 3 passos

```powershell
./learn check 01     # 1.ª vez: descarrega o JUCE 9.0.4 e o Catch2 e compila (~3–5 min); os testes falham até completares os TODOs
./learn web          # a web app de teoria em http://localhost:5173 (instala as dependências na 1.ª vez)
./learn run 01       # o standalone do exercício 01, para ouvires o que escreveste
```

Depois: lê `exercises/01-first-buffer/README.md`, completa os `// TODO` em `Processor.cpp` e repete
`./learn check 01` até passar. Encravado? No Claude Code: `/tutor 01` (pistas por níveis, sem dar a
solução) ou `/explica <conceito>`. Pré-requisitos e detalhes: `README.md`.

## O que foi construído

- **40 exercícios** em 10 módulos (`curriculum.json`): fundamentos (buffers, amostragem, Nyquist,
  bit depth, tempo-real), ganho/dB/pan/smoothing, delay, filtros (one-pole, biquad RBJ, SVF TPT,
  cadeias `juce::dsp`), modulação (LFO, tremolo, chorus, flanger), distorção (waveshaper, fuzz,
  bitcrusher, FFT para medir aliasing, oversampling), reverb (comb/allpass, Schroeder, FDN, difusor,
  shimmer), plugin real (APVTS, estado/presets, editor com attachments, VST3), performance (medir CPU,
  SIMD, denormais/evitar trabalho) e o projeto final (glitch e o pedal fuzz + crush + glitch + reverb
  como VST3, escrito do zero contra uma especificação).
  Cada um: lição em PT-PT com "Para saber mais", esqueleto com TODOs, testes Catch2 offline
  (265 casos ao todo: resposta em frequência, tempos exatos de delay, cliques, RT60, alocações no
  `process`, ...), e um standalone com fonte de teste, osciloscópio, espectro, A/B e limitador.
- **Soluções de referência** no branch `solutions` (nunca no `main`, nem no histórico).
- **Web app** (`web/`, Vite + TypeScript, sem backend): 27 páginas interativas com som (Web Audio),
  fontes em cada página, e a página inicial com o teu progresso (lê `progress.json` ao vivo).
- **`./learn`**: `check NN|all`, `run NN`, `web`, `list`, `smoke`, que atualizam `progress.json`
  (feito / em curso / por começar).
- **Modo tutor**: `CLAUDE.md` + skills `/tutor` e `/explica`.
- **Documentação**: `PLAN.md` (com os desvios na secção 10), `DECISIONS.md` (29 decisões),
  `PROGRESS.md`, `SOURCES.md` (114 fontes, por módulo, e licenças).

## Como foi verificado

- No `solutions`: os 40 exercícios passam (`./learn check all`, 36–38 em Release); os 38 standalones
  abrem e produzem som (`./learn smoke all`); os VST3 do 35 e do 40 compilam, e o JUCE carrega-os ao
  gerar o `moduleinfo.json`; os Standalone abrem.
- No `main`: todos os esqueletos compilam e os testes de cada exercício falham.
- Clone limpo (`git clone` para outra pasta): o setup do README funciona — download do JUCE e do
  Catch2, compilação, `./learn check 01` (0/6, como deve), `progress.json` escrito, standalone com
  som, `npm ci` + `npm run build` da web app.
- Web app: as 28 rotas abrem sem erros de consola; os botões de som tocam; o progresso atualiza.
- Revisão independente por três subagentes (lições 1–5, lições 6–10, infraestrutura). Os achados
  foram corrigidos (commit "Correções da revisão independente"); os que não foram estão abaixo.

## Desvios ao pedido e porquê

- A página `aliasing-distortion` foi absorvida por `waveshaping`, `fft` e `oversampling` (seria
  repetida).
- Exercícios que precisam de código de um anterior usam o **teu** código desse exercício
  (`DEPENDS`), em vez de copiar: no `main`, um exercício só passa depois do anterior.
- O exercício 40 não tem TODOs: README = especificação, testes de caixa preta.
- Os plugins (35, 40) usam o Standalone do JUCE, não o standalone partilhado (sem fonte de teste nem
  limitador).
- Detalhe de tudo em `PLAN.md` (secção 10) e `DECISIONS.md`.

## Incertezas e limitações (lê isto)

- **Só testado nesta máquina** (Windows 11, Visual Studio 2022 Professional 17.14, CMake 4.4/3.31,
  Ninja, Node 24). Não testei Visual Studio Community/Build Tools sozinhos nem outro PC.
- **Os VST3 não foram testados num DAW nem com o pluginval** (seria preciso descarregar e executar
  um binário externo). A validação foi: carregamento pelo próprio JUCE no build, o Standalone a
  abrir, e testes que imitam parte do pluginval.
- **O código TypeScript da web app implementa a mesma matemática dos exercícios** (é o que desenha
  as visualizações). Está marcado como *spoiler* e o tutor está proibido de o ler, mas tu podes —
  não abras `web/src/lib` nem `web/src/pages` antes de fazeres os exercícios.
- **Testes de velocidade (36–37)** dependem da máquina e das opções do compilador; as margens aqui
  foram grandes (17× e 4× contra 3× e 2× exigidos).
- **Deteção de alocações**: em Release só vê `operator new`; por isso o `learn` repete os testes de
  alocações em Debug para os exercícios 36–38. Alocações noutras threads não são detetadas.
- **Fontes**: a página da Valhalla DSP bloqueia pedidos automáticos (verificada num browser). O PDF do
  livro do Zavalishin deu 404 no site da Native Instruments; usei a revisão 2.1.2 num espelho.
  A documentação online do JUCE (`docs.juce.com/master`) é do branch `master`; as afirmações das
  lições foram confirmadas nos headers da 9.0.4.
- **Plugins sem limitador**: um feedback ou fuzz mal feito no 35/40 pode sair muito alto. Volume baixo.
- O `.github/workflows/cmake.yml` é do curso do tremolo pré-existente e não testa esta plataforma.

## O que não fiz (de propósito) e recomendo

- **Push**: o `main` está no GitHub (`origin`, a teu pedido, a 2026-10-26). O branch `solutions`
  **não** foi enviado: existe só neste repositório local. Se um dia o quiseres no remoto:
  `git push origin solutions` (fica visível a quem tiver acesso ao repositório).
- **Permissões do Claude Code**: para reforçar a regra "não ler as soluções", podes acrescentar ao
  `.claude/settings.json` regras `permissions.deny` para `git show solutions*`, `git diff*solutions*`
  e `git checkout solutions*`. Não o fiz porque mexer em permissões é decisão tua.
- **Fora do repositório** ficaram ferramentas usadas na construção, que podes apagar:
  `C:\Vasco\Learn\sol` (worktree do `solutions`; antes de apagares, `git worktree remove
  C:\Vasco\Learn\sol`), `C:\Vasco\Learn\refs` (clones do JUCE 9.0.4 e Catch2 para consulta),
  `C:\Vasco\Learn\staging`, `C:\Vasco\Learn\tools` e `C:\Vasco\Learn\cleanclone`.
- A tua alteração em *stage* em `complete/CMakeLists.txt` ficou como estava (nenhum commit meu a
  inclui).
