#pragma once

#ifndef BIQUAD3_QCALC_H
#define BIQUAD3_QCALC_H

#include <cmath>
#include <algorithm>
#include <numbers>

struct BiquadCoeffs { double b0, b1, b2, a1, a2; };

enum class QMode { Constant_Q, Proportional_Q };
enum class FilterType { Peaking, LowShelf, HighShelf };

class Qcalc {
public:
    static BiquadCoeffs calculate(double sampleRate,
                                    double frequency,
                                    double gainDB,
                                    double qControl,
                                    QMode mode,
                                    FilterType type)
    {
        if (sampleRate <= 0.0 || frequency <= 0.0) {
            return { 1.0, 0.0, 0.0, 0.0, 0.0 };
        }

        // Keep frequency inside a safe open interval to avoid pathological sin/cos behavior.
        frequency = std::clamp(frequency, 1.0e-9, 0.5 * sampleRate - 1.0e-9);

        // A = 10^(dBgain/40)
        const double ln10 = std::numbers::ln10_v<double>;
        const double A = std::exp(gainDB * (ln10 / 40.0));
        const double sqrtA = std::sqrt(A);

        const double w0 = (2.0 * std::numbers::pi_v<double>) * frequency / sampleRate;
        
        // Compiler will often optimize adjacent sin/cos into a single 'fsincos' instruction
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);

        double finalQ = qControl;
    
        // Opt 2: Branchless logic for Proportional Q
        if (type == FilterType::Peaking && mode == QMode::Proportional_Q) {
            const double minQ = 0.5;
            const double maxQ = 3.0;
            // 1.0 / 12.0 = 0.08333...
            double gainFactor = std::min(std::abs(gainDB) * 0.0833333333333333, 1.0);
            finalQ = minQ + (gainFactor * (maxQ - minQ));
            finalQ *= qControl;
        }

        // Clamp to avoid division by zero / invalid sqrt when parameters are abused.
        finalQ = std::max(finalQ, 1.0e-9);

        double b0, b1, b2, a0, a1, a2;
        
        switch (type) {
            case FilterType::LowShelf: {
                // (low shelf). For shelves, `qControl` is treated as shelf slope S.
                const double minS = 1.0e-9;
                double S = std::max(qControl, minS);

                // Shelf alpha domain constraint (RBJ):
                // radicand = (A + 1/A) * (1/S - 1) + 2 must be >= 0.
                // For gain != 0, this implies an upper bound on S:
                // S <= (A + 1/A) / ((A + 1/A) - 2).
                const double k = A + (1.0 / A);
                const double denom = k - 2.0;
                if (denom > 0.0) {
                    const double sMax = k / denom;
                    S = std::min(S, sMax);
                }

                const double radicand = (k * ((1.0 / S) - 1.0)) + 2.0;
                const double alpha = (sinW0 / 2.0) * std::sqrt(std::max(radicand, 0.0));

                const double Ap1 = A + 1.0;
                const double Am1 = A - 1.0;
                const double twoSqrtAAlpha = 2.0 * sqrtA * alpha;

                b0 = A * (Ap1 - (Am1 * cosW0) + twoSqrtAAlpha);
                b1 = 2.0 * A * (Am1 - (Ap1 * cosW0));
                b2 = A * (Ap1 - (Am1 * cosW0) - twoSqrtAAlpha);
                a0 = Ap1 + (Am1 * cosW0) + twoSqrtAAlpha;
                a1 = -2.0 * (Am1 + (Ap1 * cosW0));
                a2 = Ap1 + (Am1 * cosW0) - twoSqrtAAlpha;
            }
            break;
        
            case FilterType::HighShelf: {
                // (high shelf). For shelves, `qControl` is treated as shelf slope S.
                const double minS = 1.0e-9;
                double S = std::max(qControl, minS);

                // Shelf alpha domain constraint (RBJ):
                // radicand = (A + 1/A) * (1/S - 1) + 2  must be >= 0.
                // For gain != 0, this implies an upper bound on S:
                // S <= (A + 1/A) / ((A + 1/A) - 2).
                const double k = A + (1.0 / A);
                const double denom = k - 2.0;
                if (denom > 0.0) {
                    const double sMax = k / denom;
                    S = std::min(S, sMax);
                }

                const double radicand = (k * ((1.0 / S) - 1.0)) + 2.0;
                const double alpha = (sinW0 / 2.0) * std::sqrt(std::max(radicand, 0.0));

                const double Ap1 = A + 1.0;
                const double Am1 = A - 1.0;
                const double twoSqrtAAlpha = 2.0 * sqrtA * alpha;

                b0 = A * (Ap1 + (Am1 * cosW0) + twoSqrtAAlpha);
                b1 = -2.0 * A * (Am1 + (Ap1 * cosW0));
                b2 = A * (Ap1 + (Am1 * cosW0) - twoSqrtAAlpha);
                a0 = Ap1 - (Am1 * cosW0) + twoSqrtAAlpha;
                a1 = 2.0 * (Am1 - (Ap1 * cosW0));
                a2 = Ap1 - (Am1 * cosW0) - twoSqrtAAlpha;
            }    
            break;
            
            case FilterType::Peaking:
            default:
            {
                const double alpha = sinW0 / (2.0 * finalQ);
                // Opt 3: Peaking Symmetry (b2 = 2-b0, a2 = 2-a0, b1 = a1)
                const double alphaA = alpha * A;
                const double alphaDivA = alpha / A;

                b0 = 1.0 + alphaA;
                b2 = 2.0 - b0; // == 1.0 - alphaA
                a0 = 1.0 + alphaDivA;
                a2 = 2.0 - a0; // == 1.0 - alphaDivA
                
                b1 = -2.0 * cosW0;
                a1 = b1;
            }
            break;
        }
    
        double invA0 = 1.0 / a0;
        return { b0 * invA0, b1 * invA0, b2 * invA0, a1 * invA0, a2 * invA0 };
    }
};

#endif