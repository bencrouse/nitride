#include "ReviewRegressionTests.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include <stdexcept>
#include <iostream>

namespace
{
using namespace Nitride;
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
struct RenderHarness
{
    InstrumentSession session{juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-review-"+juce::Uuid().toString())};
    InstrumentRenderer renderer{session};
    explicit RenderHarness(bool mono=false)
    {auto p=factoryPatches()[1];p.mono=mono;p.values[glideOn]=0;p.values[release]=8;p.values[spaceMix]=0;session.apply(p);renderer.prepare(48000);}
    void event(juce::MidiMessage message,int offset=0)
    {juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.addEvent(message,offset);renderer.process(audio,midi);}
    float tick(int samples=64)
    {juce::AudioBuffer<float> audio(2,samples);juce::MidiBuffer midi;renderer.process(audio,midi);return audio.getMagnitude(0,samples);}
};
void midiChannelModes()
{
    for(const auto mono:{false,true})
    {
        RenderHarness h(mono);h.event(juce::MidiMessage::noteOn(1,60,.8f));h.tick(2000);
        h.event(juce::MidiMessage::allSoundOff(1),32);require(h.session.live.voices.load()==0,"F2: All Sound Off entered ordinary release");h.tick(1024);require(h.tick(48000)<1e-6f,"F2: Panic left a long oscillator release");
        h.event(juce::MidiMessage::noteOn(1,60,.8f));h.event(juce::MidiMessage::controllerEvent(1,64,127));h.event(juce::MidiMessage::allNotesOff(1));
        require(h.session.live.voices.load()==1&&h.session.editorMidi.hostHeld[60].load()==0&&h.session.editorMidi.hostSounding[60].load()!=0,"F2: All Notes Off ignored the sustain pedal");
        h.event(juce::MidiMessage::controllerEvent(1,64,0));h.tick(400000);require(h.session.live.voices.load()==0,"F2: Pedal-up did not release All Notes Off owners");
        h.event(juce::MidiMessage::noteOn(1,60,.8f));h.session.keyboard.noteOn(16,60,.9f);h.tick();h.event(juce::MidiMessage::allSoundOff(1));
        require(h.session.live.voices.load()==1,"F2: Host panic killed an editor Hold capture");h.session.keyboard.noteOff(16,60,0);h.tick();h.tick(400000);require(h.session.live.voices.load()==0,"F2: Transferred Hold voice could not release");
    }
    RenderHarness isolated;isolated.event(juce::MidiMessage::noteOn(1,60,.8f));isolated.event(juce::MidiMessage::noteOn(2,67,.8f));isolated.event(juce::MidiMessage::allSoundOff(1));
    require(isolated.session.live.voices.load()==1,"F2: Channel panic killed an unrelated channel");
    isolated.event(juce::MidiMessage::noteOff(2,67));isolated.event(juce::MidiMessage::allSoundOff(2));require(isolated.session.live.voices.load()==0,"F2: Panic missed an already-releasing channel voice");
    RenderHarness samePitch;samePitch.event(juce::MidiMessage::noteOn(1,60,.8f));samePitch.event(juce::MidiMessage::noteOn(2,60,.8f));samePitch.event(juce::MidiMessage::allSoundOff(1));require(samePitch.session.live.voices.load()==1,"F2: Panic did not isolate same-pitch voices by origin");
    RenderHarness boundary;boundary.event(juce::MidiMessage::noteOn(1,60,.8f));boundary.tick(2000);juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::allSoundOff(1),64);boundary.renderer.process(audio,midi);
    require(audio.getMagnitude(0,64)>.001f&&boundary.session.live.voices.load()==0,"F2: Panic was applied before its sample offset");
    RenderHarness oldTail;oldTail.event(juce::MidiMessage::noteOn(2,60,.8f));oldTail.tick(2000);oldTail.event(juce::MidiMessage::noteOff(2,60));
    oldTail.event(juce::MidiMessage::noteOn(1,60,.8f));oldTail.session.keyboard.noteOn(16,60,.8f);oldTail.tick();oldTail.event(juce::MidiMessage::allSoundOff(1));oldTail.tick(400000);
    require(oldTail.session.live.voices.load()==1,"F2: An unrelated release tail prevented transfer to a held editor capture");
}
bool samePatch(const Patch& a,const Patch& b)
{if(a.name!=b.name||a.mono!=b.mono||a.chord!=b.chord)return false;for(size_t i=0;i<parameterCount;++i)if(std::abs(a.values[i]-b.values[i])>1e-8)return false;return true;}
void interruptedRecall()
{
    for(const auto p:{fm,fmRatio,response,coupling})
    {
        InstrumentSession s(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-recall-"+juce::Uuid().toString()));
        int begins=0,ends=0;s.beginHostGesture=[&](int){++begins;};s.endHostGesture=[&](int){++ends;};
        const auto generation=s.restoreEpoch.load();s.beginEdit({hostIndex(p),hostIndex(stress)});s.set(p,hostParameters[static_cast<size_t>(p)].maximum);s.set(stress,.91);const auto edited=s.read();
        require(s.selectPreset("factory:1"),"F3: Factory selection failed");const auto recalled=s.read();
        require(!s.setIfCurrent(p,.123,generation)&&!s.setGroupIfCurrent({{p,.123},{stress,.12}},generation),"F3: Old gesture overwrote preset recall");s.endEdit({hostIndex(p),hostIndex(stress)});
        require(samePatch(s.read(),factoryPatches()[1])&&begins==ends,"F3: Recall mixed edits or left unbalanced host gestures");
        const auto undoGeneration=s.restoreEpoch.load();s.undo();require(samePatch(s.read(),edited)&&s.selectedPresetId()=="factory:0","F3: Recall Undo did not restore the complete pre-selection state");
        require(!s.setIfCurrent(p,.123,undoGeneration),"F3: Undo retained obsolete gesture generation");s.redo();require(samePatch(s.read(),recalled)&&s.selectedPresetId()=="factory:1","F3: Recall Redo lost document identity");
        s.undo();s.undo();require(samePatch(s.read(),factoryPatches()[0]),"F3: Interrupted edit history crossed preset boundaries");
    }
    const auto directory=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-import-recall-"+juce::Uuid().toString());InstrumentSession s(directory);
    const auto file=directory.getChildFile("import.nitridepreset");require(PresetStore::exportFile(file,{"import",factoryPatches()[1]}).wasOk(),"F3: Cannot prepare import regression");
    for(int mode=0;mode<3;++mode)
    {
        const auto generation=s.restoreEpoch.load();s.beginEdit({hostIndex(fmRatio)});s.set(fmRatio,3.5);
        if(mode==0)require(s.importPreset(file).wasOk(),"F3: Import failed");
        else if(mode==1)require(s.keepPatch("Saved during edit",factoryPatches()[2]).wasOk(),"F3: Keep selection failed");
        else s.apply(factoryPatches()[3]);
        const auto recalled=s.read();require(!s.setIfCurrent(fmRatio,.4,generation),"F3: Import/Keep/apply retained an old gesture");s.endEdit({hostIndex(fmRatio)});require(samePatch(s.read(),recalled),"F3: Ending an old edit damaged recall");
    }
    directory.deleteRecursively();
}
}
void runReviewRegressionTests(){midiChannelModes();interruptedRecall();std::cout<<"Code-review MIDI and interrupted-recall regressions passed\n";}
