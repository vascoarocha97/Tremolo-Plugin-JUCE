#include "HostComponent.h"

#include <learn_host/ExerciseAudioProcessor.h>

namespace learn
{
namespace
{
    void setUpSlider (juce::Slider& slider, double min, double max, double value, const juce::String& suffix)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 20);
        slider.setRange (min, max, 0.1);
        slider.setValue (value, juce::dontSendNotification);
        slider.setTextValueSuffix (suffix);
    }
}

HostComponent::HostComponent (std::unique_ptr<juce::AudioProcessor> processorToHost,
                              const juce::String& exerciseNumber, const juce::String& page)
    : processor (std::move (processorToHost)),
      engine (*processor),
      webPage (page)
{
    // ---- Título --------------------------------------------------------------------------------
    titleLabel.setText (juce::String::fromUTF8 ("Exercício ") + exerciseNumber + juce::String::fromUTF8 (" · ")
                            + processor->getName(),
                        juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    webButton.setTooltip (juce::String::fromUTF8 ("Abre http://localhost:5173 (corre primeiro: ./learn web)"));
    webButton.onClick = [this]
    {
        juce::URL ("http://localhost:5173/#/" + webPage).launchInDefaultBrowser();
    };
    webButton.setVisible (webPage.isNotEmpty());
    addChildComponent (webButton);

    // ---- Fonte de teste ------------------------------------------------------------------------
    sourceBox.addItemList (TestSignalSource::getSourceNames(), 1);
    sourceBox.setSelectedId (static_cast<int> (SourceType::demoLoop) + 1, juce::dontSendNotification);
    sourceBox.onChange = [this]
    {
        engine.getSource().setType (static_cast<SourceType> (sourceBox.getSelectedId() - 1));
        updateSourceControls();
    };
    addAndMakeVisible (sourceBox);

    setUpSlider (levelSlider, -60.0, 0.0, -6.0, " dB");
    levelSlider.onValueChange = [this] { engine.getSource().setLevelDb ((float) levelSlider.getValue()); };
    engine.getSource().setLevelDb (-6.0f);
    levelLabel.setText (juce::String::fromUTF8 ("Nível"), juce::dontSendNotification);
    addAndMakeVisible (levelSlider);
    addAndMakeVisible (levelLabel);

    setUpSlider (frequencySlider, 20.0, 20000.0, 220.0, " Hz");
    frequencySlider.setSkewFactorFromMidPoint (1000.0);
    frequencySlider.onValueChange = [this] { engine.getSource().setFrequency ((float) frequencySlider.getValue()); };
    frequencyLabel.setText (juce::String::fromUTF8 ("Frequência"), juce::dontSendNotification);
    addAndMakeVisible (frequencySlider);
    addAndMakeVisible (frequencyLabel);

    loadButton.onClick = [this] { chooseFile(); };
    addAndMakeVisible (loadButton);

    // ---- Saída ---------------------------------------------------------------------------------
    bypassButton.onClick = [this] { engine.setBypassed (bypassButton.getToggleState()); };
    addAndMakeVisible (bypassButton);

    setUpSlider (outputSlider, -60.0, 6.0, -12.0, " dB");
    outputSlider.onValueChange = [this] { engine.setOutputGainDb ((float) outputSlider.getValue()); };
    engine.setOutputGainDb (-12.0f);
    outputLabel.setText (juce::String::fromUTF8 ("Saída"), juce::dontSendNotification);
    addAndMakeVisible (outputSlider);
    addAndMakeVisible (outputLabel);

    settingsButton.onClick = [this] { showAudioSettings(); };
    addAndMakeVisible (settingsButton);

    resetButton.setTooltip (juce::String::fromUTF8 ("Pára e volta a arrancar o áudio: chama outra vez prepareToPlay()"));
    resetButton.onClick = [this] { restartAudio(); };
    addAndMakeVisible (resetButton);

    // ---- Parâmetros do exercício ---------------------------------------------------------------
    // Editor próprio (exercícios com GUI) e/ou parâmetros genéricos. Os processadores dos exercícios
    // de DSP (ExerciseAudioProcessor) mostram sempre os parâmetros; os do módulo 8 (plugins
    // completos) têm a sua própria GUI.
    if (processor->hasEditor())
    {
        editor.reset (processor->createEditorAndMakeActive());
        parameterContent.addAndMakeVisible (*editor);
    }

    const auto isExercise = dynamic_cast<ExerciseAudioProcessor*> (processor.get()) != nullptr;

    if (! processor->getParameters().isEmpty() && (editor == nullptr || isExercise))
    {
        genericEditor = std::make_unique<juce::GenericAudioProcessorEditor> (*processor);
        parameterContent.addAndMakeVisible (*genericEditor);
    }

    parametersLabel.setText (editor != nullptr || genericEditor != nullptr ? juce::String::fromUTF8 ("Parâmetros")
                                                                         : juce::String::fromUTF8 ("Este exercício não tem parâmetros."),
                             juce::dontSendNotification);
    addAndMakeVisible (parametersLabel);
    parameterViewport.setViewedComponent (&parameterContent, false);

    parameterViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (parameterViewport);

    // ---- Análise -------------------------------------------------------------------------------
    addAndMakeVisible (scope);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (meter);

    statusLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (statusLabel);

    updateSourceControls();

    // ---- Áudio ---------------------------------------------------------------------------------
    std::unique_ptr<juce::XmlElement> savedState;

    if (getDeviceSettingsFile().existsAsFile())
        savedState = juce::XmlDocument::parse (getDeviceSettingsFile());

    const auto error = deviceManager.initialise (2, 2, savedState.get(), true);

    if (error.isNotEmpty())
        statusLabel.setText (juce::String::fromUTF8 ("Erro de áudio: ") + error, juce::dontSendNotification);

    deviceManager.addAudioCallback (&engine);

    setSize (1180, 780);
    startTimerHz (30);
}

HostComponent::~HostComponent()
{
    stopTimer();

    if (auto state = deviceManager.createStateXml())
    {
        getDeviceSettingsFile().getParentDirectory().createDirectory();
        state->writeTo (getDeviceSettingsFile());
    }

    // Ordem importa: primeiro parar o áudio, depois destruir o editor, por fim o processador.
    deviceManager.removeAudioCallback (&engine);
    deviceManager.closeAudioDevice();
    parameterViewport.setViewedComponent (nullptr, false);
    genericEditor.reset();
    editor.reset();
}

juce::File HostComponent::getDeviceSettingsFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("LearnJuce")
        .getChildFile ("audio-device.xml");
}

void HostComponent::updateSourceControls()
{
    const auto type = engine.getSource().getType();
    frequencySlider.setEnabled (type == SourceType::sine || type == SourceType::naiveSaw);
    loadButton.setEnabled (type == SourceType::file);
}

void HostComponent::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser> (juce::String::fromUTF8 ("Escolhe um ficheiro de áudio"),
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto file = chooser.getResult();

                                  if (file == juce::File())
                                      return;

                                  juce::String error;

                                  if (engine.getSource().loadFile (file, error))
                                      loadButton.setButtonText (file.getFileName());
                                  else
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                              "Erro", error);
                              });
}

void HostComponent::showAudioSettings()
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent> (deviceManager, 0, 2, 1, 2,
                                                                          false, false, true, false);
    selector->setSize (520, 420);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (selector.release());
    options.dialogTitle = juce::String::fromUTF8 ("Definições de áudio");
    options.dialogBackgroundColour = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void HostComponent::restartAudio()
{
    // Fecha e reabre o dispositivo: o motor volta a chamar prepareToPlay() no processador.
    auto setup = deviceManager.getAudioDeviceSetup();
    deviceManager.closeAudioDevice();
    deviceManager.setAudioDeviceSetup (setup, true);
}

void HostComponent::timerCallback()
{
    const auto sampleRate = engine.getSampleRate();

    auto pullInto = [this] (AnalysisFifo& fifo, SampleHistory& history)
    {
        const auto num = fifo.pull (pullBuffer.data(), static_cast<int> (pullBuffer.size()));
        history.append (pullBuffer.data(), num);
    };

    pullInto (engine.getInputFifo(), inputHistory);
    pullInto (engine.getOutputFifo(), outputHistory);

    scope.update (inputHistory, outputHistory, sampleRate);
    spectrum.update (inputHistory, outputHistory, sampleRate);
    const auto peakLeft = engine.takeOutputPeak (0), peakRight = engine.takeOutputPeak (1);
    meter.setLevels (peakLeft, peakRight);

    if (smokeTestTicks >= 0)
    {
        smokeTestPeak = juce::jmax (smokeTestPeak, peakLeft, peakRight);

        if (smokeTestTicks == 75)
        {
            // Captura da janela (para rever a interface sem a abrir à mão)
            const auto snapshot = createComponentSnapshot (getLocalBounds());
            juce::FileOutputStream stream (smokeTestFile.withFileExtension ("png"));

            if (stream.openedOk())
            {
                stream.setPosition (0);
                stream.truncate();
                juce::PNGImageFormat().writeImageToStream (snapshot, stream);
            }
        }

        if (++smokeTestTicks >= 90)
        {
            const auto ok = sampleRate > 0.0 && smokeTestPeak > 1.0e-4f && ! engine.isMutedByNaN();
            smokeTestFile.replaceWithText ("{ \"ok\": " + juce::String (ok ? "true" : "false")
                                           + ", \"sampleRate\": " + juce::String (sampleRate)
                                           + ", \"peak\": " + juce::String (smokeTestPeak)
                                           + ", \"nan\": " + juce::String (engine.isMutedByNaN() ? "true" : "false")
                                           + " }\n");
            smokeTestTicks = -1;
            juce::JUCEApplication::getInstance()->setApplicationReturnValue (ok ? 0 : 1);
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    }

    if (engine.takeClipCount() > 0)
        clipDisplayCountdown = 30;
    else if (clipDisplayCountdown > 0)
        --clipDisplayCountdown;

    juce::String status;

    if (sampleRate > 0.0)
    {
        const auto blockSize = engine.getBlockSize();
        const auto dot = juce::String::fromUTF8 ("  ·  ");
        status << juce::String (sampleRate, 0) << " Hz" << dot << "bloco de " << blockSize << " amostras ("
               << juce::String (1000.0 * blockSize / sampleRate, 1) << " ms)" << dot << "CPU "
               << juce::String (100.0 * deviceManager.getCpuUsage(), 1) << " %";
    }
    else
    {
        status << juce::String::fromUTF8 ("Sem dispositivo de áudio ativo — carrega em \"Áudio...\"");
    }

    if (engine.isMutedByNaN())
        status << juce::String::fromUTF8 ("  ·  ⚠ NaN/inf na saída: silenciado. Corrige o código e carrega em \"Reiniciar áudio\".");
    else if (clipDisplayCountdown > 0)
        status << juce::String::fromUTF8 ("  ·  ⚠ acima de 0 dBFS: o limitador de proteção está a cortar.");

    statusLabel.setText (status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId,
                           engine.isMutedByNaN() || clipDisplayCountdown > 0 ? juce::Colours::orange
                                                                             : juce::Colours::lightgrey);
}

void HostComponent::startSmokeTest (const juce::File& resultFile)
{
    smokeTestFile = resultFile;
    smokeTestPeak = 0.0f;
    smokeTestTicks = 0;
}

void HostComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void HostComponent::resized()
{
    auto area = getLocalBounds().reduced (10);

    auto titleRow = area.removeFromTop (32);
    webButton.setBounds (titleRow.removeFromRight (200));
    titleLabel.setBounds (titleRow);
    area.removeFromTop (6);

    auto row1 = area.removeFromTop (28);
    sourceBox.setBounds (row1.removeFromLeft (300));
    row1.removeFromLeft (12);
    levelLabel.setBounds (row1.removeFromLeft (50));
    levelSlider.setBounds (row1.removeFromLeft (230));
    row1.removeFromLeft (12);
    frequencyLabel.setBounds (row1.removeFromLeft (80));
    frequencySlider.setBounds (row1.removeFromLeft (260));
    row1.removeFromLeft (12);
    loadButton.setBounds (row1);
    area.removeFromTop (6);

    auto row2 = area.removeFromTop (28);
    bypassButton.setBounds (row2.removeFromLeft (130));
    row2.removeFromLeft (12);
    outputLabel.setBounds (row2.removeFromLeft (50));
    outputSlider.setBounds (row2.removeFromLeft (280));
    row2.removeFromLeft (12);
    settingsButton.setBounds (row2.removeFromLeft (100));
    row2.removeFromLeft (8);
    resetButton.setBounds (row2.removeFromLeft (140));
    area.removeFromTop (10);

    statusLabel.setBounds (area.removeFromBottom (24));
    area.removeFromBottom (6);

    auto left = area.removeFromLeft (400);
    parametersLabel.setBounds (left.removeFromTop (22));
    parameterViewport.setBounds (left);

    // Editor próprio em cima (com o tamanho que pediu), parâmetros genéricos por baixo
    const auto contentWidth = parameterViewport.getMaximumVisibleWidth();
    int y = 0;

    if (editor != nullptr)
    {
        editor->setTopLeftPosition (0, 0);
        y = editor->getBottom() + 8;
    }

    if (genericEditor != nullptr)
    {
        genericEditor->setBounds (0, y, contentWidth, genericEditor->getHeight());
        y = genericEditor->getBottom();
    }

    parameterContent.setSize (juce::jmax (contentWidth, editor != nullptr ? editor->getWidth() : 0), y);

    area.removeFromLeft (10);
    meter.setBounds (area.removeFromRight (34));
    area.removeFromRight (8);
    scope.setBounds (area.removeFromTop (area.getHeight() * 2 / 5));
    area.removeFromTop (8);
    spectrum.setBounds (area);
}

} // namespace learn
