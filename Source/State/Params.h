// ============================================================================
//  Params.h - complete parameter state + XML serialisation + apply/pull
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include "dsp/ProcessingChain.h"

struct Params
{
    // ---- 10-band EQ -----------------------------------------------------------
    float eqGainDb[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    // ---- Reverb ---------------------------------------------------------------
    float revRoom  = 0.55f;
    float revDamp  = 0.40f;
    float revWet   = 0.22f;
    float revDry   = 1.00f;
    float revWidth = 1.00f;
    bool  revFreeze = false;

    // ---- Field Surround -------------------------------------------------------
    float surAmount = 0.35f;
    float surSpread = 1.15f;
    float surBass   = 0.70f;
    float surCenter = 1.00f;

    // ---- Compressor -----------------------------------------------------------
    float compThresholdDb = -18.0f;
    float compRatio       = 3.0f;
    float compAttackMs    = 10.0f;
    float compReleaseMs   = 150.0f;
    float compMakeupDb    = 0.0f;

    // ---- Limiter --------------------------------------------------------------
    float limCeilingDb = -0.3f;
    float limReleaseMs = 120.0f;

    // ---- Master / AGC ---------------------------------------------------------
    float volDb      = -6.0f;
    bool  agcEnabled = true;
    float agcTargetDb = -18.0f;

    // ---- Bypass ---------------------------------------------------------------
    bool eqBypass = false, revBypass = false, surBypass = false;
    bool compBypass = false, limBypass = false, globalBypass = false;

    // ---- Devices --------------------------------------------------------------
    int   captureMode = 0;          // 0 = device (VB-Cable), 1 = WASAPI loopback
    juce::String inputDevice, outputDevice;
    float windowW = 1280.0f, windowH = 820.0f;

    // ------------------------------------------------------------------ XML ---
    juce::XmlElement* toXml() const
    {
        auto* root = new juce::XmlElement ("AudioFXParams");
        auto* eq = root->createNewChildElement ("EQ");
        for (int i = 0; i < 10; ++i)
            eq->setAttribute ("b" + juce::String (i), (double) eqGainDb[i]);
        eq->setAttribute ("bypass", eqBypass);

        auto* rev = root->createNewChildElement ("Reverb");
        rev->setAttribute ("room",   (double) revRoom);
        rev->setAttribute ("damp",   (double) revDamp);
        rev->setAttribute ("wet",    (double) revWet);
        rev->setAttribute ("dry",    (double) revDry);
        rev->setAttribute ("width",  (double) revWidth);
        rev->setAttribute ("freeze", revFreeze);
        rev->setAttribute ("bypass", revBypass);

        auto* sur = root->createNewChildElement ("Surround");
        sur->setAttribute ("amount", (double) surAmount);
        sur->setAttribute ("spread", (double) surSpread);
        sur->setAttribute ("bass",   (double) surBass);
        sur->setAttribute ("center", (double) surCenter);
        sur->setAttribute ("bypass", surBypass);

        auto* comp = root->createNewChildElement ("Compressor");
        comp->setAttribute ("threshold", (double) compThresholdDb);
        comp->setAttribute ("ratio",     (double) compRatio);
        comp->setAttribute ("attack",    (double) compAttackMs);
        comp->setAttribute ("release",   (double) compReleaseMs);
        comp->setAttribute ("makeup",    (double) compMakeupDb);
        comp->setAttribute ("bypass",    compBypass);

        auto* lim = root->createNewChildElement ("Limiter");
        lim->setAttribute ("ceiling", (double) limCeilingDb);
        lim->setAttribute ("release", (double) limReleaseMs);
        lim->setAttribute ("bypass",  limBypass);

        auto* mas = root->createNewChildElement ("Master");
        mas->setAttribute ("volume", (double) volDb);
        mas->setAttribute ("agc",    agcEnabled);
        mas->setAttribute ("target", (double) agcTargetDb);

        root->setAttribute ("globalBypass", globalBypass);
        root->setAttribute ("captureMode", captureMode);
        root->setAttribute ("inputDevice", inputDevice);
        root->setAttribute ("outputDevice", outputDevice);
        root->setAttribute ("windowW", (double) windowW);
        root->setAttribute ("windowH", (double) windowH);
        return root;
    }

    void fromXml (const juce::XmlElement& root)
    {
        if (auto* eq = root.getChildByName ("EQ"))
        {
            for (int i = 0; i < 10; ++i)
                eqGainDb[i] = (float) eq->getDoubleAttribute ("b" + juce::String (i), 0.0);
            eqBypass = eq->getBoolAttribute ("bypass", false);
        }
        if (auto* rev = root.getChildByName ("Reverb"))
        {
            revRoom   = (float) rev->getDoubleAttribute ("room",   revRoom);
            revDamp   = (float) rev->getDoubleAttribute ("damp",   revDamp);
            revWet    = (float) rev->getDoubleAttribute ("wet",    revWet);
            revDry    = (float) rev->getDoubleAttribute ("dry",    revDry);
            revWidth  = (float) rev->getDoubleAttribute ("width",  revWidth);
            revFreeze = rev->getBoolAttribute ("freeze", false);
            revBypass = rev->getBoolAttribute ("bypass", false);
        }
        if (auto* sur = root.getChildByName ("Surround"))
        {
            surAmount = (float) sur->getDoubleAttribute ("amount", surAmount);
            surSpread = (float) sur->getDoubleAttribute ("spread", surSpread);
            surBass   = (float) sur->getDoubleAttribute ("bass",   surBass);
            surCenter = (float) sur->getDoubleAttribute ("center", surCenter);
            surBypass = sur->getBoolAttribute ("bypass", false);
        }
        if (auto* comp = root.getChildByName ("Compressor"))
        {
            compThresholdDb = (float) comp->getDoubleAttribute ("threshold", compThresholdDb);
            compRatio       = (float) comp->getDoubleAttribute ("ratio",     compRatio);
            compAttackMs    = (float) comp->getDoubleAttribute ("attack",    compAttackMs);
            compReleaseMs   = (float) comp->getDoubleAttribute ("release",   compReleaseMs);
            compMakeupDb    = (float) comp->getDoubleAttribute ("makeup",    compMakeupDb);
            compBypass      = comp->getBoolAttribute ("bypass", false);
        }
        if (auto* lim = root.getChildByName ("Limiter"))
        {
            limCeilingDb = (float) lim->getDoubleAttribute ("ceiling", limCeilingDb);
            limReleaseMs = (float) lim->getDoubleAttribute ("release", limReleaseMs);
            limBypass    = lim->getBoolAttribute ("bypass", false);
        }
        if (auto* mas = root.getChildByName ("Master"))
        {
            volDb      = (float) mas->getDoubleAttribute ("volume", volDb);
            agcEnabled = mas->getBoolAttribute ("agc", true);
            agcTargetDb = (float) mas->getDoubleAttribute ("target", agcTargetDb);
        }
        globalBypass = root.getBoolAttribute ("globalBypass", false);
        captureMode  = (int) root.getIntAttribute ("captureMode", 0);
        inputDevice  = root.getStringAttribute ("inputDevice");
        outputDevice = root.getStringAttribute ("outputDevice");
        windowW = (float) root.getDoubleAttribute ("windowW", 1280.0);
        windowH = (float) root.getDoubleAttribute ("windowH", 820.0);
    }

    // -------------------------------------------------------- apply to DSP ---
    void applyTo (audiofx::ProcessingChain& chain) const
    {
        for (int i = 0; i < 10; ++i)
            chain.getEq().setGainDb (i, eqGainDb[i]);
        chain.getEq().setBypassed (eqBypass);

        chain.getReverb().setRoomSize (revRoom);
        chain.getReverb().setDamping (revDamp);
        chain.getReverb().setWetLevel (revWet);
        chain.getReverb().setDryLevel (revDry);
        chain.getReverb().setWidth (revWidth);
        chain.getReverb().setFreeze (revFreeze);
        chain.getReverb().setBypassed (revBypass);

        chain.getSurround().setAmount (surAmount);
        chain.getSurround().setSpread (surSpread);
        chain.getSurround().setBassFocus (surBass);
        chain.getSurround().setCenter (surCenter);
        chain.getSurround().setBypassed (surBypass);

        chain.getCompressor().setThresholdDb (compThresholdDb);
        chain.getCompressor().setRatio (compRatio);
        chain.getCompressor().setAttackMs (compAttackMs);
        chain.getCompressor().setReleaseMs (compReleaseMs);
        chain.getCompressor().setMakeupDb (compMakeupDb);
        chain.getCompressor().setBypassed (compBypass);

        chain.getLimiter().setCeilingDb (limCeilingDb);
        chain.getLimiter().setReleaseMs (limReleaseMs);
        chain.getLimiter().setBypassed (limBypass);

        chain.getMaster().setVolumeDb (volDb);
        chain.getMaster().setAgcEnabled (agcEnabled);
        chain.getMaster().setAgcTargetDb (agcTargetDb);

        chain.setGlobalBypass (globalBypass);
    }

    static Params fromChain (const audiofx::ProcessingChain& chain)
    {
        Params p;
        for (int i = 0; i < 10; ++i)
            p.eqGainDb[i] = chain.getEq().getGainDb (i);
        p.eqBypass = chain.getEq().isBypassed();

        p.revRoom   = chain.getReverb().getRoomSize();
        p.revDamp   = chain.getReverb().getDamping();
        p.revWet    = chain.getReverb().getWetLevel();
        p.revDry    = chain.getReverb().getDryLevel();
        p.revWidth  = chain.getReverb().getWidth();
        p.revFreeze = chain.getReverb().getFreeze();
        p.revBypass = chain.getReverb().isBypassed();

        p.surAmount = chain.getSurround().getAmount();
        p.surSpread = chain.getSurround().getSpread();
        p.surBass   = chain.getSurround().getBassFocus();
        p.surCenter = chain.getSurround().getCenter();
        p.surBypass = chain.getSurround().isBypassed();

        p.compThresholdDb = chain.getCompressor().getThresholdDb();
        p.compRatio       = chain.getCompressor().getRatio();
        p.compAttackMs    = chain.getCompressor().getAttackMs();
        p.compReleaseMs   = chain.getCompressor().getReleaseMs();
        p.compMakeupDb    = chain.getCompressor().getMakeupDb();
        p.compBypass      = chain.getCompressor().isBypassed();

        p.limCeilingDb = chain.getLimiter().getCeilingDb();
        p.limReleaseMs = chain.getLimiter().getReleaseMs();
        p.limBypass    = chain.getLimiter().isBypassed();

        p.volDb      = chain.getMaster().getVolumeDb();
        p.agcEnabled = chain.getMaster().isAgcEnabled();
        p.agcTargetDb = chain.getMaster().getAgcTargetDb();

        p.globalBypass = chain.isGlobalBypass();
        return p;
    }
};
