#pragma once

#include "InstrumentSession.h"
#include "../DSP/Engine.h"
#include "../DSP/FilterEnvelope.h"
#include "../DSP/Lfo.h"
#include "../DSP/FilterModulation.h"

namespace Nitride
{
// The same rendering path is compiled into the hosted instrument and review app.
// This class owns no device, view, or message-thread component.
class InstrumentRenderer
{
public:
    explicit InstrumentRenderer(InstrumentSession& state) : session(state) {}
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&, bool injectEditorNotes = true,const DSP::Transport& transport={});
    void reset() noexcept;
    SoundStudies::Engine::PitchSnapshot pitchSnapshot(int note=-1) const noexcept { return engine.getPitchSnapshot(note); }
    double filterEnvelopeLevel() const noexcept {return filterEnvelope.level();}
    DSP::FilterEnvelope::Stage filterEnvelopeStage() const noexcept {return filterEnvelope.currentStage();}
    int filterNoteNumber() const noexcept {return filterNote;}
    double filterNoteVelocity() const noexcept {return filterVelocityValue;}
    double filterCutoffFrequency() const noexcept {return lastFilterCutoff;}
    SoundStudies::Engine::FmSnapshot fmSnapshot(int note=-1) const noexcept {return engine.getFmSnapshot(note);}

private:
    void updateEngine();
    void renderRange(float*, float*, int);
    void handleMidi(const juce::MidiMessage&, bool editor);
    bool heldAnywhere(int note) const noexcept;
    enum class ChannelRelease {detach,notes,sound};
    void releaseChannel(int channel,ChannelRelease mode=ChannelRelease::detach);
    void publish();
    void refreshMonoNote();
    int latestNoteOwner() const noexcept;
    void refreshFilterNote(int newest=-1);
    void beginMidiGroup();
    void releaseFilterGate() noexcept;

    InstrumentSession& session;
    SoundStudies::Engine engine;
    juce::Reverb reverb;
    std::array<std::array<double, 2>, 2> tone {};
    DSP::FilterEnvelope filterEnvelope;
    juce::SmoothedValue<double> filterAmountSmooth;
    juce::SmoothedValue<double> filterNoteSmooth,filterFmSmooth;
    int filterNote=DSP::filterTrackingReference,filterOwner=-1;
    double filterVelocityValue=0,keyTrackingAmount=0,velocityAmount=0,lastFilterCutoff=5500;
    bool fmRouteFrames=false;
    DSP::LfoBank lfos;
    // Separate owners prevent closing an editor/key from releasing a host note
    // on the same channel and pitch.
    std::array<bool, 32 * 128> held {}, deferred {};
    std::array<bool,32*128> groupHeld{};
    std::array<std::uint16_t,32*128> keyCounts{};
    std::array<std::uint64_t,32*128> presses{};
    std::array<float,32*128> velocities{};
    std::uint64_t nextPress=0;
    int physicalKeys=0,groupKeys=0,soundingKeys=0,monoNote=-1;
    bool monoMode=false;
    std::array<bool, 32> sustainPedal {};
    std::array<EditorMidiBridge::Command,EditorMidiBridge::blockBudget> editorCommands{};
    std::uint64_t editorEpoch=0;
    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Multiplicative> cutoffSmooth;
    juce::SmoothedValue<float> resonanceSmooth, gainSmooth;
    double rate = 48000, wheel = 0;
};
}
