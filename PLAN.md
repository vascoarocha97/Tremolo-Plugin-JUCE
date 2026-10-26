# PLAN — Plataforma para aprender JUCE e DSP de áudio

> Documento de planeamento. Juntamente com `PROGRESS.md` e `DECISIONS.md`, permite retomar o trabalho
> a qualquer momento.

## 1. Versões fixadas

| Componente | Versão | Onde está fixada | Verificado em |
|---|---|---|---|
| JUCE | **9.0.4** (tag `9.0.4`, commit `3fb5cbc0340758596dac39f7c8de47d562b4896e`, publicado a 2026-10-12) | `cmake/LearnDependencies.cmake` | API do GitHub (releases) a 2026-10-26 |
| Catch2 | **v3.16.1** (publicado a 2026-10-08) | `cmake/LearnDependencies.cmake` | API do GitHub (releases) a 2026-10-26 |
| CMake | ≥ 3.22 (exigido pelo JUCE). Testado com 4.4.3 | `CMakeLists.txt` | `docs/CMake API.md` do JUCE |
| C++ | C++20 (JUCE 9 exige ≥ 17) | `CMakeLists.txt` | `minimumCppStandard: 17` nos headers dos módulos |
| Compilador | MSVC 19.4x (Visual Studio 2022 17.14) | — | máquina local |
| Gerador | Ninja Multi-Config | `CMakePresets.json` | — |
| Node.js | ≥ 20 (testado com 24.14) | `web/package.json` (`engines`) | — |
| Vite | 7.x + TypeScript 5.x | `web/package.json` | npm |

As tags `8.0.16-security` e `7.0.13-security` (2026-10-13) são correções de segurança para ramos
antigos; a versão estável mais recente da linha principal é a 9.0.4.

## 2. Estrutura do repositório

```
/
├── README.md              setup exato (Windows + VS Code + MSVC), como usar
├── PLAN.md                este ficheiro
├── PROGRESS.md            estado do trabalho (feito / falta / próximo passo)
├── DECISIONS.md           decisões tomadas e porquê
├── SOURCES.md             todas as fontes, por módulo, com licenças
├── CLAUDE.md              regras do modo tutor para o Claude Code
├── READY.md               (no fim) como começar, o que foi construído, limitações
├── curriculum.json        fonte de verdade: módulos, exercícios, páginas web
├── learn.cmd / learn      wrappers → scripts/learn.ps1
├── scripts/learn.ps1      ./learn check NN | check all | run NN | web | list
├── CMakeLists.txt         projeto único
├── CMakePresets.json      preset "default" (Ninja Multi-Config, MSVC x64)
├── cmake/
│   ├── LearnDependencies.cmake   FetchContent: JUCE 9.0.4 + Catch2 v3.16.1
│   ├── LearnJuceModules.cmake    biblioteca estática única com os módulos JUCE
│   └── LearnExercise.cmake       learn_add_exercise(): _dsp, _tests, _app
├── shared/
│   ├── testing/           helpers Catch2: sinais, medições, deteção de alocações
│   └── host/              app standalone partilhada (dispositivo áudio, fontes de teste,
│                          parâmetros, osciloscópio, espectro, limitador de proteção)
├── exercises/
│   └── NN-nome/
│       ├── README.md      a lição (PT-PT) + link para a web app + "Para saber mais"
│       ├── Processor.h/.cpp   código DSP com // TODO
│       ├── tests/         testes Catch2 offline
│       └── app/           wrapper AudioProcessor + main do standalone (não é para editar)
├── web/                   Vite + TypeScript, uma página por conceito, lê progress.json
├── .claude/skills/        /tutor e /explica
├── complete/, todo/       curso oficial do tremolo (pré-existente, não tocado)
```

Branches:
- `main` — exercícios com `TODO`. Compila; os testes de cada exercício falham até o aluno resolver.
- `solutions` — `main` + soluções de referência nos mesmos ficheiros. Nunca é fundido em `main`.

## 3. Arquitetura de build

- **JUCE compilado uma vez**: `learn_juce_modules` é uma biblioteca estática que liga todos os módulos
  necessários (`juce_audio_utils` + `juce_dsp`, e dependências) — padrão sugerido por um developer do
  JUCE no fórum. Todos os alvos de exercícios ligam a esta biblioteca e nunca diretamente aos módulos.
- Por exercício `NN`:
  - `exNN_dsp` — biblioteca estática só com o código DSP (`Processor.cpp`).
  - `exNN_tests` — executável Catch2 (testes + helpers), liga `exNN_dsp`. Registado no CTest.
  - `exNN_app` — `juce_add_gui_app` com o host partilhado + `app/ExerciseApp.cpp`. `EXCLUDE_FROM_ALL`.
- Exceção (módulos 8 e 10): os exercícios que geram VST3 usam `juce_add_plugin`, que compila os módulos
  com as definições do plugin. Esses alvos ficam fora do `ALL` e só são compilados com `./learn run`.
- Configuração por omissão: `Debug` (asserts do JUCE ativos). O módulo 9 (performance) usa `Release`.

## 4. Convenções de código DSP

- Módulo 1–2 (até ao 08): a classe DSP imita a interface do `juce::AudioProcessor`:
  `prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)`, `processBlock (juce::AudioBuffer<float>&)`,
  `releaseResources()`. Isto liga diretamente ao ciclo de vida de um plugin.
- A partir do 09: convenção dos processadores `juce::dsp`:
  `prepare (const juce::dsp::ProcessSpec&)`, `process (const juce::dsp::ProcessContextReplacing<float>&)`,
  `reset()`. É o que permite compor blocos com `juce::dsp::ProcessorChain`.
- Um bloco é escrito à mão primeiro; nos exercícios seguintes usa-se o equivalente do JUCE
  (ex.: delay à mão no 10–12 → `juce::dsp::DelayLine` no 13+). Assim nenhum exercício revela a solução
  de um anterior.
- Quando um exercício precisa de código de um anterior que não existe no JUCE (ex.: comb/allpass do 27
  no 28), o esqueleto pede explicitamente para copiar a solução do aluno.

## 5. Currículo (um exercício por linha)

Tempo estimado entre parênteses. "Testes" resume o que os testes verificam.

### Módulo 1 — Fundamentos
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 01 | `01-first-buffer` | `AudioBuffer`: canais × amostras; ganho fixo de 0.5 (5 min) | cada amostra × 0.5, todos os canais, blocos de 1 a 4096 |
| 02 | `02-sine-oscillator` | sample rate, fase, acumulador de fase (15 min) | frequência (Goertzel), continuidade entre blocos de tamanho variável, mudança de sample rate |
| 03 | `03-nyquist-aliasing` | Nyquist, frequência de alias, dente de serra ingénuo (20 min) | fórmula de folding; energia medida na frequência de alias |
| 04 | `04-bit-depth` | quantização, LSB, SNR ≈ 6.02N + 1.76 dB, dither TPDF (25 min) | nº de níveis, erro ≤ LSB/2, SNR, dither não silencia sinais pequenos |
| 05 | `05-realtime-rules` | regras de tempo-real: sem alocações, locks, I/O no `processBlock` (25 min) | zero alocações no `processBlock`, valor do medidor via `std::atomic` |

### Módulo 2 — Ganho, dB, pan, smoothing
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 06 | `06-decibels` | dB ↔ ganho linear, piso −∞ (15 min) | valores conhecidos, comparação com `juce::Decibels` |
| 07 | `07-pan-laws` | leis de pan: linear, potência constante (−3 dB), −4.5 dB (20 min) | ganhos no centro/extremos, soma de potência constante |
| 08 | `08-parameter-smoothing` | porque há clicks; rampa linear por amostra (30 min) | salto máximo entre amostras, chega ao alvo em N amostras, re-alvo a meio |
| 09 | `09-smoothed-gain` | `juce::SmoothedValue`, `std::atomic` entre threads, convenção `juce::dsp` (30 min) | sem clicks, sem alocações, escrita concorrente do parâmetro |

### Módulo 3 — Delay
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 10 | `10-circular-buffer` | buffer circular, delay inteiro (30 min) | delay de exatamente N amostras, qualquer tamanho de bloco, sem alocações |
| 11 | `11-fractional-delay` | interpolação linear, delay fracionário (30 min) | impulso com 2.25 amostras → pesos 0.75/0.25; fase de um seno |
| 12 | `12-feedback-echo` | feedback, dry/wet, estabilidade (30 min) | ecos com amplitude fb^k, saída limitada com fb=0.95, mix=0 → dry exato |
| 13 | `13-pingpong-delayline` | `juce::dsp::DelayLine`, ping-pong estéreo (30 min) | ecos alternam L/R nos tempos certos |

### Módulo 4 — Filtros
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 14 | `14-one-pole` | filtro de um polo LP/HP, constante de tempo (30 min) | ganho DC, resposta ao degrau (63.2% em τ), atenuação crescente |
| 15 | `15-biquad-rbj` | biquad TDF-II + coeficientes do Audio EQ Cookbook (45 min) | \|H(f0)\| conforme o cookbook; igual a `juce::dsp::IIR::Coefficients` |
| 16 | `16-svf-tpt` | State Variable Filter TPT (Zavalishin/Simper) (45 min) | respostas LP/BP/HP; estável com cutoff modulado por amostra |
| 17 | `17-juce-dsp-chain` | `ProcessorChain`, `ProcessorDuplicator`, `IIR::ArrayCoefficients` (40 min) | resposta da cadeia; atualizar coeficientes sem alocar |

### Módulo 5 — Modulação
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 18 | `18-lfo` | LFO: seno, triângulo, dente de serra, quadrada; fase (30 min) | período, gamas, offset de fase, continuidade |
| 19 | `19-tremolo` | modulação de amplitude, profundidade, estéreo (25 min) | envelope mínimo/máximo, sem alocações |
| 20 | `20-chorus` | delay modulado + interpolação (40 min) | gama do delay, mix=0 → dry, modulação presente |
| 21 | `21-flanger` | filtro pente, notches, feedback (40 min) | posição dos notches com delay fixo, estabilidade |

### Módulo 6 — Distorção
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 22 | `22-waveshaper` | hard/soft clip, tanh, assimetria e harmónicos (30 min) | limites, simetria ⇒ sem harmónicos pares; assimetria ⇒ 2.º harmónico |
| 23 | `23-fuzz` | cadeia de fuzz: pré-ênfase, clip assimétrico, DC blocker, tom (40 min) | média ≈ 0 (DC removido), saída limitada |
| 24 | `24-bitcrusher` | redução de bits e de sample rate (sample & hold) (30 min) | níveis distintos, retenção durante N amostras |
| 25 | `25-measuring-aliasing` | FFT, janelas, medir aliasing (45 min) | seno puro ⇒ aliasing baixo; seno com hard clip ⇒ alto |
| 26 | `26-oversampling` | `juce::dsp::Oversampling`, latência (45 min) | aliasing reduzido ≥ 20 dB a 4×, latência reportada, sem alocações |

### Módulo 7 — Reverb
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 27 | `27-comb-allpass` | comb com feedback e allpass de Schroeder (40 min) | resposta impulsional; allpass com magnitude plana |
| 28 | `28-schroeder-reverb` | Schroeder/Freeverb, RT60 → ganho (45 min) | RT60 medido ±20%, estéreo descorrelacionado |
| 29 | `29-fdn` | Feedback Delay Network, matriz de Householder (60 min) | matriz ortogonal (energia conservada), RT60 |
| 30 | `30-diffuser-reverb` | difusor multicanal (Hadamard) + FDN (60 min) | densidade de ecos, decaimento |
| 31 | `31-shimmer` | pitch shift por delay no ciclo de feedback (60 min) | seno 440 Hz → pico em 880 Hz; ciclo estável |

### Módulo 8 — Plugin real
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 32 | `32-apvts-parameters` | `AudioProcessorValueTreeState`, `ParameterLayout`, `ParameterID` versionado (40 min) | IDs, gamas, defaults, ponteiros atómicos |
| 33 | `33-state-and-presets` | `get/setStateInformation`, presets (45 min) | round-trip de estado, carregar preset |
| 34 | `34-editor-attachments` | GUI: `SliderAttachment`, layout, `ButtonAttachment` (45 min) | o editor altera os parâmetros |
| 35 | `35-vst3-plugin` | `juce_add_plugin`, bus layouts, bypass, latência, VST3 (60 min) | mini-validação estilo pluginval |

### Módulo 9 — Performance e otimização
| # | Pasta | Conceito | Testes (Release) |
|---|---|---|---|
| 36 | `36-measuring-cpu` | medir tempo por bloco vs orçamento real-time (40 min) | igual à referência e ≥ 3× mais rápido |
| 37 | `37-simd` | `FloatVectorOperations`, `dsp::SIMDRegister` (45 min) | igual ao escalar; mais rápido |
| 38 | `38-avoid-work` | denormais, coeficientes só quando mudam, silêncio (40 min) | sem denormais, nº de recálculos, cauda |

### Módulo 10 — Projeto final: pedal
| # | Pasta | Conceito | Testes |
|---|---|---|---|
| 39 | `39-glitch` | stutter/beat-repeat com crossfades (60 min) | repetição exata, sem descontinuidades |
| 40 | `40-pedal` | pedal completo escrito do zero: fuzz (oversampled) + crush + glitch + reverb, APVTS, GUI, VST3 (várias horas) | caixa-preta: parâmetros, estado, sem alocações, sem NaN, bypass |

## 6. Web app (`web/`)

- Vite + TypeScript, sem framework nem backend. Router por hash (`#/aliasing`). Porta fixa 5173.
- Página inicial: currículo + progresso (lê `/progress.json`, servido pelo Vite a partir da raiz do repo,
  com polling a cada 5 s).
- Uma página por conceito (Canvas 2D + Web Audio), cada uma com fontes:
  `sampling`, `nyquist-aliasing`, `bit-depth`, `buffers`, `realtime`, `decibels`, `pan-laws`, `smoothing`,
  `delay-line`, `fractional-delay`, `feedback-delay`, `one-pole`, `biquad`, `svf`, `lfo`,
  `chorus-flanger`, `waveshaping`, `aliasing-distortion` (ver secção 10), `oversampling`, `bitcrush`, `fft`,
  `comb-allpass`, `reverb`, `fdn`, `shimmer`, `plugin-architecture`, `performance`, `pedal`.

## 7. Comando `learn`

- `./learn check NN` — configura (se preciso), compila `exNN_tests`, corre, mostra resultado em PT,
  atualiza `progress.json`.
- `./learn check all` — o mesmo para todos.
- `./learn run NN` — compila e abre o standalone.
- `./learn web` — instala dependências (1.ª vez) e arranca a web app.
- `./learn list` — currículo com estado.

## 8. Fontes por módulo (resumo — detalhe em `SOURCES.md`)

| Módulo | Fontes principais |
|---|---|
| 1 | JOS *Mathematics of the DFT* (amostragem, Nyquist); Analog Devices MT-001 (SNR 6.02N+1.76); Ross Bencina "Real-time audio programming 101"; Timur Doumler & Fabian Renn-Giles, ADC "Real-time 101"; docs JUCE `AudioBuffer`, `AudioProcessor` |
| 2 | docs JUCE `Decibels`, `SmoothedValue`, `dsp::Panner`; CMU pan laws; fórum JUCE (zipper noise) |
| 3 | JOS *Physical Audio Signal Processing* (delay lines, interpolação); docs JUCE `dsp::DelayLine` |
| 4 | JOS *Introduction to Digital Filters*; RBJ Audio EQ Cookbook (W3C); Zavalishin *The Art of VA Filter Design*; Cytomic SVF; docs JUCE `dsp::IIR`, `StateVariableTPTFilter`, `ProcessorChain` |
| 5 | JOS PASP (flanger, chorus); docs JUCE `dsp::Oscillator`, `dsp::Chorus` |
| 6 | JOS (nonlinearidades); DAFx papers sobre aliasing (Parker et al. 2016, ADAA); docs JUCE `dsp::Oversampling`, `dsp::FFT`, `WindowingFunction`; chowdsp_utils |
| 7 | JOS PASP (Schroeder, Freeverb, FDN); Dattorro "Effect Design"; Valhalla DSP blog; Signalsmith "Let's write a reverb" (ADC21) |
| 8 | tutoriais JUCE (APVTS, estado, plugin), exemplos `examples/Plugins`, pluginval (Tracktion) |
| 9 | docs JUCE `FloatVectorOperations`, `SIMDRegister`, `ScopedNoDenormals`; de Soras "Denormal numbers"; ADC talks sobre performance |
| 10 | tudo o anterior |

## 9. Fases

- **Fase 0**: scaffolding (CMake, presets, helpers de teste, host), `learn`, `CLAUDE.md`, `/tutor`, `/explica`, exercício 01 completo.
- **Fase 1**: módulos 1–3 (exercícios 02–13).
- **Fase 2**: web app com as visualizações dos módulos 1–3 + progresso.
- **Fases 3–9**: módulos 4–10, cada um com exercícios + visualizações.
- **Fase final**: revisão independente (subagente), correções, teste de clone limpo, `READY.md`.

Critérios de "fase concluída": no `solutions` todos os testes passam; no `main` tudo compila e os testes
de cada exercício novo falham; cada standalone abre e produz som; a web app compila e lê
`progress.json`; o README funciona a partir de um clone limpo.

## 10. Desvios ao plano (registados durante a construção)

O plano acima ficou como foi escrito no início. O que mudou, e porquê (detalhe em `DECISIONS.md`):

- **Página `aliasing-distortion`** não existe à parte: o aliasing causado pela distorção é mostrado nas
  páginas `waveshaping` (espectro com riscas dobradas), `fft` (medir) e `oversampling` (corrigir). Uma
  página separada repetiria as mesmas visualizações.
- **Reutilizar código de exercícios anteriores**: em vez de "o esqueleto pede para copiar a solução
  do aluno" (secção 4), os exercícios que dependem de outros ligam ao código do aluno com
  `learn_add_exercise(... DEPENDS NN)` (ex.: 19–21 usam o LFO do 18; 28 usa o comb/allpass do 27; 30–31
  a FDN do 29; 33–35 o eco do 32). Menos cópias; consequência: no `main`, um exercício só passa
  depois de o anterior estar feito (os READMEs avisam).
- **Módulo 8** é um único plugin de eco construído em quatro passos (32 → 35), com o DSP já feito, para
  o foco ficar na arquitetura de plugin.
- **Exercício 40** não tem TODOs: tem uma especificação no README e testes de caixa preta; os ficheiros
  da pasta entram no build por *glob*, para o aluno organizar o código como quiser.
- **Alvos dos plugins** chamam-se `exNN_plugin_VST3` / `exNN_plugin_Standalone` (função
  `learn_add_plugin`); o `./learn smoke` dos plugins verifica o `.vst3` e que o Standalone do JUCE abre.
- **Host partilhado** ganhou, a meio, gravação do estado do processador ao fechar, caixa de presets
  (programas) e a chamada a `editorBeingDeleted()` exigida pelo JUCE 9.
- **Módulo 9** compila em Release (`"config": "Release"` no `curriculum.json`); os testes de velocidade
  só exigem a aceleração em Release (em Debug mostram um aviso).
- **Fontes**: o PDF original do livro do Zavalishin no site da Native Instruments deu 404; usa-se a
  revisão 2.1.2 num espelho (ver `SOURCES.md`).
