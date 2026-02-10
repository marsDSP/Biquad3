#pragma once

#ifndef BIQUAD3_COLORS_H
#define BIQUAD3_COLORS_H

#include <JuceHeader.h>

namespace Colors
{
    const juce::Colour background { 245, 240, 235 };
    const juce::Colour header { 40, 40, 40 };

    namespace Group
    {
        const juce::Colour label { 0, 0, 0 };
        const juce::Colour outline { 0, 0, 0 };
    }

    namespace Knob
    {
        const juce::Colour trackBackground { 205, 200, 195 };
        const juce::Colour trackActive { 177, 101, 135 };
        const juce::Colour outline { 255, 250, 245 };
        const juce::Colour gradientTop { 250, 245, 240 };
        const juce::Colour gradientBottom { 240, 235, 230 };
        const juce::Colour dial { 100, 100, 100 };
        const juce::Colour dropShadow { 195, 190, 185 };
        const juce::Colour label { 80, 80, 80 };
        const juce::Colour textBoxBackground { 80, 80, 80 };
        const juce::Colour value { 240, 240, 240 };
        const juce::Colour caret { 255, 255, 255 };
    }

    namespace Button
    {
        const juce::Colour text { 80, 80, 80 };
        const juce::Colour textToggled { 40, 40, 40 };
        const juce::Colour background { 245, 240, 235 };
        const juce::Colour backgroundToggled { 255, 250, 245 };
        const juce::Colour outline { 235, 230, 225 };
    }

    namespace LevelMeter
    {
        const juce::Colour background { 245, 240, 235 };
        const juce::Colour tickLine { 200, 200, 200 };
        const juce::Colour tickLabel { 80, 80, 80 };
        const juce::Colour tooLoud { 226, 74, 81 };
        const juce::Colour levelOK { 65, 206, 88 };
    }
}

#endif