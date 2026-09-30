#include "SynthEngine.h"
#include <cmath>
#include <algorithm>

void SynthEngine::prepare(double sampleRate)
{
    sr = sampleRate > 1.0 ? sampleRate : 48000.0;
    reset();
}

void SynthEngine::reset()
{
    for (auto& v : voices) v = {};
    lpL = lpR = 0.0f;
}

int SynthEngine::midiForNode(int nodeId, int rootMidi, int scaleIndex) const
{
    static constexpr int chromatic[] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
    static constexpr int minorPent[] = { 0,3,5,7,10 };
    static constexpr int dorian[] = { 0,2,3,5,7,9,10 };
    static constexpr int whole[] = { 0,2,4,6,8,10 };

    const int* scale = chromatic; int count = 12;
    if (scaleIndex == 1) { scale = minorPent; count = 5; }
    if (scaleIndex == 2) { scale = dorian; count = 7; }
    if (scaleIndex == 3) { scale = whole; count = 6; }

    const int degree = nodeId % count;
    const int octave = (nodeId / count) % 5;
    return juce::jlimit(12, 108, rootMidi + scale[degree] + octave * 12);
}

void SynthEngine::trigger(int nodeId, float intensity, int rootMidi, int scaleIndex, float attackMs, float releaseMs)
{
    Voice* chosen = nullptr;
    for (auto& v : voices)
        if (!v.active) { chosen = &v; break; }
    if (chosen == nullptr)
        chosen = &*std::max_element(voices.begin(), voices.end(), [](const Voice& a, const Voice& b) { return a.age < b.age; });

    const int midi = midiForNode(nodeId, rootMidi, scaleIndex);
    const double hz = 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
    chosen->active = true;
    chosen->phaseInc = juce::MathConstants<double>::twoPi * hz / sr;
    chosen->env = juce::jmax(chosen->env, 0.001f);
    chosen->attackCoeff = 1.0f - std::exp(-1.0f / static_cast<float>(sr * juce::jmax(0.001f, attackMs * 0.001f)));
    chosen->releaseCoeff = std::exp(-1.0f / static_cast<float>(sr * juce::jmax(0.005f, releaseMs * 0.001f)));
    chosen->pan = std::sin(static_cast<float>(nodeId) * 1.6180339f) * 0.85f;
    chosen->brightness = juce::jlimit(0.05f, 0.95f, 0.2f + intensity * 0.65f);
    chosen->age = 0;
}

void SynthEngine::render(float& left, float& right, float cutoffHz, float drive)
{
    float l = 0.0f, r = 0.0f;
    for (auto& v : voices)
    {
        if (!v.active) continue;
        v.env += (1.0f - v.env) * v.attackCoeff;
        const double p = v.phase;
        const float sine = static_cast<float>(std::sin(p));
        const float second = static_cast<float>(std::sin(p * 2.003 + 0.7)) * v.brightness * 0.30f;
        const float third = static_cast<float>(std::sin(p * 3.007 + 1.2)) * v.brightness * 0.12f;
        const float sample = (sine + second + third) * v.env * 0.065f;
        const float panL = std::sqrt(0.5f * (1.0f - v.pan));
        const float panR = std::sqrt(0.5f * (1.0f + v.pan));
        l += sample * panL;
        r += sample * panR;
        v.phase += v.phaseInc;
        if (v.phase > juce::MathConstants<double>::twoPi) v.phase -= juce::MathConstants<double>::twoPi;
        v.env *= v.releaseCoeff;
        ++v.age;
        if (v.env < 0.00008f) v.active = false;
    }

    const float g = 1.0f + drive * 8.0f;
    l = std::tanh(l * g) / juce::jmax(1.0f, g * 0.55f);
    r = std::tanh(r * g) / juce::jmax(1.0f, g * 0.55f);

    const float fc = juce::jlimit(30.0f, static_cast<float>(sr * 0.45), cutoffHz);
    const float a = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * fc / static_cast<float>(sr));
    lpL += a * (l - lpL);
    lpR += a * (r - lpR);
    left = lpL;
    right = lpR;
}
