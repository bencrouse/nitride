#include "ReviewRegressionTests.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include "Instrument/SupportingControls.h"
#include "Instrument/FmPanel.h"
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
template<class T>T* child(juce::Component& root,const juce::String& name)
{for(auto* c:root.getChildren()){if(auto* found=dynamic_cast<T*>(c);found&&c->getName()==name)return found;if(auto* found=child<T>(*c,name))return found;}return nullptr;}
void accessibleUnits()
{
    RenderHarness h;NitrideAudioProcessor p(h.session.presetDirectory());auto& s=p.getInstrumentSession();std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    auto* panel=child<FmPanel>(*editor,"Advanced FM");require(panel&&panel->getDescription()=="FM LFO uses the enabled lane's waveform, independent of depth.","F5: FM caption contains corrupted source text");
    bool tooltipFound=false;std::function<void(juce::Component&)> inspectText=[&](juce::Component& root){for(auto* c:root.getChildren()){if(auto* slider=dynamic_cast<juce::Slider*>(c);slider&&slider->getTooltip().startsWith("Independent free-running")){tooltipFound=true;require(slider->getTooltip()=="Independent free-running rate, 0.01-20 Hz. Double-click resets.","F5: Motion tooltip contains corrupted source text");}inspectText(*c);}};inspectText(*editor);require(tooltipFound,"F5: Motion tooltip regression did not find its control");
    const auto enter=[&](const char* name,const char* value,Parameter parameter,double expected)
    {auto* c=child<UI::ValueControl>(*editor,name);require(c,"F4: Missing accessible value control");c->getAccessibilityHandler()->getValueInterface()->setValueAsString(value);require(std::abs(s.read().values[static_cast<size_t>(parameter)]-expected)<1e-8,"F4: Accessible units changed the wrong physical value");};
    enter("Attack","100 ms",attack,.1);enter("Attack","0.1 s",attack,.1);enter("Sustain","50%",sustain,.5);enter("Sustain","0.5",sustain,.5);
    enter("Filter attack","100 ms",filterAttack,.1);enter("Filter key tracking","50%",filterKeyTracking,.5);enter("Filter envelope amount","-2 oct",filterAmount,-2);
    enter("FM envelope attack","100 ms",fmAttack,.1);enter("FM envelope sustain","50%",fmSustain,.5);enter("FM ratio","1:1.414",fmRatio,1.414);enter("FM shape amount","50%",fmShapeAmount,.5);enter("FM operator mix","25%",fmMix,.25);enter("Output","-12 dB",output,-12);enter("Pitch","+3 st",pitch,3);
    auto* attackControl=child<UI::ValueControl>(*editor,"Attack");const auto before=s.read().values[attack];for(const auto* invalid:{"garbage","100 bananas","nan","inf","","1.2.3"})attackControl->getAccessibilityHandler()->getValueInterface()->setValueAsString(invalid);
    require(std::abs(s.read().values[attack]-before)<1e-12,"F4: Invalid string changed a control");
    std::function<void(juce::Component&)> roundtrip=[&](juce::Component& root){for(auto* c:root.getChildren()){if(auto* value=dynamic_cast<UI::ValueControl*>(c)){auto* api=value->getAccessibilityHandler()->getValueInterface();const auto text=api->getCurrentValueAsString();const auto parsed=UI::parseValue(text,value->unit);require(parsed.has_value(),"F4: Displayed value could not be parsed");api->setValueAsString(text);require(std::abs(api->getCurrentValue()-std::clamp(*parsed,api->getRange().getMinimumValue(),api->getRange().getMaximumValue()))<1e-8,"F4: Displayed unit round trip failed");}roundtrip(*c);}};roundtrip(*editor);
}
}
void runReviewRegressionTests(){midiChannelModes();interruptedRecall();accessibleUnits();std::cout<<"Code-review MIDI, recall and accessible-unit regressions passed\n";}
