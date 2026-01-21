#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/Parameters.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      vts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Get raw parameter pointers for real-time safe access
    highShelfParam = vts.getRawParameterValue(highShelfID.getParamID());
    highShelfGainParam = vts.getRawParameterValue(highShelfGainID.getParamID());
    midPeakParam = vts.getRawParameterValue(midPeakID.getParamID());
    midPeakGainParam = vts.getRawParameterValue(midPeakGainID.getParamID());
    lowShelfParam = vts.getRawParameterValue(lowShelfID.getParamID());
    lowShelfGainParam = vts.getRawParameterValue(lowShelfGainID.getParamID());
    qModeParam = vts.getRawParameterValue(qModeID.getParamID());
    bypassParam = vts.getRawParameterValue(bypassID.getParamID());

    // Add parameter listeners
    vts.addParameterListener(highShelfID.getParamID(), this);
    vts.addParameterListener(highShelfGainID.getParamID(), this);
    vts.addParameterListener(midPeakID.getParamID(), this);
    vts.addParameterListener(midPeakGainID.getParamID(), this);
    vts.addParameterListener(lowShelfID.getParamID(), this);
    vts.addParameterListener(lowShelfGainID.getParamID(), this);
    vts.addParameterListener(qModeID.getParamID(), this);
    vts.addParameterListener(bypassID.getParamID(), this);
}

PluginProcessor::~PluginProcessor()
{
    // Remove parameter listeners
    vts.removeParameterListener(highShelfID.getParamID(), this);
    vts.removeParameterListener(highShelfGainID.getParamID(), this);
    vts.removeParameterListener(midPeakID.getParamID(), this);
    vts.removeParameterListener(midPeakGainID.getParamID(), this);
    vts.removeParameterListener(lowShelfID.getParamID(), this);
    vts.removeParameterListener(lowShelfGainID.getParamID(), this);
    vts.removeParameterListener(qModeID.getParamID(), this);
    vts.removeParameterListener(bypassID.getParamID(), this);
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // High Shelf: frequency (20Hz - 20kHz, default 1kHz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        highShelfID,
        highShelfName,
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // High Shelf: Gain (-24dB to +24dB, default 0dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        highShelfGainID,
        highShelfGainName,
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Mid Peak: frequency (20Hz - 20kHz, default 1kHz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        midPeakID,
        midPeakName,
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // Mid Peak: Gain (-24dB to +24dB, default 0dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        midPeakGainID,
        midPeakGainName,
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Low Shelf: frequency (20Hz - 20kHz, default 1kHz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        lowShelfID,
        lowShelfName,
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // Low Shelf: Gain (-24dB to +24dB, default 0dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        lowShelfGainID,
        lowShelfGainName,
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Q Mode: Constant Q or Proportional Q (default Constant Q)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        qModeID,
        qModeName,
        qModeItems,
        0  // default: Constant Q
    ));

    // Bypass: On/Off (default Off)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        bypassID,
        bypassName,
        false
    ));

    return { params.begin(), params.end() };
}

void PluginProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    juce::ignoreUnused(newValue);
    
    // Update engine parameters when any filter or Q mode parameter changes
    if (paramID == highShelfID.getParamID() || 
        paramID == highShelfGainID.getParamID() ||
        paramID == midPeakID.getParamID() || 
        paramID == midPeakGainID.getParamID() ||
        paramID == lowShelfID.getParamID() ||
        paramID == lowShelfGainID.getParamID() ||
        paramID == qModeID.getParamID())
    {
        updateParameters();
    }
}

void PluginProcessor::updateParameters()
{
    if (highShelfParam == nullptr || highShelfGainParam == nullptr ||
        midPeakParam == nullptr || midPeakGainParam == nullptr ||
        lowShelfParam == nullptr || lowShelfGainParam == nullptr ||
        qModeParam == nullptr)
        return;

    // Get current Q mode from parameter (0 = Constant_Q, 1 = Proportional_Q)
    const QMode currentQMode = (qModeParam->load() < 0.5f) ? QMode::Constant_Q : QMode::Proportional_Q;

    // Engine 0: High Shelf filter
    engines[0].setParameters(highShelfParam->load(), highShelfGainParam->load(), defaultQ, FilterType::HighShelf, currentQMode);

    // Engine 1: Mid-Peak (Peaking) filter
    engines[1].setParameters(midPeakParam->load(), midPeakGainParam->load(), defaultQ, FilterType::Peaking, currentQMode);

    // Engine 2: Low Shelf filter
    engines[2].setParameters(lowShelfParam->load(), lowShelfGainParam->load(), defaultQ, FilterType::LowShelf, currentQMode);
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare all engines with the current sample rate and block size
    for (auto& engine : engines)
    {
        engine.prepare(sampleRate, samplesPerBlock);
    }

    // Set initial parameters
    updateParameters();
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                   juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // Standard JUCE safety: clear any output channels beyond what we're using
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Check bypass state
    const bool bypassed = bypassParam != nullptr && bypassParam->load() > 0.5f;

    if (bypassed)
    {
        // Bypass: pass audio through unchanged, but reset filter states
        // to avoid clicks when bypass is turned off
        for (auto& engine : engines)
        {
            engine.reset();
        }
        return;
    }

    // Update parameters from the atomic values (real-time safe)
    updateParameters();

    // Process through each engine in series: HPF -> Notch -> LPF
    for (auto& engine : engines)
    {
        engine.processBlock(buffer);
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *PluginProcessor::createEditor()
{ // Use generic gui for editor for now
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock &destData)
{
}

void PluginProcessor::setStateInformation(const void *data, int sizeInBytes)
{
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
