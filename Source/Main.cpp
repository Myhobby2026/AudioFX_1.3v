// ============================================================================
//  Main.cpp - AudioFX application entry point
// ============================================================================
#include <JuceHeader.h>
#include "BinaryData.h"
#include "UI/MainComponent.h"

class AudioFxApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "AudioFX"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (false);
            setResizable (true, true);
            setResizeLimits (1080, 720, 4000, 2400);

            content = std::make_unique<MainComponent>();
            setContentOwned (content.release(), true);

            centreWithSize (getWidth(), getHeight());
            setVisible (true);

            #if JUCE_WINDOWS
            if (auto* peer = getPeer())
                if (auto img = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize); img.isValid())
                    peer->setIcon (img);
            #endif
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        std::unique_ptr<MainComponent> content;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (AudioFxApp)
