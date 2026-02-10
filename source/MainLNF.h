#pragma once

#ifndef BIQUAD3_MAINLNF_H
#define BIQUAD3_MAINLNF_H

#include <JuceHeader.h>
#include "Colors.h"
#include "Fonts.h"

class MainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MainLookAndFeel();

    juce::Font getLabelFont(juce::Label&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLookAndFeel)
};

#endif