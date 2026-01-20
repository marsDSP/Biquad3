#pragma once

#ifndef BIQUAD3_RESAMPLER_H
#define BIQUAD3_RESAMPLER_H

#include <JuceHeader.h>
#include "Interpolator.h"

template <typename InterpolationType>
class Resampler {
public:
    Resampler() {}

private:
    std::vector<float> buffer;
    int writeIdx = 0;
};

#endif