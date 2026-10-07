#pragma once

#include <array>
#include <juce_core/juce_core.h>

namespace Parameters
{
struct Definition
{
    const char* id;
    const char* label;
    float minimum;
    float maximum;
    float step;
    float defaultValue;
};

inline constexpr std::array<Definition, 8> operatorParameters {{
    { "ratio", "Ratio", 0.25f, 16.0f, 0.25f, 1.0f },
    { "fine", "Fine", -100.0f, 100.0f, 1.0f, 0.0f },
    { "level", "Level", 0.0f, 100.0f, 1.0f, 86.0f },
    { "feedback", "Feedback", 0.0f, 100.0f, 1.0f, 8.0f },
    { "attack", "Attack", 1.0f, 2000.0f, 1.0f, 8.0f },
    { "decay", "Decay", 10.0f, 4000.0f, 1.0f, 840.0f },
    { "sustain", "Sustain", 0.0f, 100.0f, 1.0f, 62.0f },
    { "release", "Release", 10.0f, 6000.0f, 1.0f, 1800.0f }
}};

inline constexpr std::array<std::array<float, 8>, 6> operatorDefaults {{
    { 1.0f, 0.0f, 86.0f, 8.0f, 8.0f, 840.0f, 62.0f, 1800.0f },
    { 2.0f, 3.0f, 64.0f, 12.0f, 4.0f, 620.0f, 32.0f, 1200.0f },
    { 3.5f, -7.0f, 42.0f, 24.0f, 18.0f, 1100.0f, 18.0f, 2200.0f },
    { 1.0f, -12.0f, 72.0f, 0.0f, 220.0f, 1600.0f, 78.0f, 3200.0f },
    { 0.5f, 8.0f, 28.0f, 18.0f, 380.0f, 2200.0f, 46.0f, 4000.0f },
    { 7.0f, 16.0f, 18.0f, 32.0f, 2.0f, 320.0f, 0.0f, 480.0f }
}};

inline constexpr std::array<Definition, 11> globalParameters {{
    { "cutoff", "Cutoff", 40.0f, 20000.0f, 1.0f, 8400.0f },
    { "resonance", "Resonance", 0.0f, 100.0f, 1.0f, 18.0f },
    { "drive", "Drive", 0.0f, 100.0f, 1.0f, 22.0f },
    { "lfo_rate", "LFO rate", 0.05f, 20.0f, 0.05f, 0.35f },
    { "lfo_depth", "LFO depth", 0.0f, 100.0f, 1.0f, 24.0f },
    { "space", "Diffusion", 0.0f, 100.0f, 1.0f, 32.0f },
    { "echo", "Echo", 0.0f, 100.0f, 1.0f, 18.0f },
    { "output", "Output", -60.0f, 0.0f, 0.1f, -12.0f },
    { "rupture_depth", "Rupture depth", 0.0f, 100.0f, 1.0f, 68.0f },
    { "rupture_time", "Rupture duration", 0.25f, 8.0f, 0.25f, 2.0f },
    { "rupture_scatter", "Rupture scatter", 0.0f, 100.0f, 1.0f, 36.0f }
}};

inline juce::String operatorId(int index, const char* suffix)
{
    return "op" + juce::String(index + 1) + "_" + suffix;
}

inline juce::String format(const juce::String& id, double value)
{
    if (id.endsWith("_ratio")) return juce::String(value, 2) + " x";
    if (id.endsWith("_fine")) return (value > 0 ? "+" : "") + juce::String(value, 0) + " ct";
    if (id.endsWith("_attack") || id.endsWith("_decay") || id.endsWith("_release"))
        return value >= 1000 ? juce::String(value / 1000.0, 2) + " s" : juce::String(value, 0) + " ms";
    if (id == "cutoff") return value >= 1000 ? juce::String(value / 1000.0, 2) + " k" : juce::String(value, 0) + " Hz";
    if (id == "lfo_rate") return juce::String(value, 2) + " Hz";
    if (id == "rupture_time") return juce::String(value, 2) + " bt";
    if (id == "output") return juce::String(value, 1) + " dB";
    return juce::String(value, 0) + " %";
}
}
