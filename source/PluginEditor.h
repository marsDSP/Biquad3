#pragma once

#include "PluginProcessor.h"
#include "FFT.h"
#include "ResponseCurve.h"
#include "RotaryKnob.h"
#include "LevelMeter.h"
#include "LookAndFeel.h"
#include "MainLNF.h"
#include "Utils/Parameters.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include <sst/jucegui/components/MenuButton.h>

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processorRef;

    MainLookAndFeel mainLF;

    FFTSpectrumComponent fftComponent;

    juce::GroupComponent menuGroup, shapingGroup;
    juce::GroupComponent lowShelfGroup, peakGroup, highShelfGroup, outputGroup;

    RotaryKnob lowShelfFreqKnob;
    RotaryKnob lowShelfGainKnob;
    RotaryKnob midPeakFreqKnob;
    RotaryKnob midPeakGainKnob;
    RotaryKnob highShelfFreqKnob;
    RotaryKnob highShelfGainKnob;

    LevelMeter levelMeter;

    std::unique_ptr<sst::jucegui::components::MenuButton> modelMenu, configMenu;

    std::unique_ptr<sst::jucegui::components::MenuButton> pbMenu, slpMenu, drvMenu, fsmMenu;

    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
