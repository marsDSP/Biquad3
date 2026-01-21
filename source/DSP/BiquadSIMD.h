#pragma once

#ifndef BIQUAD3_BIQUADSIMD_H
#define BIQUAD3_BIQUADSIMD_H

#include <array>
#include "xsimd/include/xsimd/xsimd.hpp"
#include "Qcalc.h"

/*
 * Stereo biquad using SIMD across channels (L/R in lanes 0/1).
 * Filter structure: Direct Form II Transposed.
 */
class alignas(16) BiquadSIMD {
public:
    BiquadSIMD() { reset(); }

    void reset() noexcept
    {
        z1 = Batch(0.0f);
        z2 = Batch(0.0f);
    }

    void setCoeffs(const BiquadCoeffs& coeffs) noexcept
    {
        b0_vec = Batch(static_cast<float>(coeffs.b0));
        b1_vec = Batch(static_cast<float>(coeffs.b1));
        b2_vec = Batch(static_cast<float>(coeffs.b2));
        a1_vec = Batch(static_cast<float>(coeffs.a1));
        a2_vec = Batch(static_cast<float>(coeffs.a2));
    }

    inline void processStereo(const float* leftIn,
                              const float* rightIn,
                              float* leftOut,
                              float* rightOut) noexcept
    {
        std::array<float, Batch::size> xBuf{};
        xBuf[0] = *leftIn;
        xBuf[1] = *rightIn;
        const Batch x = Batch::load_unaligned(xBuf.data());

        const Batch y = x * b0_vec + z1;
        const Batch newZ1 = (x * b1_vec + z2) - (y * a1_vec);
        const Batch newZ2 = (x * b2_vec) - (y * a2_vec);

        z1 = newZ1;
        z2 = newZ2;

        std::array<float, Batch::size> yBuf{};
        y.store_unaligned(yBuf.data());
        *leftOut = yBuf[0];
        *rightOut = yBuf[1];
    }

    void processBlock(float* const* channelData, int numSamples) noexcept
    {
        if (channelData == nullptr || numSamples <= 0) {
            return;
        }

        float* L = channelData[0];
        float* R = channelData[1];
        if (L == nullptr || R == nullptr) {
            return;
        }

        for (int i = 0; i < numSamples; ++i) {
            processStereo(&L[i], &R[i], &L[i], &R[i]);
        }
    }

private:
    using Batch = xsimd::batch<float>;

    Batch b0_vec{}, b1_vec{}, b2_vec{}, a1_vec{}, a2_vec{};
    Batch z1{}, z2{};
};

#endif
