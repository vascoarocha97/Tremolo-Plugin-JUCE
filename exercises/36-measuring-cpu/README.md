# 36 · Medir CPU

> **Módulo 9 — Performance** · ⏱ ~40 min · Visualização: [Performance](http://localhost:5173/#/performance) · **compila em Release**

## Objetivos

- Perceber o **orçamento de tempo** de um callback de áudio e medir a carga de um processador.
- Escrever um medidor de carga com `juce::Time::getHighResolutionTicks` (e RAII).
- Otimizar uma função lenta **medindo**, sem mudar o resultado.

## Teoria

### O orçamento

Um bloco de N amostras a fs Hz representa N/fs segundos de som. O host precisa do bloco seguinte
antes de esse tempo acabar — senão a placa de som fica sem nada para tocar (*underrun*: clique, buraco).

| bloco | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| 64 | 1.45 ms | 1.33 ms | 0.67 ms |
| 512 | 11.6 ms | 10.7 ms | 5.3 ms |

E esse tempo é para **todos** os plugins de todas as pistas, mais o próprio host. A **carga** de um
processador é `tempo gasto / orçamento`. Uma carga média baixa não chega: um único bloco que demore
demasiado (por um `malloc`, um lock, um ficheiro...) já faz um clique — por isso também interessa o
**pico** e contar os blocos acima de 100%.

### Medir

`juce::Time::getHighResolutionTicks()` lê um contador de alta resolução (no Windows, o
`QueryPerformanceCounter`); `highResolutionTicksToSeconds` converte a diferença. Envolve-se o
processamento num objeto RAII que mede entre o construtor e o destrutor — o JUCE tem um igual,
`juce::AudioProcessLoadMeasurer::ScopedTimer`. Os valores para a GUI são `std::atomic`: a GUI lê-os noutra
thread.

### Medir em Release

Em Debug, o compilador não otimiza e o JUCE corre verificações (`jassert`): os números não dizem nada
sobre o plugin que os utilizadores vão correr. Os exercícios 36–38 estão marcados com
`"config": "Release"` no `curriculum.json` e o `./learn check` compila-os em Release.

### Otimizar: o que precisa mesmo de ser feito por amostra?

A `NaiveDriveFilter` (não a edites) faz, para cada amostra **e** canal: dois `std::pow`, um `std::exp`,
acesso por índice ao bloco, e o ciclo de canais por dentro do de amostras. O resultado está certo. A tua
`FastDriveFilter` começa igual; o objetivo é ser ≥ 3× mais rápida e dar o mesmo resultado (diferença
< 10⁻⁶). Perguntas para te guiares:

- O que depende só dos parâmetros (que não mudam dentro do bloco)?
- Em que ordem estão as amostras na memória? (`getChannelPointer`)
- O estado do filtro pode viver numa variável local durante o ciclo?

Não mudes as contas em si (ex.: `pow` por `exp`, `double` por `float`): o resultado tem de bater com a
referência. O teste mostra os tempos das duas versões quando falha.

## O exercício

Em `Processor.cpp`: **TODO(1)** `CpuLoadMeter::addMeasurement`, **TODO(2)** `ScopedMeasurement`,
**TODO(3)** `FastDriveFilter::process`.

## Nota sobre as medições

Os testes de velocidade assumem as opções de vírgula flutuante por omissão do MSVC (`/fp:precise`,
as deste projeto). Com `/fp:fast` (ou `-ffast-math` noutros compiladores) o compilador pode, sozinho,
tirar contas do ciclo ou vetorizar somas — a versão de referência fica mais rápida e a comparação
deixa de dizer respeito ao teu código.

## Armadilhas comuns

- **Medir em Debug** e tirar conclusões.
- **Medir uma vez só**: o sistema operativo interrompe; o teste usa o melhor de 5 corridas.
- **"Otimizar" mudando o resultado** (ex.: aproximações rápidas de `exp`): às vezes vale a pena, mas é
  uma decisão de som, não de performance — e aqui o teste exige o mesmo resultado.
- **Chamar coisas do sistema** (escrever num log, `std::cout`) para medir dentro do callback.

## Como verificar

```powershell
./learn check 36
./learn run 36     # sobe as "Instâncias" com a versão ingénua até a carga passar de 100%; muda para a tua
```

## Para saber mais

- Ross Bencina, ["Real-time audio programming 101: time waits for nothing"](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) — porque é que o pior caso importa mais do que a média.
- [juce::AudioProcessLoadMeasurer — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessLoadMeasurer.html) — o medidor do JUCE (usado pelo `AudioDeviceManager::getCpuUsage`).
- [juce::Time — documentação](https://docs.juce.com/master/classjuce_1_1Time.html) — `getHighResolutionTicks`, `highResolutionTicksToSeconds`.
- Agner Fog, [Software optimization resources](https://www.agner.org/optimize/) — manuais de referência sobre o custo de cada instrução e como otimizar C++.
