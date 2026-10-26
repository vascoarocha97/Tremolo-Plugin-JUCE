---
name: explica
description: Explica em profundidade um conceito de áudio/DSP/JUCE (ex. aliasing, biquad, SmoothedValue), ligado aos exercícios do curso Learn JUCE e com fontes verificadas.
argument-hint: <conceito>
disable-model-invocation: true
allowed-tools: Read Grep Glob WebFetch WebSearch
---

# /explica — $ARGUMENTS

Explica **"$ARGUMENTS"** em profundidade, em **português de Portugal**, a um programador experiente
(C#/VB.NET) que está a aprender DSP e JUCE. Segue o `CLAUDE.md` (Regra 1: não resolver exercícios;
Regra 2: fontes verificadas).

## Passos

1. **Situa o conceito no curso.** Procura em `curriculum.json` e nos `exercises/*/README.md` onde o
   conceito aparece (pode ser mais do que um exercício) e em que página da web app (o `id` da
   página está no `curriculum.json`, campo `web`; **não** leias o código em `web/src/` — é spoiler). Diz em que exercício(s) é usado e o que o aluno já devia saber antes.

2. **Explica por camadas:**
   - **Intuição** — uma imagem mental ou analogia (de preferência ligada a jogos/áudio, se fizer sentido).
   - **Matemática** — as fórmulas essenciais, com as variáveis explicadas e um exemplo numérico.
   - **Na prática** — onde aparece em plugins reais, armadilhas conhecidas (o fórum do JUCE é uma boa
     fonte), custos de CPU e tempo-real.
   - **No JUCE 9.0.4** — que classes/funções existem para isto. **Confirma cada uma** nos headers em
     `build/_deps/juce-src/modules/` (ou na documentação oficial) antes de a mencionar, e dá o link
     `https://docs.juce.com/master/...`. Se houver diferenças para versões antigas, consulta o
     `BREAKING_CHANGES.md` do JUCE.

3. **Código de exemplo**: podes mostrar exemplos pequenos e genéricos, **desde que não sejam a solução
   de um TODO de nenhum exercício**. Se o conceito for exatamente o que um exercício pede, explica a
   ideia e remete para `/tutor NN`.

4. **Fontes**: termina com "Para saber mais" — 3 a 6 links reconhecidos (Julius O. Smith/CCRMA, Audio
   EQ Cookbook, Ross Bencina, Valhalla DSP, Signalsmith, talks da ADC, documentação/tutoriais JUCE,
   chowdsp_utils, DAFx...), cada um com uma frase sobre o que cobre. Verifica que os links existem
   (WebFetch) quando não vierem de `SOURCES.md`. Não copies excertos longos: resume por palavras tuas.

5. **Incertezas**: se alguma afirmação não estiver confirmada numa fonte, marca-a explicitamente
   ("não confirmei isto numa fonte").

Termina sugerindo a página da web app relevante (`http://localhost:5173/#/<página>`) e o exercício
onde o conceito se pratica.
