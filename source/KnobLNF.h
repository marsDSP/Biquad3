#pragma once

#ifndef BIQUAD3_KNOBLNF_H
#define BIQUAD3_KNOBLNF_H

#include <JuceHeader.h>
#include "Colors.h"

class RotaryKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:

    void drawTextEditorOutline(juce::Graphics&, int, int, juce::TextEditor&) override { }
    void fillTextEditorBackground(juce::Graphics&, int width, int height, juce::TextEditor&) override;

    RotaryKnobLookAndFeel();
    juce::Font getLabelFont(juce::Label&) override;

    static RotaryKnobLookAndFeel* get()
    {
        static RotaryKnobLookAndFeel instance;
        return &instance;
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider ) override;

private:
    juce::Label* createSliderTextBox(juce::Slider&) override;
    juce::DropShadow dropShadow { Colors::Knob::dropShadow, 6, { 0, 3 } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnobLookAndFeel)
};

#endif