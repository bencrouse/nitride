#pragma once

#include "InstrumentSession.h"

namespace Nitride
{
// The same rendering path is compiled into the hosted instrument and review app.
// This class owns no device, view, or message-thread component.
class InstrumentRenderer
{
public:
    explicit InstrumentRenderer(InstrumentSession& state) : session(state) {}
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&, bool injectEditorNotes = true);
    void reset() noexcept;
    SoundStudies::Engine::PitchSnapshot pitchSnapshot(int note=-1) const noexcept { return engine.getPitchSnapshot(note); }

private:
    void updateEngine();
    void renderRange(float*, float*, int);
    void handleMidi(const juce::MidiMessage&, bool editor);
    bool heldAnywhere(int note) const noexcept;
    void releaseChannel(int channel);
    void publish();
    void refreshMonoNote();
    void beginMidiGroup();

    InstrumentSession& session;
    SoundStudies::Engine engine;
    juce::Reverb reverb;
    std::array<std::array<double, 2>, 2> tone {};
    // Separate owners prevent closing an editor/key from releasing a host note
    // on the same channel and pitch.
    std::array<bool, 32 * 128> held {}, deferred {};
    std::array<bool,32*128> groupHeld{};
    std::array<std::uint16_t,32*128> keyCounts{};
    std::array<std::uint64_t,32*128> presses{};
    std::array<float,32*128> velocities{};
    std::uint64_t nextPress=0;
    int physicalKeys=0,groupKeys=0,monoNote=-1;
    bool monoMode=false;
    std::array<bool, 32> sustainPedal {};
    juce::MidiBuffer editorMidi;
    juce::SmoothedValue<float> cutoffSmooth, resonanceSmooth, gainSmooth;
    double rate = 48000, lfoPhase = 0, wheel = 0;
};
}
