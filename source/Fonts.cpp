#include "Fonts.h"

const juce::Typeface::Ptr Fonts::typeface = juce::Typeface::createSystemTypefaceFor(nullptr, 0);

juce::Font Fonts::getFont(float height)
{
    return juce::Font(height);
}
