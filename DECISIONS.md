# DECISIONS — decisões tomadas e porquê

Formato: **Decisão** — porquê. Ordem cronológica.

## Fase 0

1. **JUCE 9.0.4, fixado por tag exata** — é a versão estável mais recente da linha principal (publicada a
   2026-10-12, confirmada na API de releases do GitHub a 2026-10-26). As tags `8.0.16-security` e
   `7.0.13-security` são só patches de segurança para ramos antigos.
2. **Catch2 v3.16.1** — versão estável mais recente; v3 compila como biblioteca (compila uma vez), tem
   reporters JUnit/XML que o script `learn` usa para mostrar resultados, e integra com CTest
   (`catch_discover_tests`) para o painel de testes do VS Code.
3. **FetchContent (e não CPM)** — vem com o CMake, não precisa de ficheiros extra, e é o mecanismo que o
   pedido sugere em primeiro lugar. `GIT_SHALLOW` para descarregar só a tag.
4. **Uma biblioteca estática `learn_juce_modules` com todos os módulos JUCE** — cumpre "JUCE só é
   compilado uma vez para todos os exercícios". É o padrão descrito por um developer do JUCE no fórum
   (thread 40492). Cuidados seguidos: todos os alvos ligam a esta biblioteca e nunca diretamente a
   módulos JUCE; o conjunto de módulos é fixo e conhecido à partida.
5. **Exceção: alvos VST3 (exercícios 35 e 40) usam `juce_add_plugin`** — o wrapper de plugin
   (`juce_audio_plugin_client`) precisa de ser compilado com as definições `JucePlugin_*` de cada plugin e
   puxa os módulos de que depende. Não há forma suportada de o ligar à biblioteca partilhada sem
   duplicar símbolos. Esses alvos ficam fora do `ALL` e só compilam com `./learn run 35|40`. Os testes
   desses exercícios continuam a usar a biblioteca partilhada.
6. **Standalone próprio (`shared/host`) em vez do wrapper Standalone do JUCE** — pela mesma razão do
   ponto 5. O host usa as mesmas peças que o wrapper oficial usa por dentro (`AudioDeviceManager` +
   `AudioProcessorPlayer`) e cada exercício expõe um `juce::AudioProcessor` real com parâmetros reais.
   Ganhamos ainda: fontes de teste (seno, ruído, impulsos, ficheiro, entrada), osciloscópio, espectro e
   um limitador de proteção (feedback mal feito não rebenta os ouvidos).
7. **Ninja Multi-Config + MSVC** — compilações incrementais muito mais rápidas e output de erros limpo.
   No VS Code o CMake Tools ativa o ambiente do Visual Studio a partir do preset
   (`toolset/architecture` com `strategy: external`). O `learn.ps1` importa o ambiente do
   `vcvars64.bat` (encontrado com `vswhere`). Ninja vem com o componente "C++ CMake tools for Windows".
8. **C++20** — o JUCE 9 exige ≥ 17; o 20 dá `std::numbers`, `std::span`, conceitos/`requires` (usados nos
   helpers de teste). É suportado pelo MSVC 2022.
9. **Duas convenções de interface DSP** — exercícios 01–08 imitam `AudioProcessor`
   (`prepareToPlay`/`processBlock`) para ligar ao ciclo de vida do plugin; a partir do 09 usa-se a
   convenção `juce::dsp` (`prepare(ProcessSpec)`/`process(ProcessContextReplacing)`/`reset`), que é a
   prática da indústria para blocos reutilizáveis e permite `ProcessorChain`.
10. **Soluções no branch `solutions`, nos mesmos ficheiros** — `git checkout solutions` mostra o código
    resolvido e `git diff main solutions -- exercises/NN-*` mostra só a solução. As soluções nunca são
    commitadas no `main` (nem no histórico). Para não revelar soluções de exercícios anteriores, um bloco
    escrito à mão é depois substituído pelo equivalente JUCE; quando não existe equivalente, o esqueleto
    pede para copiar o código do aluno.
11. **Curso do tremolo pré-existente (`complete/`, `todo/`) mantido intacto** — já estava no repositório
    (com alterações tuas em stage). Não é tocado nem incluído no CMake raiz. O README original passou a
    `TREMOLO_COURSE_README.md` para o `README.md` principal poder ter o setup desta plataforma.
    As minhas commits usam pathspecs para nunca incluir a tua alteração em stage em
    `complete/CMakeLists.txt`.
12. **Sem `git push`** — o remote `origin` é o teu repositório no GitHub; publicar é decisão tua.
13. **`learn.cmd` + `scripts/learn.ps1`** — a política de execução efetiva no Windows cliente é
    `Restricted` (verificado: `LocalMachine Undefined`), por isso `.\learn.ps1` falharia. O `learn.cmd`
    chama o PowerShell com `-ExecutionPolicy Bypass` só para esse processo. No PowerShell, `./learn`
    resolve para `learn.cmd`. Há também um `learn` (sh) para Git Bash.
14. **`progress.json` na raiz, ignorado pelo git** — é estado local (e seria diferente em cada branch).
    A web app lê-o através de um middleware do Vite, sem cache, com polling.
15. **"Em curso" via hash dos ficheiros iniciais** — `exercises/starter-hashes.json` guarda o SHA-256 dos
    ficheiros com TODO; um exercício está "em curso" quando não passa todos os testes mas os ficheiros
    já diferem do esqueleto, ou quando pelo menos um teste passa.
16. **Skills do Claude Code em `.claude/skills/<nome>/SKILL.md`** — formato recomendado na documentação
    atual (os ficheiros em `.claude/commands/` continuam a funcionar, mas as skills são o formato novo).
    `disable-model-invocation: true` para só correrem quando tu escreves `/tutor` ou `/explica`.
17. **Web app sem framework** — TypeScript + Canvas 2D + Web Audio. Menos dependências para manter e o
    código das visualizações é DSP legível (os mesmos algoritmos dos exercícios).
18. **Vite 8 + TypeScript 7** — versões estáveis mais recentes no npm a 2026-10-26.
19. **Estilo de código JUCE nos exercícios** — o `.clang-format` da raiz (estilo Chromium) é do curso do
    tremolo. `exercises/` e `shared/` têm um `.clang-format` próprio próximo do estilo do JUCE (Allman,
    4 espaços, espaço antes de parênteses), para o código parecer código JUCE.
