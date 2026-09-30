#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "BrainEngine.h"
#include "SynthEngine.h"

class FlyBrainSynthAudioProcessor final : public juce::AudioProcessor
{
public:
    FlyBrainSynthAudioProcessor();
    ~FlyBrainSynthAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;

    BrainEngine& getBrain() noexcept { return brain; }
    void requestPoke() noexcept { pokeRequested.store(true); }
    void requestReset() noexcept { resetRequested.store(true); }

private:
    BrainEngine brain;
    SynthEngine synth;
    std::vector<int> fired;
    juce::Random random;
    double sr = 48000.0;
    int controlIntervalSamples = 48;
    int controlCountdown = 0;
    float autoCountdownMs = 0.0f;
    std::atomic<bool> pokeRequested { false };
    std::atomic<bool> resetRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlyBrainSynthAudioProcessor)
};
