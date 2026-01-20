#pragma once

#ifndef BIQUAD3_BIQUADNEON_H
#define BIQUAD3_BIQUADNEON_H

#include <arm_neon.h>
#include "Qcalc.h"

/*
 * Direct Form II Transposed Biquadratic Filter with ARM NEON SIMD optimization
 */

class BiquadNEON {
public:
    BiquadNEON () = default;

private:
};

#endif