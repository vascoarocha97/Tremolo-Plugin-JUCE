# SOURCES — fontes usadas na plataforma

Todas as lições explicam por palavras próprias e ligam às fontes; não há excertos longos copiados.
Esta lista junta as secções "Para saber mais" dos READMEs e as "Fontes" das páginas da web app.
Uma fonte usada em vários módulos aparece no primeiro em que é usada (a coluna "Usado em" lista
todos os exercícios e páginas).
Todos os links foram verificados a **2026-10-26** (HTTP 200, exceto os da Valhalla DSP, que bloqueiam
pedidos automáticos com 403 mas abrem num browser — verificado no browser).

Cada afirmação sobre a API do JUCE foi confirmada nos headers/código da versão **9.0.4** (o clone em
`build/_deps/juce-src`). Quando a documentação online (`docs.juce.com/master`) e o código divergem,
vale o código da 9.0.4.

## Referências gerais do JUCE (versão 9.0.4)

| Fonte | O que cobre |
|---|---|
| [Repositório JUCE, tag 9.0.4](https://github.com/juce-framework/JUCE/tree/9.0.4) | Código-fonte e headers (a documentação mais exata desta versão). |
| [BREAKING_CHANGES.md](https://github.com/juce-framework/JUCE/blob/9.0.4/BREAKING_CHANGES.md) | O que mudou entre versões (ex.: `createEditor()` privado desde 8.0.13). |
| [docs/CMake API.md](https://github.com/juce-framework/JUCE/blob/9.0.4/docs/CMake%20API.md) | `juce_add_gui_app`, `juce_add_plugin`, opções de CMake. |
| [Documentação da API](https://docs.juce.com/master/) | Páginas por classe (geradas a partir do branch `master`). |
| [Tutoriais oficiais](https://juce.com/learn/tutorials/) | Tutoriais por tema (plugins, DSP, GUI). |
| [Fórum do JUCE](https://forum.juce.com/) | Armadilhas práticas conhecidas. |
| [Thread: biblioteca estática com os módulos JUCE](https://forum.juce.com/t/using-cmake-flow-to-build-libraries-android-apps/40492/2) | Padrão usado em `cmake/LearnJuceModules.cmake`. |


## Módulo 1 — Fundamentos

| Fonte | O que cobre | Usado em |
|---|---|---|
| [juce::AudioBuffer — documentação da API](https://docs.juce.com/master/classjuce_1_1AudioBuffer.html) | todos os métodos e a explicação da flag "isClear". | 01; web `buffers` |
| [juce::AudioProcessor — documentação da API](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html) | as garantias (e a falta delas) de `prepareToPlay` e `processBlock`. | 01, 33, 35; web `buffers` |
| [Tutorial JUCE: Create a basic Audio/MIDI plugin, Part 1](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/) | onde vivem `prepareToPlay` e `processBlock` num plugin (usa o Projucer, mas os conceitos são os mesmos). | 01, 35 |
| [Julius O. Smith III, *Mathematics of the DFT* — "Sampling Theory"](https://ccrma.stanford.edu/~jos/mdft/Sampling_Theory.html) | o que significa representar um sinal por amostras (base para o exercício 02/03). | 01 |
| [Julius O. Smith III, *Mathematics of the DFT* — "Sinusoids"](https://ccrma.stanford.edu/~jos/mdft/Sinusoids.html) | amplitude, frequência, fase, e o que muda ao amostrar. | 02; web `sampling` |
| [Julius O. Smith III, "Sampled Sinusoids"](https://ccrma.stanford.edu/~jos/mdft/Sampled_Sinusoids.html) | amplitude, frequência, fase, e o que muda ao amostrar. | 02, 03; web `sampling` |
| [Tutorial JUCE: Build a sine wave synthesiser](https://juce.com/tutorials/tutorial_sine_synth/) | o mesmo acumulador de fase dentro de um sintetizador JUCE. | 02, 08; web `sampling` |
| [juce::dsp::Oscillator — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oscillator.html) | a versão do JUCE. | 02, 18; web `lfo` |
| [Julius O. Smith III, *Mathematics of the DFT* — "Sampling Theorem"](https://ccrma.stanford.edu/~jos/mdft/Sampling_Theorem.html) | o enunciado e a prova do teorema da amostragem. | 03; web `nyquist-aliasing` |
| [T. Stilson e J. Smith, "Alias-Free Digital Synthesis of Classic Analog Waveforms" (ICMC 1996)](https://ccrma.stanford.edu/~stilti/papers/blit.pdf) | o artigo clássico sobre o problema do dente de serra ingénuo e a solução BLIT. | 03; web `nyquist-aliasing` |
| [Martin Finke, "PolyBLEP Oscillator"](https://www.martin-finke.de/articles/audio-plugins-018-polyblep-oscillator/) | uma técnica simples e muito usada para reduzir o aliasing de osciladores (blog de plugins em C++). | 03; web `nyquist-aliasing` |
| [Julius O. Smith III, *Mathematics of the DFT* — "Round-Off Error Variance"](https://ccrma.stanford.edu/~jos/mdft/Round_Off_Error_Variance.html) | de onde vem o LSB²/12. | 04; web `bit-depth`, web `bitcrush` |
| [S. Lipshitz, R. Wannamaker, J. Vanderkooy, "Quantization and Dither: A Theoretical Survey" (JAES, 1992)](https://hajim.rochester.edu/ece/sites/zduan/teaching/ece472/reading/Lipshitz_1992.pdf) | a referência sobre dither: porque é que o TPDF torna os dois primeiros momentos do erro independentes do sinal. | 04; web `bit-depth` |
| [R. Wannamaker, "The Theory of Dithered Quantization" (tese, Univ. of Waterloo)](https://uwspace.uwaterloo.ca/handle/10012/202) | o tratamento completo. | 04 |
| [juce::Random — documentação](https://docs.juce.com/master/classjuce_1_1Random.html) | o gerador usado. | 04 |
| [Ross Bencina, "Real-time audio programming 101: time waits for nothing"](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) | o texto de referência sobre estas regras e porquê. | 05, 36, 38, 39; web `buffers`, web `performance`, web `realtime` |
| [Dave Rowland & Fabian Renn-Giles, "Real time 101" (Meeting C++ 2019)](https://www.youtube.com/watch?v=ndeN983j_GQ) | a mesma talk da ADC 2019 (partes I e II): custos de chamadas ao sistema, alocações, *priority inversion*, atomics e FIFOs. | 05; web `realtime` |
| [JUCE: "Real-time programming in audio development"](https://juce.com/posts/real-time-programming-in-audio-development/) | o artigo do blog do JUCE sobre o mesmo tema. | 05; web `realtime` |
| [Timur Doumler, "Using locks in real-time audio processing, safely"](https://timur.audio/using-locks-in-real-time-audio-processing-safely) | quando é que (com muito cuidado) um lock pode ser aceitável, e porque é que um `std::mutex` não é. | 05 |
| [juce::AbstractFifo — documentação](https://docs.juce.com/master/classjuce_1_1AbstractFifo.html) | a FIFO lock-free do JUCE. | 05; web `realtime` |

## Módulo 2 — Ganho, dB, pan e smoothing

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Julius O. Smith III, *Mathematics of the DFT* — "Decibels"](https://ccrma.stanford.edu/~jos/mdft/Decibels.html) | a definição, amplitude vs. potência, e as escalas dB SPL/dBFS. | 06; web `decibels` |
| [juce::Decibels — documentação](https://docs.juce.com/master/classjuce_1_1Decibels.html) | as funções do JUCE e o piso de −100 dB. | 06; web `decibels` |
| [Carnegie Mellon University, *Introduction to Computer Music*, "Loudness Concepts & Panning Laws"](https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/) | as três leis, porquê o "buraco no meio" e quando usar cada uma. | 07; web `pan-laws` |
| [juce::dsp::Panner — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Panner.html) | as regras disponíveis no JUCE (a normalização do centro só se vê no código-fonte, `modules/juce_dsp/processors/juce_Panner.cpp`). | 07; web `pan-laws` |
| [juce::SmoothedValue — documentação](https://docs.juce.com/master/classjuce_1_1SmoothedValue.html) | os dois tipos de smoothing e os métodos `reset`, `setTargetValue`, `getNextValue`, `skip`. | 08, 39; web `smoothing` |
| [Tutorial JUCE: Control audio levels](https://juce.com/tutorials/tutorial_synth_level_control/) | mostra o problema (ganho aplicado por bloco pode estalar) num exemplo simples. | 08; web `smoothing` |
| [Tutorial JUCE: Introduction to DSP](https://juce.com/tutorials/tutorial_dsp_introduction/) | `ProcessSpec`, `AudioBlock`, `ProcessContext`, `ProcessorChain`. | 09, 17 |
| [juce::dsp::ProcessContextReplacing](https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessContextReplacing.html) | documentação. | 09 |
| [juce::dsp::AudioBlock](https://docs.juce.com/master/classjuce_1_1dsp_1_1AudioBlock.html) | documentação. | 09 |
| [juce::dsp::Gain](https://docs.juce.com/master/classjuce_1_1dsp_1_1Gain.html) | documentação. | 09 |
| [juce::dsp::ProcessorChain](https://docs.juce.com/master/classjuce_1_1dsp_1_1ProcessorChain.html) | documentação. | 09, 17 |
| [std::atomic — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic) | `load`, `store`, `is_always_lock_free` e as ordens de memória. | 09 |

## Módulo 3 — Delay

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Julius O. Smith III, *Physical Audio Signal Processing* — "Delay Lines"](https://ccrma.stanford.edu/~jos/pasp/Delay_Lines.html) | a delay line como bloco fundamental e a sua implementação com buffer circular. | 10, 13, 39; web `delay-line` |
| [juce::dsp::DelayLine — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html) | a versão do JUCE. | 10, 11, 12, 13; web `delay-line`, web `feedback-delay` |
| [Julius O. Smith III, PASP — "Delay-Line Interpolation"](https://ccrma.stanford.edu/~jos/pasp/Delay_Line_Interpolation.html) | a fórmula e a resposta em frequência. | 11; web `fractional-delay` |
| [Julius O. Smith III, "Linear Interpolation"](https://ccrma.stanford.edu/~jos/pasp/Linear_Interpolation.html) | a fórmula e a resposta em frequência. | 11; web `fractional-delay` |
| [Julius O. Smith III, PASP — "Lagrange Interpolation"](https://ccrma.stanford.edu/~jos/pasp/Lagrange_Interpolation.html) | a alternativa de ordem mais alta. | 11 |
| [Julius O. Smith III, PASP — "Doppler Effect"](https://ccrma.stanford.edu/~jos/pasp/Doppler_Effect.html) | porque é que um atraso variável muda a altura. | 11; web `fractional-delay` |
| [Julius O. Smith III, PASP — "Feedback Comb Filters"](https://ccrma.stanford.edu/~jos/pasp/Feedback_Comb_Filters.html) | o eco com feedback como filtro, a sua estabilidade e a resposta em frequência. | 12, 21, 27; web `comb-allpass`, web `feedback-delay` |
| [juce::dsp::DryWetMixer](https://docs.juce.com/master/classjuce_1_1dsp_1_1DryWetMixer.html) | mistura seco/molhado com várias leis e compensação de latência. | 12; web `feedback-delay` |

## Módulo 4 — Filtros

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Julius O. Smith III, *Introduction to Digital Filters* — "One-Pole"](https://ccrma.stanford.edu/~jos/filters/One_Pole.html) | o filtro de um polo, o seu polo e a resposta em frequência. | 14; web `one-pole` |
| [Nigel Redmon (EarLevel Engineering), "A one-pole filter"](https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/) | a mesma fórmula do coeficiente, com a derivação e usos práticos (DC blocker, smoothing). | 14; web `one-pole` |
| [juce::dsp::IIR::Coefficients](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1Coefficients.html) | `makeFirstOrderLowPass/HighPass` (bilinear transform) para comparar. | 14, 17 |
| [R. Bristow-Johnson, Audio EQ Cookbook (W3C)](https://www.w3.org/TR/audio-eq-cookbook/) | as fórmulas. | 15; web `biquad` |
| [Julius O. Smith III, *Introduction to Digital Filters* — "BiQuad Section"](https://ccrma.stanford.edu/~jos/filters/BiQuad_Section.html) | a estrutura e as formas de implementação. | 15; web `biquad` |
| [Julius O. Smith III, "Transposed Direct-Forms"](https://ccrma.stanford.edu/~jos/filters/Transposed_Direct_Forms.html) | a estrutura e as formas de implementação. | 15 |
| [Julius O. Smith III, PASP — "Bilinear Transformation"](https://ccrma.stanford.edu/~jos/pasp/Bilinear_Transformation.html) | de onde vem o `tan` e a pré-distorção. | 15 |
| [juce::dsp::IIR::ArrayCoefficients](https://docs.juce.com/master/structjuce_1_1dsp_1_1IIR_1_1ArrayCoefficients.html) | as mesmas fórmulas no JUCE. | 15, 17, 38; web `biquad` |
| [Vadim Zavalishin, *The Art of VA Filter Design* (rev. 2.1.2, PDF)](https://www.discodsp.net/VAFilterDesign_2.1.2.pdf) | o livro de referência sobre TPT (capítulos sobre integradores trapezoidais, ciclos sem atraso e o SVF). (O PDF original no site da Native Instruments, rev. 2.1.0, deu 404 em 2026-10-26; esta é a revisão 2.1.2, num espelho anunciado no fórum KVR. O livro permite cópias integrais e não modificadas.) | 16; web `svf` |
| [Andrew Simper (Cytomic), "Linear Trapezoidal Integrated SVF"](https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf) | a versão otimizada, com as fórmulas para todos os tipos de saída. | 16; web `svf` |
| [juce::dsp::StateVariableTPTFilter — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1StateVariableTPTFilter.html) | . | 16; web `svf` |
| [juce::dsp::IIR::Filter](https://docs.juce.com/master/classjuce_1_1dsp_1_1IIR_1_1Filter.html) | documentação. | 17 |
| [ProcessorDuplicator](https://docs.juce.com/master/structjuce_1_1dsp_1_1ProcessorDuplicator.html) | documentação. | 17 |
| [J. O. Smith — Pole-Zero Analysis](https://ccrma.stanford.edu/~jos/filters/Pole_Zero_Analysis_I.html) | como ler um diagrama de polos e zeros | web `biquad` |

## Módulo 5 — Modulação

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Julius O. Smith III, *Mathematics of the DFT* — "Sinusoidal Amplitude Modulation (AM)"](https://ccrma.stanford.edu/~jos/mdft/Sinusoidal_Amplitude_Modulation_AM.html) | o que acontece ao espectro quando um LFO modula a amplitude (base do exercício 19). | 18, 19; web `lfo` |
| [Julius O. Smith III, PASP — "Chorus Effect"](https://ccrma.stanford.edu/~jos/pasp/Chorus_Effect.html) | o chorus como delay modulado. | 20; web `chorus-flanger` |
| [Julius O. Smith III, PASP — "Vibrato Simulation"](https://ccrma.stanford.edu/~jos/pasp/Vibrato_Simulation.html) | o vibrato por delay modulado (o mesmo mecanismo, só com a cópia). | 20 |
| [juce::dsp::Chorus — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Chorus.html) | . | 20; web `chorus-flanger` |
| [Julius O. Smith III, PASP — "Flanging"](https://ccrma.stanford.edu/~jos/pasp/Flanging.html) | o flanger como pente variável. | 21; web `chorus-flanger` |
| [Julius O. Smith III, PASP — "Feedforward Comb Filters"](https://ccrma.stanford.edu/~jos/pasp/Feedforward_Comb_Filters.html) | as duas metades deste efeito. | 21 |
| [juce::dsp::Phaser — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Phaser.html) | o efeito irmão, sem delay. | 21 |

## Módulo 6 — Distorção

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Julius O. Smith III, PASP — "Cubic Soft Clipper"](https://ccrma.stanford.edu/~jos/pasp/Cubic_Soft_Clipper.html) | a curva cúbica e porque é suave. | 22; web `waveshaping` |
| [J. Parker, V. Zavalishin, E. Le Bivic, "Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution" (DAFx 2016)](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf) | o aliasing das curvas de distorção e uma técnica alternativa ao oversampling (antiderivative antialiasing). | 22, 26; web `oversampling`, web `waveshaping` |
| [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) | o módulo `chowdsp_waveshapers` tem clippers com ADAA usados em plugins reais (**GPLv3**: bom para ler e aprender, não para copiar para um produto fechado). | 22 |
| [juce::dsp::WaveShaper — documentação](https://docs.juce.com/master/structjuce_1_1dsp_1_1WaveShaper.html) | . | 22; web `waveshaping` |
| [Julius O. Smith III, *Introduction to Digital Filters* — "DC Blocker"](https://ccrma.stanford.edu/~jos/filters/DC_Blocker.html) | a fórmula, a resposta em frequência e a escolha de R. | 23 |
| [juce::dsp::FirstOrderTPTFilter — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1FirstOrderTPTFilter.html) | . | 23 |
| [Julius O. Smith III, "Digital Audio Resampling Home Page"](https://ccrma.stanford.edu/~jos/resample/) | como se muda o sample rate *bem* (com filtros) — o contrário do que o bitcrusher faz de propósito. | 24, 26; web `bitcrush`, web `oversampling` |
| [Julius O. Smith III, *Mathematics of the DFT* — "Fast Fourier Transform (FFT)"](https://ccrma.stanford.edu/~jos/mdft/Fast_Fourier_Transform_FFT.html) | a DFT e a FFT. | 25; web `fft` |
| [Julius O. Smith III, *Spectral Audio Signal Processing* — "Spectrum Analysis Windows"](https://ccrma.stanford.edu/~jos/sasp/Spectrum_Analysis_Windows.html) | janelas e escorrimento. | 25; web `fft` |
| [juce::dsp::FFT](https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html) | documentação. | 25; web `fft` |
| [juce::dsp::WindowingFunction](https://docs.juce.com/master/classjuce_1_1dsp_1_1WindowingFunction.html) | documentação. | 25 |
| [juce::dsp::Oversampling — documentação](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html) | construtor, tipos de filtro, latência. | 26, 40; web `oversampling`, web `pedal` |

## Módulo 7 — Reverb

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Julius O. Smith III, "Lowpass Feedback Comb Filter" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Lowpass_Feedback_Comb_Filter.html) | o comb com amortecimento usado no Freeverb. | 27; web `comb-allpass` |
| [Julius O. Smith III, "Allpass from Two Combs" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Allpass_Two_Combs.html) | de onde vem a forma do allpass. | 27 |
| [Julius O. Smith III, "Schroeder Allpass Sections"](https://ccrma.stanford.edu/~jos/pasp/Schroeder_Allpass_Sections.html) | de onde vem a forma do allpass. | 27; web `comb-allpass` |
| [Julius O. Smith III, "Freeverb Allpass Approximation" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Freeverb_Allpass_Approximation.html) | porque é que o "allpass" do Freeverb não o é. | 27; web `comb-allpass` |
| [Sean Costello (Valhalla DSP), "Reverbs: Diffusion, allpass delays, and metallic artifacts"](https://valhalladsp.com/2011/01/21/reverbs-diffusion-allpass-delays-and-metallic-artifacts/) | porque é que allpasses em série soam metálicos. | 27, 30; web `comb-allpass` |
| [Julius O. Smith III, "Schroeder Reverberators" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html) | a estrutura original e as suas variantes. | 28; web `reverb` |
| [Julius O. Smith III, "Freeverb" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html) | a estrutura, as afinações e o stereo spread. | 28; web `reverb` |
| [Julius O. Smith III, "Energy Decay Curve" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Energy_Decay_Curve.html) | a integração de Schroeder usada nos testes. | 28; web `reverb` |
| [juce::Reverb — documentação](https://docs.juce.com/master/classjuce_1_1Reverb.html) | a versão JUCE do Freeverb (compara o código com o teu: `modules/juce_audio_basics/utilities/juce_Reverb.h`). | 28; web `reverb` |
| [Julius O. Smith III, "Feedback Delay Networks (FDN)" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Feedback_Delay_Networks_FDN.html) | Gerzon (1971), Stautner & Puckette (1982), Jot. | 29; web `fdn` |
| [Julius O. Smith III, "History of FDNs for Artificial Reverberation"](https://ccrma.stanford.edu/~jos/pasp/History_FDNs_Artificial_Reverberation.html) | Gerzon (1971), Stautner & Puckette (1982), Jot. | 29; web `fdn` |
| [Julius O. Smith III, "Householder Feedback Matrix" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Householder_Feedback_Matrix.html) | a matriz deste exercício e porque é eficiente. | 29; web `fdn` |
| [Julius O. Smith III, "Achieving Desired Reverberation Times" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Achieving_Desired_Reverberation_Times.html) | o ganho por linha a partir do RT60. | 29 |
| [Geraint Luff (Signalsmith Audio), "Let's Write A Reverb"](https://signalsmith-audio.co.uk/writing/2021/lets-write-a-reverb/) | FDN multicanal com Householder, explicada passo a passo (base dos exercícios 29 e 30). | 29, 30; web `fdn`, web `pedal`, web `shimmer` |
| [Julius O. Smith III, "Hadamard Matrix" (PASP)](https://ccrma.stanford.edu/~jos/pasp/Hadamard_Matrix.html) | a matriz e o seu uso em FDNs. | 30; web `fdn` |
| [Jon Dattorro, "Effect Design, Part 1: Reverberator and Other Filters" (JAES 1997)](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf) | a reverb "plate" clássica: difusão com allpasses à entrada e um "tanque" em forma de 8. | 30, 31 |
| [Valhalla DSP, ValhallaShimmer](https://valhalladsp.com/shop/reverb/valhalla-shimmer/) | o manual descreve o pitch shift no ciclo de feedback e o som "Eno/Lanois" (+12 semitons, feedback ≥ 0.5). | 31; web `shimmer` |
| [Geraint Luff (Signalsmith Audio), "Four Ways To Write A Pitch-Shifter" (ADC 2022) — e o, código de exemplo](https://github.com/Signalsmith-Audio/pitch-time-example-code) | usado nas demos. | 31, 39; web `shimmer` |

## Módulo 8 — Plugin real

| Fonte | O que cobre | Usado em |
|---|---|---|
| [Tutorial JUCE: Saving and loading your plug-in state (APVTS)](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) | o APVTS passo a passo. | 32, 33, 34, 40; web `plugin-architecture` |
| [Tutorial JUCE: Adding plug-in parameters](https://juce.com/tutorials/tutorial_audio_parameter/) | parâmetros sem APVTS (o que fizeste nos wrappers dos exercícios anteriores). | 32 |
| [juce::AudioProcessorValueTreeState — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html) | `ParameterLayout`, `getRawParameterValue`, `getParameter`. | 32, 33; web `plugin-architecture` |
| [juce::ParameterID — documentação](https://docs.juce.com/master/classjuce_1_1ParameterID.html) | o que é a *version hint*. | 32 |
| [juce::NormalisableRange — documentação](https://docs.juce.com/master/classjuce_1_1NormalisableRange.html) | gamas, passo e `setSkewForCentre`. | 32; web `plugin-architecture` |
| [pluginval (Tracktion)](https://github.com/Tracktion/pluginval) | um dos testes que faz é precisamente gravar/repor estado com dados aleatórios. | 33, 35, 40; web `pedal`, web `plugin-architecture` |
| [AudioProcessorValueTreeState::SliderAttachment — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1SliderAttachment.html) | e as classes irmãs para ComboBox e Button. | 34 |
| [juce::AudioProcessorEditor — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessorEditor.html) | tamanho, redimensionamento, ciclo de vida. | 34 |
| [juce::Rectangle — documentação](https://docs.juce.com/master/classjuce_1_1Rectangle.html) | `removeFromTop` e companhia. | 34 |
| [Tutorial JUCE: Code your first audio plug-in](https://juce.com/tutorials/tutorial_code_basic_plugin/) | o editor mais simples possível, sem APVTS. | 34 |
| [JUCE — CMake API (`juce_add_plugin`)](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md) | todas as opções (formatos, códigos, categorias, `COPY_PLUGIN_AFTER_BUILD`, `VST3_AUTO_MANIFEST`). | 35, 40; web `plugin-architecture` |
| [juce::AudioChannelSet — documentação](https://docs.juce.com/master/classjuce_1_1AudioChannelSet.html) | mono, estéreo, surround. | 35 |
| [Steinberg — localização dos plugins VST3](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html) | onde os hosts procuram plugins em cada sistema. | 35 |

## Módulo 9 — Performance e otimização

| Fonte | O que cobre | Usado em |
|---|---|---|
| [juce::AudioProcessLoadMeasurer — documentação](https://docs.juce.com/master/classjuce_1_1AudioProcessLoadMeasurer.html) | o medidor do JUCE (usado pelo `AudioDeviceManager::getCpuUsage`). | 36; web `performance` |
| [juce::Time — documentação](https://docs.juce.com/master/classjuce_1_1Time.html) | `getHighResolutionTicks`, `highResolutionTicksToSeconds`. | 36 |
| [Agner Fog, Software optimization resources](https://www.agner.org/optimize/) | manuais de referência sobre o custo de cada instrução e como otimizar C++. | 36, 37 |
| [juce::FloatVectorOperations — documentação](https://docs.juce.com/master/classjuce_1_1FloatVectorOperations.html) | a lista completa de operações. | 37 |
| [juce::dsp::SIMDRegister — documentação](https://docs.juce.com/master/structjuce_1_1dsp_1_1SIMDRegister.html) | `fromRawArray`, `isSIMDAligned`, `sum`, operadores. | 37; web `performance` |
| [SIMDRegisterDemo.h](https://github.com/juce-framework/JUCE/blob/master/examples/DSP/SIMDRegisterDemo.h) | exemplo oficial: um filtro IIR a processar vários canais em paralelo. | 37 |
| [Microsoft — Auto-Parallelization and Auto-Vectorization](https://learn.microsoft.com/en-us/cpp/parallel/auto-parallelization-and-auto-vectorization) | o que o vetorizador do MSVC faz e quando desiste. | 37 |
| [Laurent de Soras, "Denormal numbers in floating point signal processing applications" (PDF)](http://ldesoras.free.fr/doc/articles/denormal-en.pdf) | o artigo clássico sobre denormais em áudio e as soluções. | 38; web `performance` |
| [Wikipedia — Subnormal number](https://en.wikipedia.org/wiki/Subnormal_number) | o formato e as consequências de performance. | 38; web `performance` |
| [juce::ScopedNoDenormals — documentação](https://docs.juce.com/master/classjuce_1_1ScopedNoDenormals.html) | FTZ/DAZ com RAII. | 38 |

## Referências bibliográficas sem link (citadas nas lições)

| Referência | Usada em |
|---|---|
| M. R. Schroeder, "Natural Sounding Artificial Reverberation", *JAES* 10(3), 1962 | 28 |
| J.-M. Jot, A. Chaigne, "Digital Delay Networks for Designing Artificial Reverberators", 90.ª Convenção AES, 1991 | 29 |
| U. Zölzer (ed.), *DAFX: Digital Audio Effects*, 2.ª ed., Wiley, 2011 | 31 |

## Bibliotecas de terceiros e licenças

| Biblioteca | Licença | Uso |
|---|---|---|
| [JUCE 9.0.4](https://github.com/juce-framework/JUCE) | AGPLv3 ou licença comercial JUCE 9 | Framework; descarregado pelo CMake (FetchContent), não incluído no repositório. Para distribuir plugins fechados é preciso a licença comercial (ou cumprir a AGPLv3). |
| [Catch2 v3.16.1](https://github.com/catchorg/Catch2) | Boost Software License 1.0 | Testes; descarregado pelo CMake. |
| Vite 8, TypeScript 7 (npm) | MIT / Apache-2.0 | Web app (dependências de desenvolvimento, `web/package.json`). |

**Nenhum código de terceiros foi copiado para o repositório.** Algumas fontes consultadas têm licenças
que impediriam a cópia num produto comercial fechado, por isso foram usadas só como leitura:

- [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils): os módulos de DSP são **GPLv3**
  (os módulos "core" são BSD). Citado no exercício 22 apenas como referência de leitura.
- Código de exemplo da Signalsmith Audio (reverb e pitch-shift): consultado o artigo/explicação; a
  implementação dos exercícios é própria.
- O "Freeverb" (domínio público) e o `juce::Reverb` foram estudados; o exercício 28 implementa a
  estrutura a partir da descrição do JOS.
