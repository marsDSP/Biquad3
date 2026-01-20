#pragma once

#ifndef BIQUAD3_PARAMETERS_H
#define BIQUAD3_PARAMETERS_H

#include <JuceHeader.h>

// ============================================ //

static inline const juce::StringArray osItems = { "Off", "2x", "4x", "8x", "16x" };
static inline const juce::StringArray qItems = {};

// ============================================ //

static const juce::ParameterID osChoiceID = { "osChoice", 1};
static constexpr auto osChoiceName = "Oversampling";

static const juce::ParameterID qChoiceID = { "qChoice", 1 };
static constexpr auto qChoiceName = "Q";

static const juce::ParameterID hpfID = { "hpfID", 1 };
static constexpr auto hpfName = "HPF";

static const juce::ParameterID notchID = { "notchID", 1 };
static constexpr auto notchName = "Notch";

static const juce::ParameterID lpfID = { "lpfID", 1 };
static constexpr auto lpfName = "LPF";

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