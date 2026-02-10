#pragma once

#ifndef BIQUAD3_FONTS_H
#define BIQUAD3_FONTS_H

#include <JuceHeader.h>

class Fonts
{
public:
    static juce::Font getFont(float height = 16.0f);

    Fonts() = delete;

private:
    static const juce::Typeface::Ptr typeface;
};

#endif