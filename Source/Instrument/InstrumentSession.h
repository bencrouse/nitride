#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Studies/StudyEngine.h"
#include <array>
#include <atomic>
#include <cmath>

namespace Nitride
{
enum Parameter { coupling, stress, response, fm, pitch, attack, decay, sustain, release,
                 cutoff, resonance, motionRate, motionDepth, spaceMix, spaceSize, output, parameterCount };

struct Patch
{
    juce::String name;
    std::array<double, parameterCount> values {};
    bool mono = false;
    bool chord = false;
};

inline const std::array<Patch, 4>& factoryPatches()
{
    static const std::array<Patch, 4> patches {{
        { "01 / Soft pad", {.55,.08,.35,1.65,0,.65,.7,.72,1.4,5500,.18,.25,.08,.25,.7,-6}, false, true },
        { "02 / Glass keys", {.60,.20,.035,2.7,0,.004,.65,.2,.45,12000,.1,.5,0,.12,.4,-6}, false, false },
        { "03 / Contact lead", {.75,.4,.025,2.7,0,.006,.16,.62,.16,15000,.2,.75,0,.08,.3,-6}, true, false },
        { "04 / Hard hit", {.90,.70,.012,2.7,0,.003,.16,0,.22,15000,.15,.5,0,.05,.3,-6}, false, false }
    }};
    return patches;
}

struct LiveState
{
    std::array<std::atomic<float>, 6> offsets {};
    std::array<std::atomic<float>, 9> links {};
    std::atomic<float> level {0}, coherence {1}, peak {0};
    std::atomic<int> voices {0}, externalKeys {0};
    std::atomic<std::uint64_t> faults {0};

    void publish(const SoundStudies::Engine::NetworkSnapshot& snapshot) noexcept
    {
        for (size_t i = 0; i < offsets.size(); ++i) offsets[i].store(snapshot.offsets[i], std::memory_order_relaxed);
        for (size_t i = 0; i < links.size(); ++i) links[i].store(snapshot.links[i], std::memory_order_relaxed);
        level.store(snapshot.level); coherence.store(snapshot.coherence); voices.store(snapshot.activeVoices);
    }
};

// Processor-owned state. Views write conditions and queue notes; only the audio
// renderer touches the mutable DSP. A new editor can reopen on the current conditions.
class InstrumentSession
{
public:
    InstrumentSession() { apply(factoryPatches()[0]); }

    static double clamp(Parameter parameter, double value) noexcept
    {
        constexpr std::array<double, parameterCount> low {0,0,.01,0,-12,.001,.01,0,.02,40,0,.05,0,0,0,-24};
        constexpr std::array<double, parameterCount> high {1,1,1.6,4,12,4,4,1,8,18000,.85,8,1,.65,1,0};
        const auto i = static_cast<size_t>(parameter);
        return std::isfinite(value) ? juce::jlimit(low[i], high[i], value) : factoryPatches()[0].values[i];
    }

    void set(Parameter parameter, double value) noexcept
    {
        values[static_cast<size_t>(parameter)].store(clamp(parameter, value), std::memory_order_relaxed);
        revision.fetch_add(1, std::memory_order_release);
    }
    void apply(const Patch& patch) noexcept
    {
        for (size_t i = 0; i < parameterCount; ++i) values[i].store(clamp(static_cast<Parameter>(i), patch.values[i]), std::memory_order_relaxed);
        mono.store(patch.mono); chord.store(patch.chord);
        revision.fetch_add(1, std::memory_order_release);
    }
    Patch read() const
    {
        Patch patch;
        const auto index = juce::jlimit(0, 3, program.load());
        patch.name = factoryPatches()[static_cast<size_t>(index)].name;
        for (size_t i = 0; i < parameterCount; ++i) patch.values[i] = values[i].load(std::memory_order_relaxed);
        patch.mono = mono.load(); patch.chord = chord.load();
        return patch;
    }
    void applyProgram(int index) noexcept
    {
        if (index < 0 || index >= static_cast<int>(factoryPatches().size())) return;
        program.store(index); apply(factoryPatches()[static_cast<size_t>(index)]);
    }

    std::array<std::atomic<double>, parameterCount> values {};
    std::atomic<bool> mono {false}, chord {true};
    std::atomic<int> program {0};
    std::atomic<std::uint64_t> revision {0};
    juce::MidiKeyboardState keyboard;
    LiveState live;
};
}
