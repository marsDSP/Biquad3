#pragma once

#include <JuceHeader.h>
#include "DSP/Engine.h"

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

    juce::AudioProcessorValueTreeState& vts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void parameterChanged (const juce::String& paramID, float newValue) override;
    void updateParameters();

    /*
     * I used to just use Parameters params; here.
     * I do now think it is more prudent to be able to ensure the processor
     * is fully initialized before the parameter logic starts running or hooking into callbacks.
     * Being able to control when objects are created is worth the heap allocation.
     *
     * If the Parameters class eventually needs a reference to the PluginProcessor or VTS
     * (to attach listeners or look up parameter IDs), passing *this in the
     * initialization list can be dangerous. PluginProcessor isn't fully built yet.
     * std::unique_ptr delays this construction until the body of the constructor
     * where it is safe to pass *this or other members.
     *
     * However, if at any point we don't have all the info needed to create the
     * parameters object immediately, we can just leave it as nullptr initially.
     *
     * We can also reset the parameters object if we ever need to completely reload
     * the parameter structure!
     */

    std::unique_ptr<Parameters> params;

    std::array<Engine, 5> engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
