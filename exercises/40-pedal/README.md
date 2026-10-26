# 40 · O pedal

> **Módulo 10 — Projeto final** · ⏱ várias horas · Visualização: [O pedal](http://localhost:5173/#/pedal)

## Objetivo

Escrever **do zero** um pedal de efeitos completo, como plugin VST3 (e Standalone):

```
entrada ─► FUZZ (oversampling) ─► CRUSH ─► GLITCH ─► REVERB ─► SAÍDA ─► (bypass sem cliques) ─► saída
```

Desta vez não há TODOs: há uma **especificação** (abaixo) e **testes de caixa preta** que a verificam
como um host faria — só pela interface `juce::AudioProcessor` e pelos IDs dos parâmetros. Como
organizas o código por dentro é contigo.

Reaproveita o **teu** código: o fuzz e o oversampling dos exercícios 22–26, o crush do 24, o glitch do
39, a reverb dos 29–31, os parâmetros/estado/editor/bypass dos 32–35, e o que aprendeste no módulo 9.
Copia os ficheiros para esta pasta (qualquer `.h`/`.cpp` aqui, fora de `tests/`, entra no build;
renomeia as classes se colidirem).

## Especificação

### A classe

`Processor.h` declara `class PedalProcessor : public juce::AudioProcessor`, com construtor sem
argumentos. `PluginEntry.cpp` (já feito) cria-a.

### Parâmetros

Doze parâmetros, com exatamente estes IDs (versão 1), tipos, gamas e valores por omissão:

| ID | Nome | Tipo | Gama | Omissão | Função |
|---|---|---|---|---|---|
| `fuzz_on` | Fuzz | bool | | **true** | liga o fuzz |
| `fuzz_drive` | Drive | float | 0 … 40 dB | 20 | ganho antes da saturação |
| `fuzz_tone` | Tom | float | 500 … 12000 Hz | 4000 | passa-baixo depois do fuzz |
| `crush_on` | Crush | bool | | false | liga o bitcrusher |
| `crush_bits` | Bits | int | 2 … 16 | 8 | resolução da quantização |
| `crush_downsample` | Downsample | int | 1 … 32 | 1 | *sample & hold* de N amostras |
| `glitch_on` | Glitch | bool | | false | repete a última fatia enquanto ligado |
| `glitch_slice` | Fatia | float | 20 … 1000 ms | 125 | comprimento da fatia |
| `reverb_mix` | Reverb | float | 0 … 1 | 0.25 | mistura seco/reverb (0 = sem reverb) |
| `reverb_decay` | Decay | float | 0.2 … 20 s | 2.5 | RT60 da reverb |
| `output` | Saída | float | −24 … +12 dB | 0 | ganho final |
| `bypass` | Bypass | bool | | false | bypass (também o do host: `getBypassParameter`) |

### Comportamento verificado pelos testes

1. **Parâmetros** como na tabela; `getBypassParameter()` devolve o `bypass`.
2. **Estado**: `getStateInformation` / `setStateInformation` repõem todos os parâmetros noutra instância;
   dados inválidos não podem crashar.
3. **Latência**: com tudo desligado (fuzz off, crush off, glitch off, reverb_mix 0, saída 0 dB), um
   impulso sai na posição `getLatencySamples()` (±1 amostra) com amplitude > 0.7. A latência **não
   muda** quando o fuzz liga — o caminho do sinal tem a mesma latência com e sem fuzz.
4. **Fuzz**: num seno de 220 Hz (amplitude 0.5, drive 30 dB, tom no máximo) o 3.º harmónico fica acima de
   −30 dBc; com o fuzz desligado, abaixo de −60 dBc. Num seno de 5 kHz com drive 30 dB, o alias do 7.º
   harmónico (a 13 kHz) fica abaixo de −40 dBc → precisas de **oversampling** (exercício 26).
5. **Crush**: com 4 bits, a saída tem no máximo 17 valores diferentes (e pelo menos 4); com downsample 8,
   a saída só muda de 8 em 8 amostras.
6. **Glitch**: com `glitch_on`, a saída fica periódica, com período igual a `glitch_slice` (±10⁻⁴).
7. **Reverb**: com `reverb_mix` 0.5, um impulso deixa cauda; com decay 4 s, a energia entre 1.0 e 1.5 s
   é pelo menos 20 dB maior do que com decay 0.5 s.
8. **Bypass**: depois do crossfade (≤ 100 ms), a saída é a entrada **atrasada pela latência**
   (diferença < 10⁻⁵), sem saltos na transição.
9. **Robustez** (mini-pluginval): estéreo aceite, 5.1 recusado; a 44.1 e 96 kHz, com blocos de 1 a 2×
   o tamanho prometido e automação aleatória: sem alocações no `processBlock`, sem NaN/inf, pico < 8.
10. **Editor**: `hasEditor()`, e o editor abre (> 100×100 px) e fecha. Com reverb, `getTailLengthSeconds() > 0`.

### Sugestões de desenho (não são testadas)

- **Oversampling** com `juce::dsp::Oversampling` (factor 4×), filtros `filterHalfBandFIREquiripple`
  (fase linear: um impulso continua um impulso, atrasado) e `useIntegerLatency = true`. Passa **sempre**
  por ele (com o fuzz desligado, simplesmente não satures lá dentro): latência constante.
  `setLatencySamples` no `prepareToPlay`.
- **Bypass** com crossfade entre o processado e o seco **atrasado** por uma `juce::dsp::DelayLine` de
  `latência` amostras (exercício 35) — senão, ao ligar o bypass, o som "salta" para trás no tempo.
- **Glitch** antes da reverb: a cauda da reverb disfarça as junções. A reverb usa o difusor + FDN do
  exercício 30 (alocar as linhas no `prepare`).
- **Ordem dos `SmoothedValue`**: o do drive corre à taxa do oversampling.
- Um **editor** com uma secção por efeito; `GroupComponent` ou simplesmente desenhar caixas no `paint`.

### Experimentar

```powershell
./learn check 40   # os testes (Debug)
./learn run 40     # compila VST3 + Standalone e abre o Standalone
```

O Standalone do JUCE não tem fonte de teste: usa a entrada da placa de som, que vem **silenciada** por
omissão (para evitar realimentação microfone → colunas). Em *Options → Audio/MIDI Settings*, desliga
"Mute audio input" — com auscultadores. Para um DAW, copia a pasta `.vst3` de
`build\exercises\40-pedal\ex40_plugin_artefacts\Debug\VST3\` para `%LOCALAPPDATA%\Programs\Common\VST3`.

## Armadilhas comuns

- **Latência que muda** com o fuzz on/off (oversampling só quando o fuzz está ligado).
- **Filtros IIR no oversampling**: fase não linear — o impulso "espalha-se" e o teste da latência falha.
- **`std::vector::resize` / `new` no `processBlock`** (o teste de alocações apanha).
- **Bypass sem atrasar o seco**: salto no tempo e um clique.
- **Glitch e blocos maiores do que o prometido**: o histórico tem de ter folga (ou processa aos pedaços,
  como no exercício 35).

## Para saber mais

- [juce::dsp::Oversampling — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html) — tipos de filtro, latência inteira.
- [Tutorial JUCE: Saving and loading your plug-in state](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) — APVTS e estado.
- [pluginval (Tracktion)](https://github.com/Tracktion/pluginval) — corre-o no teu VST3 antes de o dares a alguém.
- [JUCE — CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md) — se quiseres acrescentar formatos (AU no macOS, etc.).
- Os READMEs dos exercícios 22–39: são a tua documentação.
