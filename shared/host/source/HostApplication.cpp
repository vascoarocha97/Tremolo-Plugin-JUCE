#include <learn_host/HostApplication.h>

#include "HostComponent.h"

namespace learn
{

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow (const juce::String& name, std::unique_ptr<HostComponent> content)
        : DocumentWindow (name,
                          juce::Desktop::getInstance().getDefaultLookAndFeel()
                              .findColour (juce::ResizableWindow::backgroundColourId),
                          DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (content.release(), true);
        setResizable (true, true);
        setResizeLimits (900, 600, 4000, 3000);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

//==============================================================================
HostApplication::HostApplication() = default;
HostApplication::~HostApplication() = default;

const juce::String HostApplication::getApplicationName()
{
    return "Learn JUCE " + getExerciseNumber();
}

const juce::String HostApplication::getApplicationVersion()
{
    return "0.1.0";
}

void HostApplication::initialise (const juce::String& commandLine)
{
    auto content = std::make_unique<HostComponent> (createProcessor(), getExerciseNumber(), getWebPage());

    // ./learn verify usa: --smoke-test=<ficheiro de resultado>
    if (commandLine.contains ("--smoke-test="))
        content->startSmokeTest (juce::File (commandLine.fromFirstOccurrenceOf ("--smoke-test=", false, false)
                                                 .trim()
                                                 .unquoted()));

    mainWindow = std::make_unique<MainWindow> (getApplicationName(), std::move (content));
}

void HostApplication::shutdown()
{
    mainWindow = nullptr;
}

void HostApplication::systemRequestedQuit()
{
    quit();
}

} // namespace learn
