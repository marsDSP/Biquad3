#pragma once

#include <JuceHeader.h>
#include "DSP/Engine.h"

#include <array>
#include <atomic>
#include <memory>

// we don't need to force everything including this class to recompile if we
// end up changing or editing params! (because we used std::unique_ptr)
class Parameters;

class PluginProcessor : public juce::AudioProcessor, juce::AudioProcessorValueTreeState::Listener {
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getTreeState() { return vts; }

private:

    juce::AudioProcessorValueTreeState vts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void parameterChanged (const juce::String& paramID, float newValue) override;
    void updateParameters();

    // Atomic parameter pointers for real-time safe access
    std::atomic<float>* highShelfParam = nullptr;
    std::atomic<float>* highShelfGainParam = nullptr;
    std::atomic<float>* midPeakParam = nullptr;
    std::atomic<float>* midPeakGainParam = nullptr;
    std::atomic<float>* lowShelfParam = nullptr;
    std::atomic<float>* lowShelfGainParam = nullptr;
    std::atomic<float>* qModeParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    // Engines: 0 = HighShelf, 1 = MidPeak (Peaking), 2 = LowShelf
    static constexpr int NUM_ENGINES = 3;
    std::array<Engine, NUM_ENGINES> engines;

    // Default Q value for filters
    static constexpr float defaultQ = 0.707f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
