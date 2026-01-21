#pragma once

#ifndef BIQUAD3_PARAMETERS_H
#define BIQUAD3_PARAMETERS_H

#include <JuceHeader.h>

// ============================================ //

static inline const juce::StringArray osItems = { "Off", "2x", "4x", "8x", "16x" };
static inline const juce::StringArray qModeItems = { "Constant Q", "Proportional Q" };

// ============================================ //

static const juce::ParameterID osChoiceID = { "osChoice", 1};
static constexpr auto osChoiceName = "Oversampling";

static const juce::ParameterID qModeID = { "qModeID", 1 };
static constexpr auto qModeName = "Q Mode";

static const juce::ParameterID highShelfID = { "highShelfID", 1 };
static constexpr auto highShelfName = "High Shelf Freq";

static const juce::ParameterID highShelfGainID = { "highShelfGainID", 1 };
static constexpr auto highShelfGainName = "High Shelf Gain";

static const juce::ParameterID midPeakID = { "midPeakID", 1 };
static constexpr auto midPeakName = "Mid Peak Freq";

static const juce::ParameterID midPeakGainID = { "midPeakGainID", 1 };
static constexpr auto midPeakGainName = "Mid Peak Gain";

static const juce::ParameterID lowShelfID = { "lowShelfID", 1 };
static constexpr auto lowShelfName = "Low Shelf Freq";

static const juce::ParameterID lowShelfGainID = { "lowShelfGainID", 1 };
static constexpr auto lowShelfGainName = "Low Shelf Gain";

static const juce::ParameterID bypassID = { "bypassID", 1 };
static constexpr auto bypassName = "Bypass";

// ============================================ //

class Parameters {
public:
    explicit Parameters() {

    }

    ~Parameters() = default;

private:

    // ============================================ //

    // Parameters
    // Oversample: { OFF, 2x, 4x, 8x, 16x, 32x }
    // QSelector: Some analog stuff? |
    // High-Pass: 0Hz -> 12kHz | Default: 0Hz |
    // Notch: 0Hz <- 6kHz -> 12kHz | Default: 6kHz |
    // Low-Pass: 12kHz -> 0Hz | Default: 20kHz |
    // Bypass: On/Off | Default: Off |

    // ============================================ //

    juce::AudioParameterChoice* osChoice { nullptr };
    juce::AudioParameterChoice* qChoice  { nullptr };

    // ============================================ //

    juce::AudioParameterFloat* hpf   { nullptr };
    juce::AudioParameterFloat* notch { nullptr };
    juce::AudioParameterFloat* lpf   { nullptr };

    // ============================================ //

    juce::AudioParameterBool* bypass   { nullptr };

    // ============================================ //

};

#endif