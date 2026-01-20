#pragma once

#ifndef BIQUAD3_POLYPHASEAVX_H
#define BIQUAD3_POLYPHASEAVX_H

#include <immintrin.h> // AVX, AVX2, FMA
#include <vector>
#include <algorithm>

/*
 * We're using AVX Fused Multiply-Add here, because otherwise the math for a 32x oversample
 * would cripple the CPU. Especially one with high-quality kernels (many taps).. it just won't work lol.
 * The immediate solution is AVX with '_mm256_fmadd_ps' here, this instruction performs (a * b) + c in a
 * single CPU cycle for 8 floats at once.
 *
 * So instead of inefficiently looping through taps 1 by 1, we get to process 8 taps per cycle.
 * 8 coefficients go into one register, 8 state (z-1) samples into another, multiply-add them into an accumulator,
 * and sum the 8 values inside the accumulator register to get the single yn output at the end.
 */

class PolyphaseStageAVX {

};

#endif