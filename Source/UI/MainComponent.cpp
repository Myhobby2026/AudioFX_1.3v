// ============================================================================
//  MainComponent.cpp
// ============================================================================
#include "MainComponent.h"
#include "Theme.h"

MainComponent::MainComponent()
{
    setOpaque (true);
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (deviceBar);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (eqPanel);
    addAndMakeVisible (reverbPanel);
    addAndMakeVisible (surroundPanel);
    addAndMakeVisible (compPanel);
    addAndMakeVisible (limiterPanel);
    addAndMakeVisible (masterPanel);

    // ---- FX tab bar -----------------------------------------------------------
    auto setupTab = [this] (juce::TextButton& b, int idx)
    {
        addAndMakeVisible (b);
        b.setRadioGroupId (777);
        b.setClickingTogglesState (true);
        b.onClick = [this, idx] { showFx (idx); };
    };
    setupTab (tabReverb, 0);
    setupTab (tabField, 1);
    setupTab (tabComp, 2);
    setupTab (tabLimiter, 3);
    tabComp.setToggleState (true, juce::dontSendNotification);

    // ---- restore settings -----------------------------------------------------
    engine.initialise();
    if (presetManager.loadSettings (params))
    {
        params.applyTo (engine.getChain());
        engine.setCaptureMode (static_cast<AudioEngine::CaptureMode> (params.captureMode));
        if (params.inputDevice.isNotEmpty() && params.captureMode == 0)
            engine.setInputDevice (params.inputDevice);
        if (params.outputDevice.isNotEmpty())
            engine.setOutputDevice (params.outputDevice);
    }
    else
    {
        // first run: prefer the VB-Cable capture endpoint
        if (auto vb = engine.findVBCableInput(); vb.isNotEmpty())
        {
            params.inputDevice = vb;
            engine.setInputDevice (vb);
            params.captureMode = 0;
        }
        params.applyTo (engine.getChain());
    }

    setSize ((int) params.windowW, (int) params.windowH);

    // ---- UI bindings ----------------------------------------------------------
    deviceBar.setModeItems ({ "VB-CABLE / DEVICE", "WASAPI LOOPBACK" }, params.captureMode + 1);

    deviceBar.onModeChanged = [this] (int id)
    {
        params.captureMode = juce::jlimit (0, 1, id - 1);
        engine.setCaptureMode (static_cast<AudioEngine::CaptureMode> (params.captureMode));
        markDirty();
    };
    deviceBar.onInputChanged = [this]
    {
        params.inputDevice = deviceBar.inputBox.getText();
        engine.setInputDevice (params.inputDevice);
        markDirty();
    };
    deviceBar.onOutputChanged = [this]
    {
        params.outputDevice = deviceBar.outputBox.getText();
        engine.setOutputDevice (params.outputDevice);
        markDirty();
    };
    deviceBar.onPresetChanged = [this]
    {
        const auto name = deviceBar.presetBox.getText();
        Params p;
        if (PresetManager::getFactoryPreset (name, p) || presetManager.getUserPreset (name, p))
        {
            p.inputDevice  = params.inputDevice;
            p.outputDevice = params.outputDevice;
            p.captureMode  = params.captureMode;
            p.windowW = params.windowW;
            p.windowH = params.windowH;
            loadParams (p);
            currentPresetName = name;
            markDirty();
        }
    };
    deviceBar.onSavePreset   = [this] { promptSavePreset(); };
    deviceBar.onDeletePreset = [this]
    {
        const auto name = deviceBar.presetBox.getText();
        if (PresetManager::factoryPresetNames().contains (name))
            return;
        presetManager.deleteUserPreset (name);
        refreshPresetList();
        markDirty();
    };
    deviceBar.onGlobalBypass = [this] (bool byp)
    {
        engine.getChain().setGlobalBypass (byp);
        params.globalBypass = byp;
        markDirty();
    };
    deviceBar.globalBypass.setToggleState (params.globalBypass, juce::dontSendNotification);

    auto bindBypass = [this] (ModulePanel& panel, bool initial, std::function<void (bool)> apply)
    {
        panel.onBypassChanged = [this, apply] (bool bypassed)
        {
            apply (bypassed);
            markDirty();
        };
        panel.setBypassActive (initial);
    };
    bindBypass (eqPanel, params.eqBypass,       [this] (bool b) { engine.getChain().getEq().setBypassed (b);       params.eqBypass = b; });
    bindBypass (reverbPanel, params.revBypass,  [this] (bool b) { engine.getChain().getReverb().setBypassed (b);   params.revBypass = b; });
    bindBypass (surroundPanel, params.surBypass,[this] (bool b) { engine.getChain().getSurround().setBypassed (b); params.surBypass = b; });
    bindBypass (compPanel, params.compBypass,   [this] (bool b) { engine.getChain().getCompressor().setBypassed (b); params.compBypass = b; });
    bindBypass (limiterPanel, params.limBypass, [this] (bool b) { engine.getChain().getLimiter().setBypassed (b);  params.limBypass = b; });

    auto paramChanged = [this] { markDirty(); };
    eqPanel.onParamChanged       = paramChanged;
    reverbPanel.onParamChanged   = paramChanged;
    surroundPanel.onParamChanged = paramChanged;
    compPanel.onParamChanged     = paramChanged;
    limiterPanel.onParamChanged  = paramChanged;
    masterPanel.onParamChanged   = paramChanged;

    eqPanel.syncFrom (params);
    reverbPanel.syncFrom (params);
    surroundPanel.syncFrom (params);
    compPanel.syncFrom (params);
    limiterPanel.syncFrom (params);
    masterPanel.syncFrom (params);

    showFx (activeFx);

    refreshDeviceLists();
    refreshPresetList (currentPresetName);

    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    params = Params::fromChain (engine.getChain());
    params.inputDevice  = deviceBar.inputBox.getText();
    params.outputDevice = deviceBar.outputBox.getText();
    params.captureMode  = (int) engine.getCaptureMode();
    params.windowW = (float) getWidth();
    params.windowH = (float) getHeight();
    presetManager.saveSettings (params);

    setLookAndFeel (nullptr);
    engine.shutdown();
}

void MainComponent::paint (juce::Graphics& g)
{
    theme::paintBackdrop (g, getLocalBounds());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    deviceBar.setBounds (area.removeFromTop (64).reduced (6, 2));

    auto master = area.removeFromRight (252);
    masterPanel.setBounds (master.reduced (6, 4));

    auto top = area.removeFromTop (juce::jlimit (190, 300, area.getHeight() * 34 / 100));
    spectrum.setBounds (top.reduced (6, 4));

    auto eq = area.removeFromTop (area.getHeight() * 48 / 100);
    eqPanel.setBounds (eq.reduced (6, 4));

    // FX tab bar
    auto tabs = area.removeFromTop (30).reduced (6, 2);
    const int tw = tabs.getWidth() / 4;
    tabReverb.setBounds  (tabs.removeFromLeft (tw));
    tabField.setBounds   (tabs.removeFromLeft (tw));
    tabComp.setBounds    (tabs.removeFromLeft (tw));
    tabLimiter.setBounds (tabs);

    // active FX panel fills the bottom area
    auto bottom = area.reduced (6, 4);
    reverbPanel.setBounds (bottom);
    surroundPanel.setBounds (bottom);
    compPanel.setBounds (bottom);
    limiterPanel.setBounds (bottom);
}

void MainComponent::showFx (int idx)
{
    activeFx = juce::jlimit (0, 3, idx);
    reverbPanel.setVisible  (activeFx == 0);
    surroundPanel.setVisible(activeFx == 1);
    compPanel.setVisible    (activeFx == 2);
    limiterPanel.setVisible (activeFx == 3);
    repaint();
}

void MainComponent::timerCallback()
{
    const auto& chain = engine.getChain();
    deviceBar.setStatus (engine.isStreaming(), engine.getCurrentSampleRate(), engine.getCurrentBlockSize());
    compPanel.refreshMeters();
    limiterPanel.refreshMeters();
    masterPanel.refreshMeters();

    if (settingsDirty)
    {
        static int countdown = 0;
        if (--countdown <= 0)
        {
            params = Params::fromChain (engine.getChain());
            params.inputDevice  = deviceBar.inputBox.getText();
            params.outputDevice = deviceBar.outputBox.getText();
            params.captureMode  = (int) engine.getCaptureMode();
            params.windowW = (float) getWidth();
            params.windowH = (float) getHeight();
            presetManager.saveSettings (params);
            settingsDirty = false;
            countdown = 15;      // ~0.5 s debounce at 30 Hz
        }
    }
    (void) chain;
}

void MainComponent::refreshDeviceLists()
{
    deviceBar.setInputItems (engine.availableInputDevices(), engine.getInputDevice());
    deviceBar.setOutputItems (engine.availableOutputDevices(), engine.getOutputDevice());
}

void MainComponent::refreshPresetList (const juce::String& select)
{
    deviceBar.setPresetItems (presetManager.allPresetNames(), select.isNotEmpty() ? select : "Flat");
}

void MainComponent::loadParams (const Params& p)
{
    params = p;
    params.applyTo (engine.getChain());
    eqPanel.syncFrom (params);
    reverbPanel.syncFrom (params);
    surroundPanel.syncFrom (params);
    compPanel.syncFrom (params);
    limiterPanel.syncFrom (params);
    masterPanel.syncFrom (params);
    deviceBar.globalBypass.setToggleState (params.globalBypass, juce::dontSendNotification);
    repaint();
}

void MainComponent::markDirty()
{
    settingsDirty = true;
}

void MainComponent::promptSavePreset()
{
    auto* window = new juce::AlertWindow ("Save Preset",
                                          "Name this preset:",
                                          juce::MessageBoxIconType::NoIcon, this);
    window->addTextEditor ("presetName", currentPresetName);
    window->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, window] (int result)
        {
            if (result == 1)
            {
                auto name = window->getTextEditorContents ("presetName").trim();
                if (name.isNotEmpty())
                {
                    auto p = Params::fromChain (engine.getChain());
                    p.inputDevice = params.inputDevice;
                    p.outputDevice = params.outputDevice;
                    p.captureMode = params.captureMode;
                    p.windowW = params.windowW;
                    p.windowH = params.windowH;
                    presetManager.saveUserPreset (name, p);
                    currentPresetName = name;
                    refreshPresetList (name);
                    markDirty();
                }
            }
        }), true);
}
