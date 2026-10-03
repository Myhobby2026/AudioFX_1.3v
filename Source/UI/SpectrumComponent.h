// ============================================================================
//  SpectrumComponent.h - live FFT spectrum with peak hold
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "dsp/ProcessingChain.h"

class SpectrumComponent : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumComponent (audiofx::ProcessingChain& chainToUse);
    ~SpectrumComponent() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float xForFrequency (double hz) const;
    float yForDb (float db) const;

    audiofx::ProcessingChain& chain;
    std::vector<float> displayDb, displayHold;
    juce::Path spectrumPath, holdPath;

    static constexpr float kMinDb = -72.0f;
    static constexpr float kMaxDb = 6.0f;
    static constexpr double kMinHz = 28.0;
    static constexpr double kMaxHz = 20000.0;
};
