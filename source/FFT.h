#pragma once

#ifndef BIQUAD3_FFT_H
#define BIQUAD3_FFT_H

#include <JuceHeader.h>
#include "SPSC.h"
#include <atomic>

// Forward declaration
class PluginProcessor;

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        window->multiplyWithWindowingTable (fftData.data(), fftSize);
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());

        int numBins = (int)fftSize / 2;

        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[i];
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }

        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }

    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+10),   top);
        };

        auto y = map(renderData[0]);

        if( std::isnan(y) || std::isinf(y) )
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 2;

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<juce::AudioBuffer<float>>& scsf) :
    channelFifo(&scsf)
    {
        fftDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, fftDataGenerator.getFFTSize());
    }

    void process(juce::Rectangle<float> fftBounds, double sampleRate)
    {
        juce::AudioBuffer<float> tempIncomingBuffer;
        while( channelFifo->getNumCompleteBuffersAvailable() > 0 )
        {
            if( channelFifo->getAudioBuffer(tempIncomingBuffer) )
            {
                auto size = tempIncomingBuffer.getNumSamples();

                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                                  monoBuffer.getReadPointer(0, size),
                                                  monoBuffer.getNumSamples() - size);

                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                                  tempIncomingBuffer.getReadPointer(0, 0),
                                                  size);

                fftDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
            }
        }

        const auto fftSize = fftDataGenerator.getFFTSize();
        const auto binWidth = sampleRate / double(fftSize);

        while( fftDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
        {
            std::vector<float> fftData;
            if( fftDataGenerator.getFFTData(fftData) )
            {
                pathGenerator.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
            }
        }

        while( pathGenerator.getNumPathsAvailable() > 0 )
        {
            pathGenerator.getPath(channelFFTPath);
        }
    }

    juce::Path getPath() { return channelFFTPath; }

private:
    SingleChannelSampleFifo<juce::AudioBuffer<float>>* channelFifo;

    juce::AudioBuffer<float> monoBuffer;

    FFTDataGenerator<std::vector<float>> fftDataGenerator;

    AnalyzerPathGenerator<juce::Path> pathGenerator;

    juce::Path channelFFTPath;
};

//==============================================================================
// FFT Spectrum Component - displays the FFT analysis
struct FFTSpectrumComponent : juce::Component,
                             juce::Timer,
                             juce::AudioProcessorValueTreeState::Listener
{
    FFTSpectrumComponent(PluginProcessor& p);
    ~FFTSpectrumComponent() override;

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

private:
    PluginProcessor& processorRef;

    PathProducer leftPathProducer, rightPathProducer;

    void drawBackgroundGrid(juce::Graphics& g);
    void drawTextLabels(juce::Graphics& g);
    void drawResponseCurve(juce::Graphics& g);

    enum class DragHandle
    {
        none,
        lowShelf,
        midPeak,
        highShelf
    };

    void drawDragHandles(juce::Graphics& g);
    juce::Point<float> getHandlePosition(DragHandle h);
    DragHandle getHandleAtPosition(juce::Point<float> pos);

    void beginGestureForHandle(DragHandle h);
    void endGestureForHandle(DragHandle h);
    void setHandleFromPosition(DragHandle h, juce::Point<float> pos);

    static void setParameterValue(juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& paramID,
                                  float newValue);
    static void beginGesture(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);
    static void endGesture  (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

    DragHandle activeHandle { DragHandle::none };

    std::atomic<float> hsFreqHz { 1000.0f };
    std::atomic<float> hsGainDb { 0.0f };
    std::atomic<float> mpFreqHz { 1000.0f };
    std::atomic<float> mpGainDb { 0.0f };
    std::atomic<float> lsFreqHz { 1000.0f };
    std::atomic<float> lsGainDb { 0.0f };

    std::atomic<bool> hoverAnyHandle { false };

    std::vector<float> getFrequencies();
    std::vector<float> getGains();
    std::vector<float> getXs(const std::vector<float>& freqs, float left, float width);

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();
};

#endif
