#pragma once

#ifndef BIQUAD3_BIQUADNEON_H
#define BIQUAD3_BIQUADNEON_H

// xsimd provides a portable SIMD layer (NEON on arm64).
#include "BiquadSIMD.h"

// Kept as a named type for Engine's arch selection.
using BiquadNEON = BiquadSIMD;

#endif