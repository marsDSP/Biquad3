#pragma once

#ifndef BIQUAD3_BIQUADAVX_H
#define BIQUAD3_BIQUADAVX_H

// xsimd provides a portable SIMD layer (SSE/AVX/NEON as available).
#include "BiquadSIMD.h"

// Kept as a named type for Engine's arch selection.
using BiquadAVX = BiquadSIMD;

#endif