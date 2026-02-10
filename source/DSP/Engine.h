#pragma once

#include <JuceHeader.h>
#include "Qcalc.h"

// Auto-select the correct SIMD architecture
#if defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
    // ARM64: Apple Silicon (M1/M2/M3), ARM-based devices
    #include "BiquadNEON.h"
    using Biquad = BiquadNEON;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    // x86/x86_64: Intel/AMD processors
    #include "BiquadAVX.h"
    using Biquad = BiquadAVX;
#else
    #error "Unsupported architecture: requires ARM64 (NEON) or x86/x86_64 (AVX)"
#endif

/**
 * Biquad Engine with parameter smoothing.
 * 
 * Wraps a SIMD-optimized Biquad filter with smooth parameter transitions
 * to avoid clicks and zipper noise when parameters change.
 */
class Engine {
public:
    Engine() = default;

    /**
     * Prepare the engine for playback.
     * Must be called before processing, typically from prepareToPlay().
     * 
     * @param sampleRate The sample rate in Hz
     * @param samplesPerBlock Maximum expected block size
     * @param smoothingTimeMs Time in milliseconds for parameter smoothing (default 20ms)
     */
    void prepare(double sampleRate, int samplesPerBlock, double smoothingTimeMs = 20.0)
    {
        currentSampleRate = sampleRate;
        maxBlockSize = samplesPerBlock;

        // Reset smoothed values with the new sample rate
        const double smoothingTimeSec = smoothingTimeMs / 1000.0;
        smoothedFrequency.reset(sampleRate, smoothingTimeSec);
        smoothedGainDB.reset(sampleRate, smoothingTimeSec);
        smoothedQ.reset(sampleRate, smoothingTimeSec);

        // Set initial values (will be overwritten by setParameters)
        smoothedFrequency.setCurrentAndTargetValue(1000.0f);
        smoothedGainDB.setCurrentAndTargetValue(0.0f);
        smoothedQ.setCurrentAndTargetValue(0.707f);

        // Reset the filter state
        biquad.reset();
        
        // Calculate initial coefficients
        updateCoefficients();
    }

    /**
     * Set the target filter parameters.
     * Parameters will smoothly transition to these values over the smoothing time.
     * 
     * @param frequency Center/cutoff frequency in Hz
     * @param gainDB Gain in decibels (for peaking/shelf filters)
     * @param q Q factor or bandwidth
     * @param type Filter type (Peaking, LowShelf, HighShelf)
     * @param mode Q mode (Constant_Q or Proportional_Q)
     */
    void setParameters(float frequency, float gainDB, float q,
                       FilterType type = FilterType::Peaking,
                       QMode mode = QMode::Constant_Q)
    {
        smoothedFrequency.setTargetValue(frequency);
        smoothedGainDB.setTargetValue(gainDB);
        smoothedQ.setTargetValue(q);
        
        filterType = type;
        qMode = mode;
    }

    /**
     * Set parameters immediately without smoothing.
     * Useful for initialization or when instant changes are desired.
     */
    void setParametersImmediate(float frequency, float gainDB, float q,
                                FilterType type = FilterType::Peaking,
                                QMode mode = QMode::Constant_Q)
    {
        smoothedFrequency.setCurrentAndTargetValue(frequency);
        smoothedGainDB.setCurrentAndTargetValue(gainDB);
        smoothedQ.setCurrentAndTargetValue(q);
        
        filterType = type;
        qMode = mode;
        
        updateCoefficients();
    }

    /**
     * Process a stereo audio block with parameter smoothing.
     * Coefficients are updated per-sample when parameters are changing.
     * 
     * @param buffer Audio buffer to process in-place (must have at least 2 channels)
     */
    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        if (numSamples == 0 || buffer.getNumChannels() < 2)
            return;

        float* leftChannel = buffer.getWritePointer(0);
        float* rightChannel = buffer.getWritePointer(1);

        // Check if any parameters are still smoothing
        const bool isSmoothing = smoothedFrequency.isSmoothing() ||
                                  smoothedGainDB.isSmoothing() ||
                                  smoothedQ.isSmoothing();

        if (isSmoothing)
        {
            // Process sample-by-sample with coefficient updates
            for (int i = 0; i < numSamples; ++i)
            {
                // Advance smoothed values
                const float freq = smoothedFrequency.getNextValue();
                const float gain = smoothedGainDB.getNextValue();
                const float q = smoothedQ.getNextValue();

                // Update coefficients if values changed significantly
                if (std::abs(freq - lastFrequency) > 0.01f ||
                    std::abs(gain - lastGainDB) > 0.001f ||
                    std::abs(q - lastQ) > 0.0001f)
                {
                    lastFrequency = freq;
                    lastGainDB = gain;
                    lastQ = q;
                    
                    auto coeffs = Qcalc::calculate(currentSampleRate,
                                                   static_cast<double>(freq),
                                                   static_cast<double>(gain),
                                                   static_cast<double>(q),
                                                   qMode, filterType);
                    biquad.setCoeffs(coeffs);
                }

                // Process single stereo sample
                biquad.processStereo(&leftChannel[i], &rightChannel[i],
                                     &leftChannel[i], &rightChannel[i]);
            }
        }
        else
        {
            // No smoothing needed - process entire block at once (more efficient)
            float* channels[2] = { leftChannel, rightChannel };
            biquad.processBlock(channels, numSamples);
        }
    }

    /**
     * Process a stereo audio block using raw channel pointers.
     * 
     * @param channelData Array of channel pointers [left, right]
     * @param numSamples Number of samples to process
     */
    void processBlock(float* const* channelData, int numSamples)
    {
        if (channelData == nullptr || numSamples <= 0)
            return;

        float* leftChannel = channelData[0];
        float* rightChannel = channelData[1];

        if (leftChannel == nullptr || rightChannel == nullptr)
            return;

        const bool isSmoothing = smoothedFrequency.isSmoothing() ||
                                  smoothedGainDB.isSmoothing() ||
                                  smoothedQ.isSmoothing();

        if (isSmoothing)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float freq = smoothedFrequency.getNextValue();
                const float gain = smoothedGainDB.getNextValue();
                const float q = smoothedQ.getNextValue();

                if (std::abs(freq - lastFrequency) > 0.01f ||
                    std::abs(gain - lastGainDB) > 0.001f ||
                    std::abs(q - lastQ) > 0.0001f)
                {
                    lastFrequency = freq;
                    lastGainDB = gain;
                    lastQ = q;
                    
                    auto coeffs = Qcalc::calculate(currentSampleRate,
                                                   static_cast<double>(freq),
                                                   static_cast<double>(gain),
                                                   static_cast<double>(q),
                                                   qMode, filterType);
                    biquad.setCoeffs(coeffs);
                }

                biquad.processStereo(&leftChannel[i], &rightChannel[i],
                                     &leftChannel[i], &rightChannel[i]);
            }
        }
        else
        {
            biquad.processBlock(channelData, numSamples);
        }
    }

    /**
     * Reset the filter state (clear delay lines).
     * Call this when playback stops or when there's a discontinuity.
     */
    void reset()
    {
        biquad.reset();
    }

    /**
     * Check if parameters are currently smoothing.
     */
    bool isSmoothing() const
    {
        return smoothedFrequency.isSmoothing() ||
               smoothedGainDB.isSmoothing() ||
               smoothedQ.isSmoothing();
    }

    /**
     * Get the current (smoothed) frequency value.
     */
    float getCurrentFrequency() const { return smoothedFrequency.getCurrentValue(); }
    
    /**
     * Get the current (smoothed) gain value in dB.
     */
    float getCurrentGainDB() const { return smoothedGainDB.getCurrentValue(); }
    
    /**
     * Get the current (smoothed) Q value.
     */
    float getCurrentQ() const { return smoothedQ.getCurrentValue(); }

private:
    void updateCoefficients()
    {
        lastFrequency = smoothedFrequency.getCurrentValue();
        lastGainDB = smoothedGainDB.getCurrentValue();
        lastQ = smoothedQ.getCurrentValue();

        auto coeffs = Qcalc::calculate(currentSampleRate,
                                       static_cast<double>(lastFrequency),
                                       static_cast<double>(lastGainDB),
                                       static_cast<double>(lastQ),
                                       qMode, filterType);
        biquad.setCoeffs(coeffs);
    }

    // The underlying SIMD biquad filter
    Biquad biquad;

    // Smoothed parameter values
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFrequency;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGainDB;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedQ;

    // Last applied parameter values (to detect significant changes)
    float lastFrequency = 1000.0f;
    float lastGainDB = 0.0f;
    float lastQ = 0.707f;

    // Filter configuration
    FilterType filterType = FilterType::Peaking;
    QMode qMode = QMode::Constant_Q;

    // Audio settings
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
};
