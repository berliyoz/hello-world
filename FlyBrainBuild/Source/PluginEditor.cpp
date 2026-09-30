#include "PluginEditor.h"
#include <cmath>

BrainView::BrainView(BrainEngine& e) : brain(e)
{
    startTimerHz(30);
}

void BrainView::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(8.0f);
    g.setColour(juce::Colour(0xff10131b));
    g.fillRoundedRectangle(area, 18.0f);

    const auto& nodes = brain.getVisualNodes();
    const auto& edges = brain.getVisualEdges();
    if (nodes.empty()) return;

    auto pointFor = [&](int i)
    {
        const auto& n = nodes[static_cast<std::size_t>(i)];
        return juce::Point<float>(area.getX() + n.x * area.getWidth(), area.getY() + n.y * area.getHeight());
    };

    g.setColour(juce::Colour(0x1fffffff));
    for (std::size_t i = 0; i < edges.size(); i += 2)
    {
        const auto& e = edges[i];
        if (e.source < static_cast<int>(nodes.size()) && e.target < static_cast<int>(nodes.size()))
            g.drawLine({ pointFor(e.source), pointFor(e.target) }, 0.65f);
    }

    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
    {
        const float a = brain.getActivity(i);
        const auto p = pointFor(i);
        const float radius = 1.25f + a * 5.8f;
        auto base = juce::Colour::fromHSV(std::fmod(0.56f + nodes[static_cast<std::size_t>(i)].transmitter * 0.075f, 1.0f), 0.55f, 0.82f, 1.0f);
        if (a > 0.05f) base = base.brighter(0.7f + a);
        g.setColour(base.withAlpha(0.4f + 0.6f * a));
        g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);
    }

    g.setColour(juce::Colour(0xaaffffff));
    g.setFont(13.0f);
    g.drawText(juce::String(brain.getNodeCount()) + " nodes - neural activity", area.toNearestInt().reduced(12), juce::Justification::topLeft);
}

FlyBrainSynthAudioProcessorEditor::FlyBrainSynthAudioProcessorEditor(FlyBrainSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), brainView(p.getBrain())
{
    setSize(980, 650);
    setResizable(true, true);
    setResizeLimits(780, 520, 1500, 1000);

    addAndMakeVisible(brainView);
    setupKnob(0, "threshold", "THRESHOLD");
    setupKnob(1, "decay", "BRAIN DECAY", " ms");
    setupKnob(2, "timescale", "TIME SCALE", "x");
    setupKnob(3, "speed", "PROPAGATION", "x");
    setupKnob(4, "coupling", "COUPLING");
    setupKnob(5, "autorate", "AUTO POKE", " Hz");
    setupKnob(6, "attack", "ATTACK", " ms");
    setupKnob(7, "release", "RELEASE", " ms");
    setupKnob(8, "cutoff", "FILTER", " Hz");
    setupKnob(9, "drive", "DRIVE");

    scaleBox.addItemList({ "Chromatic", "Minor Pent", "Dorian", "Whole Tone" }, 1);
    scaleLabel.setText("SCALE", juce::dontSendNotification);
    scaleLabel.setJustificationType(juce::Justification::centred);
    scaleLabel.setFont(11.0f);
    addAndMakeVisible(scaleBox);
    addAndMakeVisible(scaleLabel);
    scaleAttachment = std::make_unique<ComboAttachment>(processor.parameters, "scale", scaleBox);

    pokeButton.onClick = [this] { processor.requestPoke(); };
    resetButton.onClick = [this] { processor.requestReset(); };
    addAndMakeVisible(pokeButton);
    addAndMakeVisible(resetButton);

    statusLabel.setText("DEMO NETWORK - ready for FlyWire subnetwork import", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaeb6c7));
    statusLabel.setFont(12.0f);
    addAndMakeVisible(statusLabel);
}

void FlyBrainSynthAudioProcessorEditor::setupKnob(int index, const char* parameterId, const char* title, const char* suffix)
{
    auto& k = knobs[static_cast<std::size_t>(index)];
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    k.slider.setTextValueSuffix(suffix);
    k.label.setText(title, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(10.5f);
    addAndMakeVisible(k.slider);
    addAndMakeVisible(k.label);
    k.attachment = std::make_unique<SliderAttachment>(processor.parameters, parameterId, k.slider);
}

void FlyBrainSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient bg(juce::Colour(0xff11141b), 0, 0, juce::Colour(0xff242231), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false);
    g.setGradientFill(bg);
    g.fillAll();

    g.setColour(juce::Colour(0xfff1f3f8));
    g.setFont(juce::FontOptions(30.0f, juce::Font::bold));
    g.drawText("FLYBRAIN SYNTH", 28, 18, getWidth() - 56, 40, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffefb4ff));
    g.setFont(13.0f);
    g.drawText("connectome-driven neural instrument - v0.1", 30, 55, 430, 20, juce::Justification::centredLeft);
}

void FlyBrainSynthAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(24);
    r.removeFromTop(66);
    auto upper = r.removeFromTop(static_cast<int>(r.getHeight() * 0.57f));
    brainView.setBounds(upper);
    r.removeFromTop(10);

    auto controls = r.removeFromTop(150);
    const int knobWidth = juce::jmax(68, controls.getWidth() / 11);
    for (int i = 0; i < 10; ++i)
    {
        auto cell = controls.removeFromLeft(knobWidth);
        knobs[static_cast<std::size_t>(i)].label.setBounds(cell.removeFromTop(18));
        knobs[static_cast<std::size_t>(i)].slider.setBounds(cell.reduced(2));
    }

    auto scaleCell = controls;
    scaleLabel.setBounds(scaleCell.removeFromTop(18));
    scaleBox.setBounds(scaleCell.reduced(6, 36));

    auto bottom = r.removeFromTop(44);
    pokeButton.setBounds(bottom.removeFromLeft(180).reduced(4));
    resetButton.setBounds(bottom.removeFromLeft(100).reduced(4));
    statusLabel.setBounds(bottom.reduced(8, 4));
}
