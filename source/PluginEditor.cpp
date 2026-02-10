#include "PluginEditor.h"
#include "BinaryData.h"
#include <sst/jucegui/style/StyleSheet.h>
//==============================================================================
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      fftComponent (p),
      lowShelfFreqKnob ("Low Shelf", p.getTreeState(), lowShelfID),
      lowShelfGainKnob ("LS Gain", p.getTreeState(), lowShelfGainID, true),
      midPeakFreqKnob ("Peak", p.getTreeState(), midPeakID),
      midPeakGainKnob ("Peak Gain", p.getTreeState(), midPeakGainID, true),
      highShelfFreqKnob ("High Shelf", p.getTreeState(), highShelfID),
      highShelfGainKnob ("HS Gain", p.getTreeState(), highShelfGainID, true),
      levelMeter (p.measurementL, p.measurementR)
{
    setLookAndFeel(&mainLF);

    sst::jucegui::style::StyleSheet::initializeStyleSheets([] {});
    auto sstStyle = sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(
        sst::jucegui::style::StyleSheet::BuiltInTypes::LIGHT);

    addAndMakeVisible(fftComponent);

    inspectButton.setButtonText("i");
    inspectButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    inspectButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    inspectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    inspectButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    inspectButton.setWantsKeyboardFocus(false);
    inspectButton.setTooltip("Open UI Inspector");
    addAndMakeVisible(inspectButton);

    menuGroup.setText("Model / Config");
    menuGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    addAndMakeVisible(menuGroup);

    shapingGroup.setText("Shape");
    shapingGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    addAndMakeVisible(shapingGroup);

    auto initMenuButton = [&](std::unique_ptr<sst::jucegui::components::MenuButton>& btn,
                              std::string label,
                              std::vector<std::string> items)
    {
        btn = std::make_unique<sst::jucegui::components::MenuButton>();
        btn->setStyle(sstStyle);
        btn->setLabel(label);

        btn->setOnCallback([b = btn.get(), items = std::move(items)] {
            juce::PopupMenu m;
            int idx = 1;
            for (const auto& it : items)
                m.addItem(idx++, it);

            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(*b));
        });
    };

    initMenuButton(modelMenu, "Model", { "Default" });
    initMenuButton(configMenu, "Config", { "Default" });
    initMenuButton(pbMenu, "PB", { "Off", "On" });
    initMenuButton(slpMenu, "SLP", { "Soft", "Hard" });
    initMenuButton(drvMenu, "DRV", { "Clean", "Drive" });
    initMenuButton(fsmMenu, "FSM", { "A", "B" });

    menuGroup.addAndMakeVisible(*modelMenu);
    menuGroup.addAndMakeVisible(*configMenu);

    shapingGroup.addAndMakeVisible(*pbMenu);
    shapingGroup.addAndMakeVisible(*slpMenu);
    shapingGroup.addAndMakeVisible(*drvMenu);
    shapingGroup.addAndMakeVisible(*fsmMenu);

    lowShelfGroup.setText("Low Shelf");
    lowShelfGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    lowShelfGroup.addAndMakeVisible(lowShelfFreqKnob);
    lowShelfGroup.addAndMakeVisible(lowShelfGainKnob);
    addAndMakeVisible(lowShelfGroup);

    peakGroup.setText("Peak");
    peakGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    peakGroup.addAndMakeVisible(midPeakFreqKnob);
    peakGroup.addAndMakeVisible(midPeakGainKnob);
    addAndMakeVisible(peakGroup);

    highShelfGroup.setText("High Shelf");
    highShelfGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    highShelfGroup.addAndMakeVisible(highShelfFreqKnob);
    highShelfGroup.addAndMakeVisible(highShelfGainKnob);
    addAndMakeVisible(highShelfGroup);

    outputGroup.setText("Output");
    outputGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    outputGroup.addAndMakeVisible(levelMeter);
    addAndMakeVisible(outputGroup);

    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    const auto black = juce::Colours::black;
    menuGroup.setColour(juce::GroupComponent::textColourId, black);
    shapingGroup.setColour(juce::GroupComponent::textColourId, black);
    lowShelfGroup.setColour(juce::GroupComponent::textColourId, black);
    peakGroup.setColour(juce::GroupComponent::textColourId, black);
    highShelfGroup.setColour(juce::GroupComponent::textColourId, black);
    outputGroup.setColour(juce::GroupComponent::textColourId, black);

    setSize (500, 700);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel(nullptr);
}
//==============================================================================
void PluginEditor::paint (juce::Graphics& g)
{
    auto noise = juce::ImageCache::getFromMemory(BinaryData::Noise_png, BinaryData::Noise_pngSize);
    auto fillType = juce::FillType(noise, juce::AffineTransform::scale(0.5f));
    g.setFillType(fillType);
    g.fillRect(getLocalBounds());
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds();
    {
        constexpr int s = 18;
        constexpr int m = 8;
        inspectButton.setBounds(bounds.getRight() - s - m, bounds.getY() + m, s, s);
    }

    // FFT on top
    auto fftArea = bounds.removeFromTop(bounds.getHeight() * 0.50f);
    fftComponent.setBounds(fftArea);

    // Controls area below
    bounds.removeFromTop(10);
    auto controlsArea = bounds.reduced(10, 10);

    // Menu groups row
    auto menuRow = controlsArea.removeFromTop(70);
    auto leftMenu = menuRow.removeFromLeft(menuRow.getWidth() / 2 - 5);
    menuGroup.setBounds(leftMenu);
    shapingGroup.setBounds(menuRow);

    if (modelMenu && configMenu)
    {
        auto r = menuGroup.getLocalBounds().reduced(10, 26);
        auto w = (r.getWidth() - 8) / 2;
        auto h = 18;
        modelMenu->setBounds(r.getX(), r.getY(), w, h);
        configMenu->setBounds(r.getX() + w + 8, r.getY(), w, h);
    }

    if (pbMenu && slpMenu && drvMenu && fsmMenu)
    {
        constexpr int buttonH = 18;
        constexpr int colGap = 8;
        constexpr int rowGap = 6;

        auto r = shapingGroup.getLocalBounds().reduced(10, 0);
        r.removeFromTop((18));
        r.removeFromBottom(6);

        auto w = (r.getWidth() - colGap) / 2;
        pbMenu->setBounds(r.getX(), r.getY(), w, buttonH);
        slpMenu->setBounds(r.getX() + w + colGap, r.getY(), w, buttonH);
        drvMenu->setBounds(r.getX(), r.getY() + buttonH + rowGap, w, buttonH);
        fsmMenu->setBounds(r.getX() + w + colGap, r.getY() + buttonH + rowGap, w, buttonH);
    }
    auto y = controlsArea.getY();
    auto height = controlsArea.getHeight();

    lowShelfGroup.setBounds(controlsArea.getX(), y, 120, height);
    peakGroup.setBounds(lowShelfGroup.getRight() + 10, y, 110, height);
    outputGroup.setBounds(controlsArea.getRight() - (80), y, 70, height);
    highShelfGroup.setBounds(peakGroup.getRight() + 10, y, outputGroup.getX() - peakGroup.getRight() - 20, height);

    lowShelfFreqKnob.setTopLeftPosition(20, 20);
    lowShelfGainKnob.setTopLeftPosition(lowShelfFreqKnob.getX(), lowShelfFreqKnob.getBottom() + 10);

    midPeakFreqKnob.setTopLeftPosition((20), (20));
    midPeakGainKnob.setTopLeftPosition(midPeakFreqKnob.getX(), midPeakFreqKnob.getBottom() + 10);

    highShelfFreqKnob.setTopLeftPosition(20, 20);
    highShelfGainKnob.setTopLeftPosition(highShelfFreqKnob.getX(), highShelfFreqKnob.getBottom() + 10);

    levelMeter.setBounds(outputGroup.getWidth() - 45, 30, 30, height - 40);
}
