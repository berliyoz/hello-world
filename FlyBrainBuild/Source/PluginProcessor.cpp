#include "PluginProcessor.h"
#include "PluginEditor.h"

FlyBrainSynthAudioProcessor::FlyBrainSynthAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    fired.reserve(2048);
}

juce::AudioProcessorValueTreeState::ParameterLayout FlyBrainSynthAudioProcessor::createParameterLayout()
{
    using P = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using I = juce::AudioParameterInt;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<P>("threshold", "Threshold", juce::NormalisableRange<float>(0.1f, 2.0f, 0.001f, 0.45f), 0.65f));
    layout.add(std::make_unique<P>("decay", "Brain Decay", juce::NormalisableRange<float>(10.0f, 1200.0f, 0.1f, 0.35f), 180.0f));
    layout.add(std::make_unique<P>("timescale", "Time Scale", juce::NormalisableRange<float>(0.01f, 100.0f, 0.0001f, 0.22f), 1.0f));
    layout.add(std::make_unique<P>("speed", "Propagation", juce::NormalisableRange<float>(0.1f, 8.0f, 0.001f, 0.4f), 1.0f));
    layout.add(std::make_unique<P>("coupling", "Coupling", juce::NormalisableRange<float>(0.05f, 2.5f, 0.001f, 0.5f), 0.82f));
    layout.add(std::make_unique<P>("autorate", "Auto Poke", juce::NormalisableRange<float>(0.0f, 12.0f, 0.001f, 0.5f), 1.2f));
    layout.add(std::make_unique<P>("attack", "Attack", juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f, 0.35f), 12.0f));
    layout.add(std::make_unique<P>("release", "Release", juce::NormalisableRange<float>(20.0f, 5000.0f, 0.1f, 0.35f), 500.0f));
    layout.add(std::make_unique<P>("cutoff", "Filter", juce::NormalisableRange<float>(80.0f, 18000.0f, 0.1f, 0.28f), 6500.0f));
    layout.add(std::make_unique<P>("drive", "Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.18f));
    layout.add(std::make_unique<I>("root", "Root", 24, 72, 43));
    layout.add(std::make_unique<C>("scale", "Scale", juce::StringArray{ "Chromatic", "Minor Pent", "Dorian", "Whole Tone" }, 2));
    layout.add(std::make_unique<P>("output", "Output", juce::NormalisableRange<float>(-30.0f, 6.0f, 0.01f), -8.0f));
    return layout;
}

void FlyBrainSynthAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    synth.prepare(sampleRate);
    brain.reset();
    controlIntervalSamples = juce::jmax(1, static_cast<int>(sampleRate / 1000.0));
    controlCountdown = 0;
    autoCountdownMs = 0.0f;
}

bool FlyBrainSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FlyBrainSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (resetRequested.exchange(false))
    {
        brain.reset();
        synth.reset();
    }

    const auto threshold = parameters.getRawParameterValue("threshold")->load();
    const auto decay = parameters.getRawParameterValue("decay")->load();
    const auto timescale = parameters.getRawParameterValue("timescale")->load();
    const auto speed = parameters.getRawParameterValue("speed")->load();
    const auto coupling = parameters.getRawParameterValue("coupling")->load();
    const auto autoRate = parameters.getRawParameterValue("autorate")->load();
    const auto attack = parameters.getRawParameterValue("attack")->load();
    const auto release = parameters.getRawParameterValue("release")->load();
    const auto cutoff = parameters.getRawParameterValue("cutoff")->load();
    const auto drive = parameters.getRawParameterValue("drive")->load();
    const int root = static_cast<int>(parameters.getRawParameterValue("root")->load());
    const int scale = static_cast<int>(parameters.getRawParameterValue("scale")->load());
    const float outGain = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load());

    BrainEngine::Parameters bp { threshold, decay, timescale, speed, coupling };

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            const int n = brain.getNodeCount() > 0 ? (msg.getNoteNumber() * 17 + msg.getNoteNumber() / 3) % brain.getNodeCount() : 0;
            brain.stimulate(n, 0.85f + msg.getFloatVelocity() * 1.35f);
        }
    }

    if (pokeRequested.exchange(false) && brain.getNodeCount() > 0)
        brain.stimulate(random.nextInt(brain.getNodeCount()), 1.75f);

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int s = 0; s < buffer.getNumSamples(); ++s)
    {
        if (--controlCountdown <= 0)
        {
            controlCountdown = controlIntervalSamples;
            const float dtMs = 1000.0f * static_cast<float>(controlIntervalSamples / sr);

            if (autoRate > 0.0001f)
            {
                autoCountdownMs -= dtMs;
                if (autoCountdownMs <= 0.0f && brain.getNodeCount() > 0)
                {
                    brain.stimulate(random.nextInt(brain.getNodeCount()), 1.15f + random.nextFloat() * 0.95f);
                    const float jitter = 0.65f + random.nextFloat() * 0.8f;
                    autoCountdownMs = 1000.0f / autoRate * jitter;
                }
            }

            brain.step(dtMs, bp, fired);
            for (const auto node : fired)
            {
                const float a = brain.getActivity(node);
                synth.trigger(node, a, root, scale, attack, release);
            }
        }

        float l = 0.0f, r = 0.0f;
        synth.render(l, r, cutoff, drive);
        left[s] = l * outGain;
        right[s] = r * outGain;
    }
}

juce::AudioProcessorEditor* FlyBrainSynthAudioProcessor::createEditor()
{
    return new FlyBrainSynthAudioProcessorEditor(*this);
}

void FlyBrainSynthAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void FlyBrainSynthAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlyBrainSynthAudioProcessor();
}
