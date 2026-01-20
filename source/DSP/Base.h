#pragma once

#ifndef BIQUAD3_BASE_H
#define BIQUAD3_BASE_H

#include <JuceHeader.h>

/*
 * The Curiously Recurring Template Pattern (CRTP)
 *
 * Another optimization here; normally I'd just write a base class and inherit from it.
 * However, it is suboptimal to include a virtual function call inside the inner process loop.
 * Devirtualization is not a trivial optimization here; the older, obsolete method prevents
 * the compiler from inlining the code, blocks vectorization, and adds function call overhead.
 */

template <typename SampleType>
class Base {
public:
    Base() = default;
    virtual ~Base() = default;

    /*
     * It should be noted here that instead of 'int' and 'size_t'
     * I am instead using auto i{0uz}. This is a C++23 optimization;
     * It prevents signed/unsigned mismatch warnings when comparing
     * with container sizes like hpfSmoother.size(). Good stuff.
     */

    void prepare(const juce::dsp::ProcessSpec& spec) {
        for (auto i{0uz}; i < hpfSmoother.size(); ++i) {
            hpfSmoother[i].reset(spec.sampleRate, 0.02);
            notchSmoother[i].reset(spec.sampleRate, 0.02);
            lpfSmoother[i].reset(spec.sampleRate, 0.02);
        }
    }

    // num_samples was changed to size_t. This matches juce::dsp::AudioBlock::getNumSamples()
    // and avoids mixing signed (int) and unsigned (size_t) types in loop comparisons.
    void process(const juce::dsp::AudioBlock<float>& block, const size_t num_samples) {
        for (auto ch{0uz}; ch < block.getNumChannels(); ++ch)
        {
            auto *data = block.getChannelPointer(ch);

            for (auto smp{0uz}; smp < num_samples; ++smp)
            {
                const float xn = data[smp];
                const float yn = static_cast<SampleType*>(this)->processSample(xn);
                data[smp] = yn;
            }
        }
    }

    virtual float processSample(float xn) = 0;

    std::array<juce::SmoothedValue<float>, 3>& getHPF() { return hpfSmoother; }
    std::array<juce::SmoothedValue<float>, 3>& getNotch() { return notchSmoother; }
    std::array<juce::SmoothedValue<float>, 3>& getLPF() { return lpfSmoother; }

private:
    std::array<juce::SmoothedValue<float>, 3>
    hpfSmoother,
    notchSmoother,
    lpfSmoother;
};

#endif