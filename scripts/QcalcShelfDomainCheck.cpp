#include <cmath>
#include <iostream>

#include "DSP/Qcalc.h"

static bool allFinite(const BiquadCoeffs& c)
{
    return std::isfinite(c.b0) && std::isfinite(c.b1) && std::isfinite(c.b2) &&
           std::isfinite(c.a1) && std::isfinite(c.a2);
}

int main()
{
    // Chosen to trigger the RBJ shelf alpha domain issue when S is too large:
    // radicand = (A + 1/A) * (1/S - 1) + 2
    // With gain != 0 and S >> 1, this can go negative without clamping.
    const double sampleRate = 48000.0;
    const double frequency = 1000.0;
    const double gainDB = 24.0;
    const double shelfSlopeS = 10.0;

    const auto low = Qcalc::calculate(sampleRate, frequency, gainDB, shelfSlopeS,
                                      QMode::Constant_Q, FilterType::LowShelf);
    const auto high = Qcalc::calculate(sampleRate, frequency, gainDB, shelfSlopeS,
                                       QMode::Constant_Q, FilterType::HighShelf);

    bool ok = true;
    if (!allFinite(low)) {
        std::cerr << "LowShelf produced non-finite coefficients for S=" << shelfSlopeS << "\n";
        ok = false;
    }
    if (!allFinite(high)) {
        std::cerr << "HighShelf produced non-finite coefficients for S=" << shelfSlopeS << "\n";
        ok = false;
    }

    if (!ok) {
        return 1;
    }

    std::cout << "OK: shelf coefficients are finite." << std::endl;
    return 0;
}
