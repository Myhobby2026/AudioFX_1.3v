// ============================================================================
//  EQPanel.h - 10-band graphic EQ with live response curve
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "Controls.h"
#include "dsp/ProcessingChain.h"

class EQPanel : public ModulePanel
{
public:
    explicit EQPanel (audiofx::ProcessingChain& chain);
    ~EQPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    void syncFrom (const struct Params& p);
    std::function<void()> onParamChanged;

private:
    void pushToChain (int band);
    void flat();

    audiofx::ProcessingChain& chain;
    std::unique_ptr<Knob> bands[10];
    juce::TextButton flatButton { "FLAT" };
};
