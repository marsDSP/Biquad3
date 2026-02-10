#include "ResponseCurve.h"
#include "PluginProcessor.h"
#include "DSP/Qcalc.h"
#include "Utils/Parameters.h"
#include <cmath>
#include <limits>

//==============================================================================
FFTSpectrumComponent::FFTSpectrumComponent(PluginProcessor& p) :
processorRef(p),
leftPathProducer(processorRef.leftChannelFifo),
rightPathProducer(processorRef.rightChannelFifo)
{
    auto& apvts = processorRef.getTreeState();

    // Initialize cached
    hsFreqHz.store(apvts.getRawParameterValue(highShelfID.getParamID())->load());
    hsGainDb.store(apvts.getRawParameterValue(highShelfGainID.getParamID())->load());
    mpFreqHz.store(apvts.getRawParameterValue(midPeakID.getParamID())->load());
    mpGainDb.store(apvts.getRawParameterValue(midPeakGainID.getParamID())->load());
    lsFreqHz.store(apvts.getRawParameterValue(lowShelfID.getParamID())->load());
    lsGainDb.store(apvts.getRawParameterValue(lowShelfGainID.getParamID())->load());

    // Listen
    apvts.addParameterListener(highShelfID.getParamID(), this);
    apvts.addParameterListener(highShelfGainID.getParamID(), this);
    apvts.addParameterListener(midPeakID.getParamID(), this);
    apvts.addParameterListener(midPeakGainID.getParamID(), this);
    apvts.addParameterListener(lowShelfID.getParamID(), this);
    apvts.addParameterListener(lowShelfGainID.getParamID(), this);

    startTimerHz(60);
}

FFTSpectrumComponent::~FFTSpectrumComponent()
{
    auto& apvts = processorRef.getTreeState();
    apvts.removeParameterListener(highShelfID.getParamID(), this);
    apvts.removeParameterListener(highShelfGainID.getParamID(), this);
    apvts.removeParameterListener(midPeakID.getParamID(), this);
    apvts.removeParameterListener(midPeakGainID.getParamID(), this);
    apvts.removeParameterListener(lowShelfID.getParamID(), this);
    apvts.removeParameterListener(lowShelfGainID.getParamID(), this);
}

void FFTSpectrumComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    g.fillAll(Colours::black);

    drawBackgroundGrid(g);

    auto responseArea = getAnalysisArea();

    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

    g.setColour(Colour(97u, 18u, 167u));
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

    g.setColour(Colour(215u, 201u, 134u));
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));

    // Draw the response curve
    drawResponseCurve(g);

    // Draw draggable points on top of the response curve
    drawDragHandles(g);

    Path border;
    border.setUsingNonZeroWinding(false);
    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());

    g.setColour(Colours::black);
    g.fillPath(border);

    drawTextLabels(g);

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
}

void FFTSpectrumComponent::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Called from the audio thread by APVTS. Only update atomics here.
    if (parameterID == highShelfID.getParamID())
        hsFreqHz.store(newValue);
    else if (parameterID == highShelfGainID.getParamID())
        hsGainDb.store(newValue);
    else if (parameterID == midPeakID.getParamID())
        mpFreqHz.store(newValue);
    else if (parameterID == midPeakGainID.getParamID())
        mpGainDb.store(newValue);
    else if (parameterID == lowShelfID.getParamID())
        lsFreqHz.store(newValue);
    else if (parameterID == lowShelfGainID.getParamID())
        lsGainDb.store(newValue);
}

void FFTSpectrumComponent::mouseMove(const juce::MouseEvent& e)
{
    const auto h = getHandleAtPosition(e.position);
    const bool hovering = (h != DragHandle::none);

    if (hoverAnyHandle.exchange(hovering) != hovering)
        setMouseCursor(hovering ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void FFTSpectrumComponent::mouseDown(const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;

    const auto h = getHandleAtPosition(e.position);
    if (h == DragHandle::none)
        return;

    activeHandle = h;
    beginGestureForHandle(activeHandle);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setHandleFromPosition(activeHandle, e.position);
}

void FFTSpectrumComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (activeHandle == DragHandle::none)
        return;

    setHandleFromPosition(activeHandle, e.position);
}

void FFTSpectrumComponent::mouseUp(const juce::MouseEvent& e)
{
    if (activeHandle == DragHandle::none)
        return;

    endGestureForHandle(activeHandle);
    activeHandle = DragHandle::none;

    // Refresh hover state and cursor
    const auto h = getHandleAtPosition(e.position);
    const bool hovering = (h != DragHandle::none);
    hoverAnyHandle.store(hovering);
    setMouseCursor(hovering ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void FFTSpectrumComponent::beginGesture(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    if (auto* p = apvts.getParameter(paramID))
        p->beginChangeGesture();
}

void FFTSpectrumComponent::endGesture(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    if (auto* p = apvts.getParameter(paramID))
        p->endChangeGesture();
}

void FFTSpectrumComponent::setParameterValue(juce::AudioProcessorValueTreeState& apvts,
                                             const juce::String& paramID,
                                             float newValue)
{
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramID)))
        p->setValueNotifyingHost(p->convertTo0to1(newValue));
}

void FFTSpectrumComponent::beginGestureForHandle(DragHandle h)
{
    auto& apvts = processorRef.getTreeState();

    switch (h)
    {
        case DragHandle::lowShelf:
            beginGesture(apvts, lowShelfID.getParamID());
            beginGesture(apvts, lowShelfGainID.getParamID());
            break;
        case DragHandle::midPeak:
            beginGesture(apvts, midPeakID.getParamID());
            beginGesture(apvts, midPeakGainID.getParamID());
            break;
        case DragHandle::highShelf:
            beginGesture(apvts, highShelfID.getParamID());
            beginGesture(apvts, highShelfGainID.getParamID());
            break;
        case DragHandle::none:
            break;
    }
}

void FFTSpectrumComponent::endGestureForHandle(DragHandle h)
{
    auto& apvts = processorRef.getTreeState();

    switch (h)
    {
        case DragHandle::lowShelf:
            endGesture(apvts, lowShelfID.getParamID());
            endGesture(apvts, lowShelfGainID.getParamID());
            break;
        case DragHandle::midPeak:
            endGesture(apvts, midPeakID.getParamID());
            endGesture(apvts, midPeakGainID.getParamID());
            break;
        case DragHandle::highShelf:
            endGesture(apvts, highShelfID.getParamID());
            endGesture(apvts, highShelfGainID.getParamID());
            break;
        case DragHandle::none:
            break;
    }
}

juce::Point<float> FFTSpectrumComponent::getHandlePosition(DragHandle h)
{
    const auto area = getAnalysisArea().toFloat();

    float freqHz = 1000.0f;
    float gainDb = 0.0f;

    switch (h)
    {
        case DragHandle::lowShelf:
            freqHz = lsFreqHz.load();
            gainDb = lsGainDb.load();
            break;
        case DragHandle::midPeak:
            freqHz = mpFreqHz.load();
            gainDb = mpGainDb.load();
            break;
        case DragHandle::highShelf:
            freqHz = hsFreqHz.load();
            gainDb = hsGainDb.load();
            break;
        case DragHandle::none:
            break;
    }

    freqHz = juce::jlimit(20.0f, 20000.0f, freqHz);
    gainDb = juce::jlimit(-24.0f, 24.0f, gainDb);

    const float x = area.getX() + area.getWidth() * juce::mapFromLog10(freqHz, 20.0f, 20000.0f);
    const float y = juce::jmap(gainDb, -24.0f, 24.0f, area.getBottom(), area.getY());

    return { x, juce::jlimit(area.getY(), area.getBottom(), y) };
}

FFTSpectrumComponent::DragHandle FFTSpectrumComponent::getHandleAtPosition(juce::Point<float> pos)
{
    const auto area = getAnalysisArea().toFloat();
    if (! area.contains(pos))
        return DragHandle::none;

    constexpr float hitRadius = 10.0f;
    constexpr float hitRadiusSq = hitRadius * hitRadius;

    struct Candidate
    {
        DragHandle h { DragHandle::none };
        float distSq { std::numeric_limits<float>::max() };
    };

    Candidate best;
    for (auto h : { DragHandle::lowShelf, DragHandle::midPeak, DragHandle::highShelf })
    {
        auto p = getHandlePosition(h);
        auto d = p - pos;
        const float dsq = d.getX() * d.getX() + d.getY() * d.getY();
        if (dsq < best.distSq)
            best = { h, dsq };
    }

    return (best.distSq <= hitRadiusSq) ? best.h : DragHandle::none;
}

void FFTSpectrumComponent::setHandleFromPosition(DragHandle h, juce::Point<float> pos)
{
    auto area = getAnalysisArea().toFloat();
    pos.x = juce::jlimit(area.getX(), area.getRight(), pos.x);
    pos.y = juce::jlimit(area.getY(), area.getBottom(), pos.y);

    const float normX = juce::jlimit(0.0f, 1.0f, (pos.x - area.getX()) / area.getWidth());
    const float freqHz = (float)juce::mapToLog10((double)normX, 20.0, 20000.0);

    const float gainDb = juce::jlimit(-24.0f, 24.0f,
                                      juce::jmap(pos.y, area.getBottom(), area.getY(), -24.0f, 24.0f));

    auto& apvts = processorRef.getTreeState();

    switch (h)
    {
        case DragHandle::lowShelf:
            setParameterValue(apvts, lowShelfID.getParamID(), freqHz);
            setParameterValue(apvts, lowShelfGainID.getParamID(), gainDb);
            lsFreqHz.store(freqHz);
            lsGainDb.store(gainDb);
            break;
        case DragHandle::midPeak:
            setParameterValue(apvts, midPeakID.getParamID(), freqHz);
            setParameterValue(apvts, midPeakGainID.getParamID(), gainDb);
            mpFreqHz.store(freqHz);
            mpGainDb.store(gainDb);
            break;
        case DragHandle::highShelf:
            setParameterValue(apvts, highShelfID.getParamID(), freqHz);
            setParameterValue(apvts, highShelfGainID.getParamID(), gainDb);
            hsFreqHz.store(freqHz);
            hsGainDb.store(gainDb);
            break;
        case DragHandle::none:
            break;
    }

    repaint();
}

void FFTSpectrumComponent::drawDragHandles(juce::Graphics& g)
{
    using namespace juce;

    const auto area = getAnalysisArea().toFloat();
    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    auto drawOne = [&](DragHandle h, Colour fill, Colour outline)
    {
        const auto p = getHandlePosition(h);
        const bool active = (activeHandle == h);
        const float r = active ? 6.0f : 5.0f;

        Rectangle<float> b(p.x - r, p.y - r, 2.0f * r, 2.0f * r);
        g.setColour(fill);
        g.fillEllipse(b);
        g.setColour(outline);
        g.drawEllipse(b, active ? 2.0f : 1.5f);
    };

    drawOne(DragHandle::lowShelf, Colours::white.withAlpha(0.9f), Colours::orange.withAlpha(0.9f));
    drawOne(DragHandle::midPeak, Colours::white.withAlpha(0.9f), Colours::orange.withAlpha(0.9f));
    drawOne(DragHandle::highShelf, Colours::white.withAlpha(0.9f), Colours::orange.withAlpha(0.9f));

    g.restoreState();
}

std::vector<float> FFTSpectrumComponent::getFrequencies()
{
    return std::vector<float>
    {
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000,
        20000
    };
}

std::vector<float> FFTSpectrumComponent::getGains()
{
    return std::vector<float>
    {
        -24, -12, 0, 12, 24
    };
}

std::vector<float> FFTSpectrumComponent::getXs(const std::vector<float>& freqs, float left, float width)
{
    std::vector<float> xs;
    for( auto f : freqs )
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back( left + width * normX );
    }
    return xs;
}

void FFTSpectrumComponent::drawBackgroundGrid(juce::Graphics& g)
{
    using namespace juce;
    auto freqs = getFrequencies();

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto xs = getXs(freqs, left, width);

    g.setColour(Colours::dimgrey);
    for( auto x : xs )
    {
        g.drawVerticalLine(x, top, bottom);
    }

    auto gain = getGains();

    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
}

void FFTSpectrumComponent::drawTextLabels(juce::Graphics& g)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for( int i = 0; i < (int)freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if( addK )
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    auto gain = getGains();

    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);

        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void FFTSpectrumComponent::drawResponseCurve(juce::Graphics& g)
{
    using namespace juce;

    auto responseArea = getAnalysisArea();
    auto w = responseArea.getWidth();

    auto& apvts = processorRef.getTreeState();
    auto sampleRate = processorRef.getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    // Read current parameter values
    const float hsFreq = apvts.getRawParameterValue(highShelfID.getParamID())->load();
    const float hsGain = apvts.getRawParameterValue(highShelfGainID.getParamID())->load();
    const float mpFreq = apvts.getRawParameterValue(midPeakID.getParamID())->load();
    const float mpGain = apvts.getRawParameterValue(midPeakGainID.getParamID())->load();
    const float lsFreq = apvts.getRawParameterValue(lowShelfID.getParamID())->load();
    const float lsGain = apvts.getRawParameterValue(lowShelfGainID.getParamID())->load();
    const float qModeVal = apvts.getRawParameterValue(qModeID.getParamID())->load();

    const QMode currentQMode = (qModeVal < 0.5f) ? QMode::Constant_Q : QMode::Proportional_Q;
    const double defaultQ = 0.707;

    // Compute biquad coefficients for each filter
    auto hsCoeffs = Qcalc::calculate(sampleRate, hsFreq, hsGain, defaultQ, currentQMode, FilterType::HighShelf);
    auto mpCoeffs = Qcalc::calculate(sampleRate, mpFreq, mpGain, defaultQ, currentQMode, FilterType::Peaking);
    auto lsCoeffs = Qcalc::calculate(sampleRate, lsFreq, lsGain, defaultQ, currentQMode, FilterType::LowShelf);

    // Helper: compute magnitude squared of biquad at angular frequency w0
    auto magSquared = [](const BiquadCoeffs& c, double w0) -> double
    {
        // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
        // at z = e^(jw): z^-1 = e^(-jw), z^-2 = e^(-2jw)
        double cosw = std::cos(w0);
        double cos2w = std::cos(2.0 * w0);

        double numSq = c.b0*c.b0 + c.b1*c.b1 + c.b2*c.b2
                      + 2.0*(c.b0*c.b1 + c.b1*c.b2)*cosw
                      + 2.0*c.b0*c.b2*cos2w;

        double denSq = 1.0 + c.a1*c.a1 + c.a2*c.a2
                      + 2.0*(c.a1 + c.a1*c.a2)*cosw
                      + 2.0*c.a2*cos2w;

        if (denSq < 1.0e-20)
            denSq = 1.0e-20;

        return numSq / denSq;
    };

    // Build the response curve path
    Path responseCurve;

    const double minFreq = 20.0;
    const double maxFreq = 20000.0;
    const float minDb = -24.f;
    const float maxDb = 24.f;

    auto top = (float)responseArea.getY();
    auto bottom = (float)responseArea.getBottom();
    auto left = (float)responseArea.getX();

    for (int i = 0; i < w; ++i)
    {
        // Map pixel to frequency (log scale)
        double normX = (double)i / (double)w;
        double freq = mapToLog10(normX, minFreq, maxFreq);
        double omega = 2.0 * MathConstants<double>::pi * freq / sampleRate;

        // Combined magnitude of all 3 filters
        double magSq = magSquared(hsCoeffs, omega)
                     * magSquared(mpCoeffs, omega)
                     * magSquared(lsCoeffs, omega);

        double magDb = Decibels::gainToDecibels(std::sqrt(std::max(magSq, 0.0)), (double)minDb);

        // Map dB to y position
        float y = jmap((float)magDb, minDb, maxDb, bottom, top);
        y = jlimit(top, bottom, y);

        if (i == 0)
            responseCurve.startNewSubPath(left + (float)i, y);
        else
            responseCurve.lineTo(left + (float)i, y);
    }

    // Draw the response curve
    g.setColour(Colours::white.withAlpha(0.9f));
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void FFTSpectrumComponent::resized()
{
}

void FFTSpectrumComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = processorRef.getSampleRate();

    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);

    repaint();
}

juce::Rectangle<int> FFTSpectrumComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> FFTSpectrumComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
