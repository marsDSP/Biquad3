#pragma once

#include <JuceHeader.h>
#include "Qcalc.h"

// auto selects correct architecture
#if defined(_arm64_) || defined(__aarch64__)
    #include "BiquadNEON.h"
    #include "PolyphaseNEON.h"
    using Biquad = BiquadNEON;
    using PolyphaseStage = PolyphaseStageNEON;
else
    #include "BiquadAVX.h"
    #include "PolyphaseAVX.h"
    using Biquad = BiquadAVX;
    using PolyphaseStage = PolyphaseStageAVX;
#endif

class Engine {
public:
    Engine() = default;

private:
};
