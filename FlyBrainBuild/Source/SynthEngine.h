#pragma once
#include <JuceHeader.h>
#include <array>

class SynthEngine
{
public:
    void prepare(double sampleRate);
    void reset();
    void trigger(int nodeId, float intensity, int rootMidi, int scaleIndex, float attackMs, float releaseMs);
    void render(float& left, float& right, float cutoffHz, float drive);

private:
    struct Voice
    {
        bool active = false;
        double phase = 0.0;
        double phaseInc = 0.0;
        float env = 0.0f;
        float attackCoeff = 0.01f;
        float releaseCoeff = 0.999f;
        float pan = 0.0f;
        float brightness = 0.5f;
        int age = 0;
    };

    static constexpr int voiceCount = 32;
    std::array<Voice, voiceCount> voices{};
    double sr = 48000.0;
    float lpL = 0.0f, lpR = 0.0f;

    int midiForNode(int nodeId, int rootMidi, int scaleIndex) const;
};
