#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class BrainView final : public juce::Component, private juce::Timer
{
public:
    explicit BrainView(BrainEngine& engine);
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    BrainEngine& brain;
};

class FlyBrainSynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit FlyBrainSynthAudioProcessorEditor(FlyBrainSynthAudioProcessor&);
    ~FlyBrainSynthAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    FlyBrainSynthAudioProcessor& processor;
    BrainView brainView;
    std::array<Knob, 10> knobs;
    juce::ComboBox scaleBox;
    juce::Label scaleLabel;
    std::unique_ptr<ComboAttachment> scaleAttachment;
    juce::TextButton pokeButton { "POKE THE FLY" };
    juce::TextButton resetButton { "RESET" };
    juce::Label statusLabel;

    void setupKnob(int index, const char* parameterId, const char* title, const char* suffix = "");
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlyBrainSynthAudioProcessorEditor)
};
