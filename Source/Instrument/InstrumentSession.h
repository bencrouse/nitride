#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Studies/StudyEngine.h"
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <optional>
#include <vector>

namespace Nitride
{
enum Parameter { coupling, stress, response, fm, pitch, attack, decay, sustain, release,
                 cutoff, resonance, motionRate, motionDepth, spaceMix, spaceSize, output, parameterCount };
inline constexpr int monoHostIndex = parameterCount;
inline constexpr int hostParameterCount = parameterCount + 1;

struct Patch
{
    juce::String name;
    std::array<double, parameterCount> values {};
    bool mono = false, chord = false;
};
struct StoredPreset { juce::String id; Patch patch; };
class PresetStore;

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
    void publish(const SoundStudies::Engine::NetworkSnapshot& s) noexcept
    {
        for(size_t i=0;i<offsets.size();++i)offsets[i].store(s.offsets[i],std::memory_order_relaxed);
        for(size_t i=0;i<links.size();++i)links[i].store(s.links[i],std::memory_order_relaxed);
        level.store(s.level);coherence.store(s.coherence);voices.store(s.activeVoices);
    }
};

class InstrumentSession
{
public:
    explicit InstrumentSession(juce::File presetDirectory = {});
    ~InstrumentSession();
    static double clamp(Parameter,double) noexcept;
    void set(Parameter,double);
    void setMono(bool);
    void storeFromHost(int,double) noexcept;
    void apply(const Patch&);
    void applyProgram(int);
    Patch read() const;
    Patch readVisible() const;
    double audioValue(Parameter p) const noexcept { return comparing.load() ? compareValues[static_cast<size_t>(p)].load() : values[static_cast<size_t>(p)].load(); }
    bool audioMono() const noexcept { return comparing.load() ? compareMono.load() : mono.load(); }

    void beginEdit(std::initializer_list<int> parameters);
    void endEdit(std::initializer_list<int> parameters);
    void endAllEdits();
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void setComparing(bool);
    bool isComparing() const noexcept { return comparing.load(); }
    Patch compareReference() const;
    void setCompareReference(const Patch&);

    std::vector<StoredPreset> presets() const;
    juce::String selectedPresetId() const;
    bool selectPreset(const juce::String&);
    juce::Result keep(const juce::String& name);
    juce::Result keepPatch(const juce::String& name,const Patch&);
    juce::Result importPreset(const juce::File&);
    juce::Result exportPreset(const juce::File&) const;
    const juce::File& presetDirectory() const;
    void refreshPresets();

    juce::var saveDocument() const;
    juce::Result restoreDocument(const juce::var&);
    void establishRestoredPatch(const Patch&,int programIndex);

    // Bound once by the processor, absent in the independent review app. Parameter
    // listeners never enter the document lock, file store, or UI history.
    std::function<void(int,double)> writeHostParameter;
    std::function<void(int)> beginHostGesture, endHostGesture;

    std::array<std::atomic<double>, parameterCount> values {};
    std::atomic<bool> mono {false}, chord {true};
    std::atomic<int> program {0};
    std::atomic<std::uint64_t> revision {0}, restoreEpoch {0}, libraryRevision {0};
    juce::MidiKeyboardState keyboard;
    LiveState live;

private:
    struct Conditions { std::array<double,parameterCount> values; bool mono,chord; };
    struct Snapshot
    {
        Patch patch, reference;
        juce::String selected;
        int program;
        // Patch selection restores the whole document; a control edit restores
        // only the parameters it wrote, preserving unrelated host automation.
        bool wholePatch = true;
        std::array<bool,hostParameterCount> parameters {};
    };
    Conditions readConditions() const;
    void storeConditions(const Patch&) noexcept;
    void writePatch(const Patch&,bool notifyHost);
    void setInternal(int,double,bool notifyHost);
    void syncProgram() const;
    Snapshot snapshot() const;
    void restoreSnapshot(const Snapshot&);
    bool editChanged() const;
    void finishEdit();
    void rememberPortable(const StoredPreset&);
    static bool same(const Patch&,const Patch&);
    static juce::var encodeSnapshot(const Snapshot&);
    static std::optional<Snapshot> decodeSnapshot(const juce::var&);
    void closeCompare() noexcept;

    mutable juce::CriticalSection documentLock;
    mutable std::atomic_flag conditionWriter = ATOMIC_FLAG_INIT;
    std::atomic<std::uint64_t> conditionVersion {0};
    mutable std::atomic<int> pendingProgram {-1};
    mutable juce::String currentName, selectedId;
    mutable Patch reference;
    std::vector<StoredPreset> userPresets, portablePresets;
    mutable std::vector<Snapshot> history, future;
    mutable std::optional<Snapshot> editStart;
    mutable std::array<int,hostParameterCount> gestures {};
    mutable int editDepth = 0;
    std::unique_ptr<PresetStore> store;
    std::array<std::atomic<double>,parameterCount> compareValues {};
    std::atomic<bool> comparing {false}, compareMono {false};
};
}
