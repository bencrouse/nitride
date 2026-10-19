#include "ReviewRegressionTests.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include "Instrument/SupportingControls.h"
#include "Instrument/FmPanel.h"
#include "FactoryPrograms.h"
#include <stdexcept>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

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
void compareReplacement()
{
    InstrumentSession s(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-compare-review-"+juce::Uuid().toString()));
    const auto edited=s.read();s.setCompareReference(factoryPatches()[1]);require(!s.isComparing()&&samePatch(s.readVisible(),edited),"F6: Off-state reference replacement changed edited sound");
    s.setComparing(true);require(std::abs(s.audioValue(fm)-2.7)<1e-12,"F6: New off-state reference was not auditioned");
    s.setCompareReference(factoryPatches()[2]);require(!s.isComparing()&&samePatch(s.readVisible(),edited),"F6: Active reference replacement did not end Compare");
    require(std::abs(s.audioValue(fm)-edited.values[fm])<1e-12&&s.audioMono()==edited.mono,"F6: View and audio disagree after reference replacement");
    s.setComparing(true);require(s.audioMono()&&samePatch(s.readVisible(),factoryPatches()[2]),"F6: Replacement reference lost mono or visible state");
    for(size_t i=0;i<parameterCount;++i)require(std::abs(s.audioValue(static_cast<Parameter>(i))-s.readVisible().values[i])<1e-12,"F6: Compared parameter bank disagrees with visible reference");
    require(samePatch(s.read(),edited),"F6: Reference replacement overwrote edited conditions");
}
void embeddedReviewPrograms()
{
    const auto library=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-embedded-review-"+juce::Uuid().toString());
    NitrideAudioProcessor p(library);require(p.getNumPrograms()==static_cast<int>(builtInPresets().size())&&p.getNumPrograms()>=63,"Review programs missing from host catalog");
    for(size_t i=0;i<factoryPatches().size();++i)require(samePatch(builtInPresets()[i].patch,factoryPatches()[i]),"Embedding review presets changed an original factory patch");
    for(int index=static_cast<int>(factoryPatches().size());index<p.getNumPrograms();++index)
    {
        const auto& expected=builtInPresets()[static_cast<size_t>(index)];p.setCurrentProgram(index);const auto actual=p.getInstrumentSession().read();
        if(!samePatch(actual,expected.patch))for(size_t i=0;i<parameterCount;++i)if(std::abs(actual.values[i]-expected.patch.values[i])>1e-8)std::cerr<<"Review echo mismatch "<<hostParameters[i].id<<": "<<actual.values[i]<<" vs "<<expected.patch.values[i]<<'\n';
        require(samePatch(actual,expected.patch),"Host review program conditions differ from embedded source");
        require(p.getInstrumentSession().selectedPresetId()==expected.id,"Review program lost stable identity");
        juce::MemoryBlock state;p.getStateInformation(state);NitrideAudioProcessor restored(library);restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        require(restored.getCurrentProgram()==index&&samePatch(restored.getInstrumentSession().read(),expected.patch)&&restored.getInstrumentSession().selectedPresetId()==expected.id,"Review program/conditions failed portable session recall");
        auto& session=p.getInstrumentSession();require(session.selectPreset("factory:0")&&session.selectPreset(expected.id),"Embedded review cannot be selected through native preset workflow");
        session.undo();require(session.selectedPresetId()=="factory:0","Review selection Undo lost original document");session.redo();require(session.selectedPresetId()==expected.id&&samePatch(session.read(),expected.patch),"Review selection Redo lost immutable conditions");
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    require(child<juce::ComboBox>(*editor,"Starting patch")==nullptr,"Hosted editor still exposes its own factory browser");
    editor.reset();
    {
        InstrumentView standalone(p.getInstrumentSession());
        auto* choices=child<juce::ComboBox>(standalone,"Starting patch");
        require(choices&&choices->getNumItems()==static_cast<int>(factoryPrograms.size()),"Standalone browser is not curated");
        for(size_t i=0;i<factoryPrograms.size();++i)
        {
            const auto& entry=factoryPrograms[i];
            require(choices->getItemText(static_cast<int>(i))==entry.name&&builtInIndex(entry.id)==entry.program,"Curated label displaced a historical identity/program");
        }
        choices->setSelectedId(1,juce::sendNotificationSync);
        require(p.getInstrumentSession().selectedPresetId()==factoryPrograms[0].id&&p.getCurrentProgram()==factoryPrograms[0].program,"Curated standalone selection remapped the legacy program");
    }
    const auto& bassR1=builtInPresets()[5].patch;const auto& bassR2=builtInPresets()[7].patch;
    require(bassR1.values[autobendOn]>.5&&bassR2.values[autobendOn]<.5&&bassR1.mono==bassR2.mono&&bassR1.chord==bassR2.chord,"Bass r2 lost its isolated attack adjustment");
    for(size_t i=0;i<parameterCount;++i)if(i!=autobendOn)require(SoundStudies::samePitchValue(bassR1.values[i],bassR2.values[i]),"Bass r2 changed a condition besides phrase-onset pitch bend");
    const auto lead=builtInIndex("review:glass-current:r1"),sequence=builtInIndex("review:pulse-lattice:r1");
    require(lead==8&&sequence==9,"Lead/sequence programs displaced an earlier review revision");
    const auto& leadPatch=builtInPresets()[static_cast<size_t>(lead)].patch;const auto& sequencePatch=builtInPresets()[static_cast<size_t>(sequence)].patch;
    require(leadPatch.mono&&!leadPatch.chord&&leadPatch.values[legato]>.5&&leadPatch.values[glideLegato]>.5&&leadPatch.values[autobendOn]<.5,"Lead sketch lost its legato/no-attack-bend conditions");
    require(!sequencePatch.mono&&!sequencePatch.chord&&sequencePatch.values[glideOn]<.5&&sequencePatch.values[autobendOn]<.5&&sequencePatch.values[cutoffLfoSync]>.5,"Sequence sketch lost its polyphonic, unbent, synced articulation");
    const auto leadR2=builtInIndex("review:glass-current:r2"),sequenceR2=builtInIndex("review:pulse-lattice:r2");
    require(leadR2==10&&sequenceR2==11,"Lead/sequence r2 displaced an earlier program");
    const auto& quieterLead=builtInPresets()[static_cast<size_t>(leadR2)].patch;const auto& steppedSequence=builtInPresets()[static_cast<size_t>(sequenceR2)].patch;
    require(quieterLead.mono==leadPatch.mono&&quieterLead.chord==leadPatch.chord&&quieterLead.values[coupling]<leadPatch.values[coupling]&&quieterLead.values[stress]<leadPatch.values[stress],"Lead r2 lost its isolated coupling/stress reduction");
    constexpr std::array sequenceChanges{fmLfoWave,fmLfoDivision,fmRoute3Source,fmRoute3Target,fmRoute3Amount};
    require(steppedSequence.mono==sequencePatch.mono&&steppedSequence.chord==sequencePatch.chord&&SoundStudies::samePitchValue(steppedSequence.values[fmLfoWave],4)&&SoundStudies::samePitchValue(steppedSequence.values[fmLfoDivision],8)&&SoundStudies::samePitchValue(steppedSequence.values[fmRoute3Source],1)&&SoundStudies::samePitchValue(steppedSequence.values[fmRoute3Target],1)&&SoundStudies::samePitchValue(steppedSequence.values[fmRoute3Amount],.11),"Sequence r2 lost its stepped FM-ratio recipe");
    for(size_t i=0;i<parameterCount;++i)
    {
        if(i!=coupling&&i!=stress)require(SoundStudies::samePitchValue(quieterLead.values[i],leadPatch.values[i]),"Lead r2 changed a condition besides coupling/stress");
        if(std::find(sequenceChanges.begin(),sequenceChanges.end(),static_cast<Parameter>(i))==sequenceChanges.end())require(SoundStudies::samePitchValue(steppedSequence.values[i],sequencePatch.values[i]),"Sequence r2 changed articulation, levels or another unrelated condition");
    }
    const auto material=builtInIndex("review:hollow-bronze:r1"),experimental=builtInIndex("review:fault-bloom:r1");
    require(material==12&&experimental==13,"Material/experimental programs displaced an earlier review revision");
    const auto& strike=builtInPresets()[static_cast<size_t>(material)].patch;const auto& bloom=builtInPresets()[static_cast<size_t>(experimental)].patch;
    require(!strike.mono&&!strike.chord&&SoundStudies::pitchIsZero(strike.values[sustain])&&strike.values[glideOn]<.5&&strike.values[autobendOn]<.5,"Material sketch lost its unbent struck-decay articulation");
    for(const auto& lane:lfoParameters)require(strike.values[lane.enabled]<.5,"Material sketch acquired cyclic motion instead of struck response");
    const auto materialR2=builtInIndex("review:hollow-bronze:r2");require(materialR2==14,"Material r2 displaced an earlier program");
    const auto& reducedStrike=builtInPresets()[static_cast<size_t>(materialR2)].patch;
    require(reducedStrike.mono==strike.mono&&reducedStrike.chord==strike.chord&&reducedStrike.values[fm]<strike.values[fm]&&std::abs((reducedStrike.values[fm]+reducedStrike.values[fmEnvelopeAmount])-(strike.values[fm]+strike.values[fmEnvelopeAmount]))<1e-12,"Material r2 lost its reduced tail FM/unchanged nominal strike peak");
    for(size_t i=0;i<parameterCount;++i)if(i!=fm&&i!=fmEnvelopeAmount)require(SoundStudies::samePitchValue(reducedStrike.values[i],strike.values[i]),"Material r2 changed a condition besides settled FM/strike compensation");
    require(bloom.mono&&!bloom.chord&&bloom.values[glideOn]>.5&&bloom.values[glideLegato]>.5&&bloom.values[legato]>.5&&SoundStudies::samePitchValue(bloom.values[glideTarget],3)&&bloom.values[fmRatio]<1&&bloom.values[autobendOn]<.5,"Experimental sketch lost its playable opposed-glide/sub-unity-FM conditions");
    const auto prism=builtInIndex("review:prism-pulse:r1"),felt=builtInIndex("review:felt-weight:r1");
    require(prism==15&&felt==16,"Contrasting candidates displaced an established program");
    const auto& rhythmicPad=builtInPresets()[static_cast<size_t>(prism)].patch;const auto& dryBass=builtInPresets()[static_cast<size_t>(felt)].patch;
    require(!rhythmicPad.mono&&!rhythmicPad.chord&&rhythmicPad.values[ampLfoOn]>.5&&rhythmicPad.values[ampLfoSync]>.5&&rhythmicPad.values[ampLfoDepth]>.8&&rhythmicPad.values[attack]<builtInPresets()[6].patch.values[attack],"Contrasting pad lost its rhythmic/quick-attack identity");
    require(dryBass.mono&&!dryBass.chord&&SoundStudies::pitchIsZero(dryBass.values[stress])&&SoundStudies::pitchIsZero(dryBass.values[spaceMix])&&dryBass.values[legato]<.5&&dryBass.values[glideOn]<.5&&dryBass.values[sustain]<.1,"Contrasting bass lost its dry/retriggered/damped identity");
    for(const auto& lane:lfoParameters)require(dryBass.values[lane.enabled]<.5,"Contrasting bass acquired cyclic motion instead of dry articulation");
    const auto prismR2=builtInIndex("review:prism-pulse:r2");require(prismR2==17,"Prism r2 displaced an established program");
    const auto& chordPad=builtInPresets()[static_cast<size_t>(prismR2)].patch;
    constexpr std::array prismChanges{fmRatio,fmRoute1Amount,fmShapeAmount,fmMix,output};
    require(chordPad.mono==rhythmicPad.mono&&chordPad.chord==rhythmicPad.chord&&SoundStudies::samePitchValue(chordPad.values[fmRatio],2)&&SoundStudies::pitchIsZero(chordPad.values[fmRoute1Amount])&&chordPad.values[fmShapeAmount]<rhythmicPad.values[fmShapeAmount]&&chordPad.values[fmMix]<rhythmicPad.values[fmMix]&&chordPad.values[output]<rhythmicPad.values[output],"Prism r2 lost its chord-friendly FM/level-trim recipe");
    for(size_t i=0;i<parameterCount;++i)if(std::find(prismChanges.begin(),prismChanges.end(),static_cast<Parameter>(i))==prismChanges.end())require(SoundStudies::samePitchValue(chordPad.values[i],rhythmicPad.values[i]),"Prism r2 changed pulse, envelopes, filter or another unrelated condition");
    const auto velvet=builtInIndex("review:prism-pulse:r3"),reed=builtInIndex("review:reed-snap:r1"),undertow=builtInIndex("review:undertow-grid:r1");
    require(velvet==18&&reed==19&&undertow==20,"Name-only/new-family programs displaced an earlier revision");
    const auto& renamed=builtInPresets()[static_cast<size_t>(velvet)].patch;const auto& stabs=builtInPresets()[static_cast<size_t>(reed)].patch;const auto& heldSequence=builtInPresets()[static_cast<size_t>(undertow)].patch;
    require(renamed.name=="Review / Pads / Velvet Pulse r3"&&renamed.mono==chordPad.mono&&renamed.chord==chordPad.chord,"Velvet rename lost name or articulation");
    for(size_t i=0;i<parameterCount;++i)require(SoundStudies::samePitchValue(renamed.values[i],chordPad.values[i]),"Name-only Velvet revision changed sound conditions");
    require(!stabs.mono&&!stabs.chord&&stabs.values[legato]<.5&&stabs.values[glideOn]<.5&&stabs.values[attack]<.01,"Reed contrast lost poly/retriggered fast articulation");
    for(const auto& lane:lfoParameters)require(stabs.values[lane.enabled]<.5,"Reed contrast acquired cyclic modulation");
    require(heldSequence.mono&&!heldSequence.chord&&heldSequence.values[legato]>.5&&heldSequence.values[sustain]>.8&&heldSequence.values[ampLfoOn]>.5&&heldSequence.values[ampLfoSync]>.5&&SoundStudies::samePitchValue(heldSequence.values[ampLfoDivision],15)&&SoundStudies::samePitchValue(heldSequence.values[cutoffLfoWave],4),"Undertow lost held-note pulse/stepped-tone contrast");
    const auto reedR2=builtInIndex("review:reed-snap:r2");require(reedR2==21,"Reed r2 displaced an earlier program");
    const auto& transientReed=builtInPresets()[static_cast<size_t>(reedR2)].patch;
    constexpr std::array reedChanges{fmShape,fmShapeAmount,fmRoute2Source,fmRoute2Target,fmRoute2Amount,output};
    require(transientReed.mono==stabs.mono&&transientReed.chord==stabs.chord&&SoundStudies::samePitchValue(transientReed.values[fmShape],1)&&SoundStudies::samePitchValue(transientReed.values[fmRoute2Source],3)&&SoundStudies::samePitchValue(transientReed.values[fmRoute2Target],2)&&SoundStudies::samePitchValue(transientReed.values[fmRoute2Amount],.3),"Reed r2 lost alternate harmonic geometry/transient route");
    for(size_t i=0;i<parameterCount;++i)if(std::find(reedChanges.begin(),reedChanges.end(),static_cast<Parameter>(i))==reedChanges.end())require(SoundStudies::samePitchValue(transientReed.values[i],stabs.values[i]),"Reed r2 changed stress, coupling, FM depth, articulation or another unrelated condition");
    const auto skin=builtInIndex("review:skin-knock:r1"),orbit=builtInIndex("review:orbit-shards:r1");
    require(skin==22&&orbit==23,"Material/experimental contrasts displaced an earlier program");
    const auto& membrane=builtInPresets()[static_cast<size_t>(skin)].patch;const auto& shards=builtInPresets()[static_cast<size_t>(orbit)].patch;
    require(!membrane.mono&&!membrane.chord&&SoundStudies::pitchIsZero(membrane.values[sustain])&&membrane.values[decay]<.3&&membrane.values[spaceMix]<.03&&membrane.values[autobendOn]>.5&&membrane.values[autobendPhrase]<.5&&SoundStudies::samePitchValue(membrane.values[autobendTarget],2)&&SoundStudies::samePitchValue(membrane.values[autobendDepth],7)&&SoundStudies::samePitchValue(membrane.values[fmRatio],.5),"Skin Knock lost its short pitch-relaxing material articulation");
    for(const auto& lane:lfoParameters)require(membrane.values[lane.enabled]<.5,"Skin Knock acquired cyclic motion instead of struck response");
    require(!shards.mono&&!shards.chord&&shards.values[glideOn]<.5&&shards.values[legato]<.5&&shards.values[autobendOn]<.5&&shards.values[stress]<.02&&shards.values[pitchLfoOn]>.5&&SoundStudies::samePitchValue(shards.values[pitchLfoWave],4)&&shards.values[pitchLfoSync]<.5&&SoundStudies::samePitchValue(shards.values[pitchLfoDepth],9)&&shards.values[fmMix]>.3&&shards.values[spaceMix]>.4,"Orbit Shards lost low-stress poly/free stepped-pitch contrast");
    const auto taffy=builtInIndex("review:taffy-coil:r1"),still=builtInIndex("review:stillwater:r1"),crooked=builtInIndex("review:crooked-velvet:r1");
    require(taffy==24&&still==25&&crooked==26,"Artist-lens studies displaced an earlier program");
    const auto& elastic=builtInPresets()[static_cast<size_t>(taffy)].patch;const auto& patient=builtInPresets()[static_cast<size_t>(still)].patch;const auto& loping=builtInPresets()[static_cast<size_t>(crooked)].patch;
    require(!elastic.mono&&!elastic.chord&&elastic.values[autobendOn]>.5&&elastic.values[autobendPhrase]<.5&&SoundStudies::samePitchValue(elastic.values[autobendTarget],3)&&SoundStudies::samePitchValue(elastic.values[autobendDepth],12)&&elastic.values[filterFmDepth]>.3,"Taffy study lost its opposed elastic strike");
    for(const auto& lane:lfoParameters)require(elastic.values[lane.enabled]<.5,"Taffy strike acquired unrelated cyclic motion");
    require(!patient.mono&&!patient.chord&&patient.values[attack]>3&&patient.values[release]>6&&patient.values[spaceMix]>.45&&patient.values[stress]<.02&&patient.values[ampLfoOn]<.5&&patient.values[fmLfoOn]>.5&&patient.values[fmLfoRate]<.04&&patient.values[fmRoute1Target]==1,"Stillwater study lost patient harmonic-space recipe");
    for(const auto& lane:lfoParameters)require(patient.values[lane.sync]<.5,"Stillwater study acquired tempo-locked motion");
    require(loping.mono&&!loping.chord&&loping.values[legato]<.5&&loping.values[glideOn]>.5&&SoundStudies::samePitchValue(loping.values[glideTarget],0)&&loping.values[autobendOn]>.5&&SoundStudies::samePitchValue(loping.values[autobendTarget],1)&&SoundStudies::samePitchValue(loping.values[autobendDepth],-5)&&loping.values[ampLfoOn]>.5&&loping.values[ampLfoSync]<.5&&SoundStudies::samePitchValue(loping.values[fmLfoWave],4),"Crooked bass study lost retriggered modulator attack/free motion");
    const auto crookedR2=builtInIndex("review:crooked-velvet:r2");require(crookedR2==27,"Crooked r2 displaced an earlier program");
    const auto& anchoredBass=builtInPresets()[static_cast<size_t>(crookedR2)].patch;
    constexpr std::array rootChanges{fm,fmRatio,fmMix,fmRoute2Amount,fmRoute3Target,output};
    require(anchoredBass.mono==loping.mono&&anchoredBass.chord==loping.chord&&SoundStudies::samePitchValue(anchoredBass.values[fm],.4)&&SoundStudies::samePitchValue(anchoredBass.values[fmRatio],1)&&SoundStudies::samePitchValue(anchoredBass.values[fmMix],.035)&&SoundStudies::samePitchValue(anchoredBass.values[fmRoute2Amount],.04)&&SoundStudies::samePitchValue(anchoredBass.values[fmRoute3Target],0),"Crooked r2 lost carrier-heavy/root-centered FM recipe");
    for(size_t i=0;i<parameterCount;++i)if(std::find(rootChanges.begin(),rootChanges.end(),static_cast<Parameter>(i))==rootChanges.end())require(SoundStudies::samePitchValue(anchoredBass.values[i],loping.values[i]),"Crooked r2 changed attack bend, glide, groove/envelope color, Tone or network/Space conditions");
    const auto replica=builtInIndex("review:replica-voice:r1"),tendon=builtInIndex("review:tendon-drift:r1"),socket=builtInIndex("review:socket-dance:r1");
    require(replica==28&&tendon==29&&socket==30,"Second artist-lens batch displaced an earlier program");
    const auto& uncanny=builtInPresets()[static_cast<size_t>(replica)].patch;const auto& drag=builtInPresets()[static_cast<size_t>(tendon)].patch;const auto& cell=builtInPresets()[static_cast<size_t>(socket)].patch;
    require(!uncanny.mono&&!uncanny.chord&&uncanny.values[glideOn]<.5&&uncanny.values[autobendOn]<.5&&SoundStudies::pitchIsZero(uncanny.values[filterKeyTracking])&&uncanny.values[filterAmount]<0&&uncanny.values[filterVelocity]<0&&uncanny.values[fmEnvelopeAmount]<0,"Replica study lost fixed-resonance/inverted-envelope poly voice recipe");
    require(drag.mono&&!drag.chord&&drag.values[legato]>.5&&drag.values[glideLegato]>.5&&SoundStudies::samePitchValue(drag.values[glideTarget],4)&&SoundStudies::samePitchValue(drag.values[glideCurve],1)&&drag.values[glideTime]>1&&drag.values[autobendOn]<.5&&SoundStudies::samePitchValue(drag.values[stressLfoWave],4)&&drag.values[stressLfoSync]<.5,"Tendon study lost Pitch + Tone drag/intermittent-strain articulation");
    require(!cell.mono&&!cell.chord&&cell.values[glideOn]<.5&&cell.values[autobendOn]<.5&&SoundStudies::pitchIsZero(cell.values[spaceMix])&&cell.values[ampLfoOn]>.5&&cell.values[ampLfoSync]>.5&&SoundStudies::samePitchValue(cell.values[ampLfoWave],3)&&SoundStudies::samePitchValue(cell.values[ampLfoDivision],17)&&SoundStudies::samePitchValue(cell.values[cutoffLfoWave],4)&&SoundStudies::samePitchValue(cell.values[cutoffLfoDivision],12)&&SoundStudies::samePitchValue(cell.values[fmLfoDivision],13)&&cell.values[fmEnvelopeAmount]<0,"Socket study lost dry poly/triplet-cell/divergent-clock recipe");
    for(const auto& [identity,parentIndex,newIndex,seconds]:std::array{
        std::tuple{"review:replica-voice:r2",replica,31,.4},std::tuple{"review:socket-dance:r2",socket,32,.045}})
    {
        const auto index=builtInIndex(identity);require(index==newIndex,"Tail-only r2 displaced an earlier program");
        const auto& before=builtInPresets()[static_cast<size_t>(parentIndex)].patch;const auto& after=builtInPresets()[static_cast<size_t>(index)].patch;
        require(before.mono==after.mono&&before.chord==after.chord&&SoundStudies::samePitchValue(after.values[release],seconds),"Tail-only r2 lost articulation or shorter release");
        for(size_t i=0;i<parameterCount;++i)if(i!=release)require(SoundStudies::samePitchValue(before.values[i],after.values[i]),"Tail-only r2 changed voice/clock/Space/output or another unrelated condition");
    }
    const auto canvas=builtInIndex("review:sub-canvas:r1"),afterimage=builtInIndex("review:afterimage-keys:r1"),lantern=builtInIndex("review:overtone-lantern:r1"),buzz=builtInIndex("review:buzz-loom:r1"),alloy=builtInIndex("review:cutoff-alloy:r1"),brake=builtInIndex("review:brake-bloom:r1");
    require(canvas==33&&afterimage==34&&lantern==35&&buzz==36&&alloy==37&&brake==38,"Frontier studies displaced earlier programs");
    const auto& bare=builtInPresets()[static_cast<size_t>(canvas)].patch;const auto& twin=builtInPresets()[static_cast<size_t>(afterimage)].patch;const auto& exposed=builtInPresets()[static_cast<size_t>(lantern)].patch;const auto& flutter=builtInPresets()[static_cast<size_t>(buzz)].patch;const auto& filterMetal=builtInPresets()[static_cast<size_t>(alloy)].patch;const auto& restrain=builtInPresets()[static_cast<size_t>(brake)].patch;
    require(bare.mono&&SoundStudies::pitchIsZero(bare.values[coupling])&&SoundStudies::pitchIsZero(bare.values[stress])&&SoundStudies::pitchIsZero(bare.values[fm])&&SoundStudies::pitchIsZero(bare.values[fmEnvelopeAmount])&&SoundStudies::pitchIsZero(bare.values[fmMix])&&SoundStudies::pitchIsZero(bare.values[spaceMix]),"Clean frontier lost bare foundation");
    require(!twin.mono&&twin.values[fmEnvelopeAmount]>1&&twin.values[filterAttack]>.4&&twin.values[filterAmount]==0&&twin.values[fmRoute1Source]==3&&twin.values[fmRoute1Target]==0&&twin.values[fmRoute1Amount]>.7,"Twin-envelope frontier lost independent depth contour");
    require(!exposed.mono&&exposed.values[fmMix]>.8&&exposed.values[fmRatio]==5&&exposed.values[fmShape]==1,"Operator frontier lost exposed upper partial");
    require(flutter.mono&&flutter.values[ampLfoOn]>.5&&flutter.values[ampLfoSync]>.5&&flutter.values[ampLfoDivision]==23&&flutter.values[cutoffLfoWave]==2&&flutter.values[cutoffLfoSync]<.5,"Fast AM frontier lost fine clock/free ramp");
    require(filterMetal.values[filterFmDepth]==2.5&&filterMetal.values[fm]<.1&&filterMetal.values[stress]<.02,"Filter-side frontier became ordinary oscillator drive");
    require(restrain.mono&&restrain.values[fmRoute1Source]==4&&restrain.values[fmRoute1Target]==0&&restrain.values[fmRoute1Amount]==-2.4&&restrain.values[glideTarget]==1&&restrain.values[legato]>.5,"Activity frontier lost negative depth route/modulator smear");
    const auto tine=builtInIndex("review:cutoff-alloy:r2"),wood=builtInIndex("review:rosewood-tap:r1"),organ=builtInIndex("review:clear-stops:r1"),brass=builtInIndex("review:copper-lift:r1"),slap=builtInIndex("review:pocket-slap:r1");
    require(tine==39&&wood==40&&organ==41&&brass==42&&slap==43,"Archetype/rename entries displaced earlier programs");
    const auto& renamedKeys=builtInPresets()[static_cast<size_t>(tine)].patch;
    require(renamedKeys.name=="Review / Physical materials / Tine Keys r2"&&renamedKeys.mono==filterMetal.mono&&renamedKeys.chord==filterMetal.chord,"Tine rename lost identity/articulation");
    for(size_t i=0;i<parameterCount;++i)require(SoundStudies::samePitchValue(renamedKeys.values[i],filterMetal.values[i]),"Tine name-only continuation changed sound conditions");
    const auto& mallet=builtInPresets()[static_cast<size_t>(wood)].patch;const auto& stops=builtInPresets()[static_cast<size_t>(organ)].patch;const auto& horn=builtInPresets()[static_cast<size_t>(brass)].patch;const auto& pop=builtInPresets()[static_cast<size_t>(slap)].patch;
    require(!mallet.mono&&mallet.values[sustain]==0&&mallet.values[fmRatio]==3&&mallet.values[fmMix]==0&&mallet.values[autobendOn]<.5&&mallet.values[fmDecay]<.04,"Wooden archetype lost damped unbent harmonic strike");
    require(!stops.mono&&stops.values[sustain]==1&&stops.values[fmEnvelopeAmount]==0&&stops.values[spaceMix]==0&&stops.values[fmRatio]==2&&stops.values[fmMix]>.3,"Organ archetype lost dry even harmonic sustain");
    require(!horn.mono&&horn.values[fmRatio]==1&&horn.values[fmAttack]>.03&&horn.values[filterAttack]>.04&&horn.values[sustain]>.7,"Brass archetype lost rising harmonic poly contour");
    require(pop.mono&&pop.values[glideOn]<.5&&pop.values[autobendOn]<.5&&pop.values[spaceMix]==0&&pop.values[fmMix]==0&&pop.values[fmDecay]<.04&&pop.values[fm]<.2,"Slap archetype lost clear dry fundamental/short index pop");
    for(const auto& lane:lfoParameters)for(const auto* patch:{&mallet,&stops,&horn,&pop})require(patch->values[lane.enabled]<.5,"Basic archetypes acquired unrelated cyclic motion");
    const auto floor=builtInIndex("review:floor-pulse:r1"),rivet=builtInIndex("review:rivet-kick:r1"),toms=builtInIndex("review:bowl-toms:r1");
    require(floor==44&&rivet==45&&toms==46,"Kick/tom studies displaced earlier programs");
    const auto& roundKick=builtInPresets()[static_cast<size_t>(floor)].patch;const auto& hardKick=builtInPresets()[static_cast<size_t>(rivet)].patch;const auto& tunedToms=builtInPresets()[static_cast<size_t>(toms)].patch;
    require(roundKick.mono&&roundKick.values[coupling]<.03&&roundKick.values[stress]==0&&roundKick.values[fm]<.02&&roundKick.values[spaceMix]==0&&roundKick.values[decay]>.3&&roundKick.values[pitch]==-3&&roundKick.values[autobendDepth]==24&&roundKick.values[autobendTarget]==2,"Rounded kick lost bare low body/fast downward entrance");
    require(hardKick.mono&&hardKick.values[coupling]>.3&&hardKick.values[fmEnvelopeAmount]>2.5&&hardKick.values[fmDecay]<.02&&hardKick.values[decay]<.2&&hardKick.values[spaceMix]==0&&hardKick.values[autobendTarget]==0&&hardKick.values[autobendDepth]==30,"Hard kick lost compact harmonic front/carrier-only drop");
    require(!tunedToms.mono&&tunedToms.values[decay]>.6&&tunedToms.values[release]>.25&&tunedToms.values[fmRatio]>1.4&&tunedToms.values[fmRatio]<1.5&&tunedToms.values[autobendTime]>.06&&tunedToms.values[autobendDepth]==4.5&&tunedToms.values[spaceMix]<.04,"Tom study lost longer lightly inharmonic poly body/gentle relaxation");
    for(const auto* patch:{&roundKick,&hardKick,&tunedToms})
    {
        require(!patch->chord&&patch->values[sustain]==0&&patch->values[autobendOn]>.5&&patch->values[autobendPhrase]<.5&&patch->values[glideOn]<.5&&patch->values[legato]<.5&&patch->values[fmMix]==0,"Percussion lost retriggered/zero-sustain articulation");
        for(const auto& lane:lfoParameters)require(patch->values[lane.enabled]<.5,"Kick/tom study acquired unrelated cyclic motion");
    }
    const auto tomsR2=builtInIndex("review:bowl-toms:r2");require(tomsR2==47,"Tom r2 displaced an earlier program");
    const auto& drumToms=builtInPresets()[static_cast<size_t>(tomsR2)].patch;
    require(drumToms.mono==tunedToms.mono&&drumToms.chord==tunedToms.chord&&drumToms.values[output]==tunedToms.values[output]&&drumToms.values[pitch]==tunedToms.values[pitch],"Tom refinement changed polyphony/output/key mapping");
    require(drumToms.values[decay]==.24&&drumToms.values[release]==.09&&drumToms.values[spaceMix]==0&&drumToms.values[fm]<.03&&drumToms.values[fmRatio]==2&&drumToms.values[fmShape]==0&&drumToms.values[fmEnvelopeAmount]==2.8&&drumToms.values[fmDecay]==.01&&drumToms.values[cutoff]>6000&&drumToms.values[autobendDepth]==12&&drumToms.values[autobendTime]==.018,"Tom refinement lost short dry body/brief harmonic strike/faster pitch entrance");
    for(const auto& lane:lfoParameters)require(drumToms.values[lane.enabled]<.5,"Tom refinement acquired unrelated cyclic motion");
    const auto lift=builtInIndex("review:pressure-lift:r1"),sink=builtInIndex("review:sinkhole:r1"),shred=builtInIndex("review:shred-relay:r1");
    require(lift==48&&sink==49&&shred==50,"Transition studies displaced earlier programs");
    const auto& rising=builtInPresets()[static_cast<size_t>(lift)].patch;const auto& falling=builtInPresets()[static_cast<size_t>(sink)].patch;const auto& breakup=builtInPresets()[static_cast<size_t>(shred)].patch;
    require(rising.mono&&rising.values[attack]>2.5&&rising.values[filterAttack]>3&&rising.values[fmAttack]>3&&rising.values[autobendDepth]==-24&&rising.values[autobendTime]==4&&rising.values[release]<.15,"Rising transition lost slow opening/upward entrance/short handoff");
    require(falling.mono&&falling.values[attack]<.003&&falling.values[sustain]==0&&falling.values[autobendDepth]==24&&falling.values[autobendTime]==1.2&&falling.values[spaceMix]==.3&&falling.values[spaceSize]==.8,"Falling transition lost impact/downward sweep/spacious tail");
    require(breakup.mono&&breakup.values[spaceMix]==0&&breakup.values[release]<.08&&breakup.values[fmAttack]>2&&breakup.values[fmEnvelopeAmount]>2&&breakup.values[filterAmount]<0&&breakup.values[fmRoute1Source]==3&&breakup.values[fmRoute1Target]==1&&breakup.values[ampLfoOn]>.5&&breakup.values[ampLfoSync]>.5&&breakup.values[ampLfoWave]==0&&breakup.values[ampLfoDivision]==15,"Breakup transition lost progressive timbre/steady sine clock/dry handoff");
    for(const auto* patch:{&rising,&falling,&breakup})require(!patch->chord&&patch->values[glideOn]<.5&&patch->values[legato]<.5&&patch->values[autobendPhrase]<.5,"Transitions acquired unrelated glide/phrase ownership");
    const auto veil=builtInIndex("review:reverse-veil:r1"),cut=builtInIndex("review:spectral-cut:r1"),flash=builtInIndex("review:flash-room:r1");
    require(veil==51&&cut==52&&flash==53,"Non-pitch candidates displaced earlier programs");
    for(const auto index:{veil,cut,flash})
    {
        const auto& patch=builtInPresets()[static_cast<size_t>(index)].patch;
        require(patch.mono&&!patch.chord&&patch.values[pitch]==0&&patch.values[coupling]==0&&patch.values[stress]==0&&patch.values[glideOn]==0&&patch.values[autobendOn]==0&&patch.values[autobendDepth]==0&&patch.values[legato]==0&&patch.values[fmRatio]==4&&patch.values[fm]==1.4&&patch.values[fmEnvelopeAmount]==0&&patch.values[filterFmDepth]==0,"Non-pitch source acquired pitch/index/network movement");
        for(const auto& lane:lfoParameters)require(patch.values[lane.enabled]==0&&patch.values[lane.depth]==0,"Non-pitch source acquired LFO movement");
        for(const auto& route:fmRoutes)require(patch.values[route.source]==0&&patch.values[route.amount]==0,"Non-pitch source acquired a dynamic FM route");
    }
    const auto& levelBuild=builtInPresets()[static_cast<size_t>(veil)].patch;const auto& shutdown=builtInPresets()[static_cast<size_t>(cut)].patch;const auto& room=builtInPresets()[static_cast<size_t>(flash)].patch;
    require(levelBuild.values[attack]==3.4&&levelBuild.values[filterAttack]==3.5&&levelBuild.values[filterAmount]==4.5&&levelBuild.values[spaceMix]==0,"Reverse-like gesture lost level/Tone build");
    require(shutdown.values[filterAttack]==3.4&&shutdown.values[filterAmount]==-5.7&&shutdown.values[spaceMix]==0,"Spectral shutdown lost filter-only descent");
    require(room.values[decay]==.065&&room.values[sustain]==0&&room.values[spaceMix]==.55&&room.values[spaceSize]==.9&&room.values[output]==0,"Flash/room lost brief excitation/fixed spatial tail");
    const auto classicKeys=builtInIndex("review:classic-tines:r1"),classicBass=builtInIndex("review:classic-bass:r1"),classicBell=builtInIndex("review:classic-tubes:r1");
    require(classicKeys==54&&classicBass==55&&classicBell==56,"Classic approximations displaced earlier programs");
    const auto& ep=builtInPresets()[static_cast<size_t>(classicKeys)].patch;const auto& bass=builtInPresets()[static_cast<size_t>(classicBass)].patch;const auto& tube=builtInPresets()[static_cast<size_t>(classicBell)].patch;
    require(!ep.mono&&ep.values[fmRatio]==1&&ep.values[decay]>1.4&&ep.values[fmRoute1Source]==3&&ep.values[fmRoute1Target]==2&&ep.values[filterDecay]==.025&&ep.values[fmDecay]>.6,"EP1 approximation lost warm1:1 body/independent brief shape strike");
    require(bass.mono&&bass.values[pitch]==0&&bass.values[fmRatio]==1&&bass.values[fmEnvelopeAmount]>2&&bass.values[fmDecay]<.05&&bass.values[coupling]<.06,"BASS1 approximation lost normal played root/brief harmonic front");
    require(!tube.mono&&tube.values[fmRatio]==3.5&&tube.values[decay]>1.7&&tube.values[fmDecay]==1.5&&tube.values[release]==1.6&&tube.values[fmRoute1Source]==0,"TUB approximation lost3.5ratio/long dry ring");
    for(const auto* patch:{&ep,&bass,&tube})
    {
        require(!patch->chord&&patch->values[spaceMix]==0&&patch->values[autobendOn]==0&&patch->values[glideOn]==0&&patch->values[legato]==0&&patch->values[stress]==0&&patch->values[filterAmount]==0,"Classic approximations acquired pitch/room/grit/filter-sweep decoration");
        for(const auto& lane:lfoParameters)require(patch->values[lane.enabled]==0,"Classic approximation acquired cyclic motion");
    }
    const auto voiceIndex=builtInIndex("review:classic-voice:r1"),orchIndex=builtInIndex("review:orch-chime:r1"),kotoIndex=builtInIndex("review:classic-koto:r1");
    require(voiceIndex==57&&orchIndex==58&&kotoIndex==59,"Requested ROM studies displaced earlier programs");
    const auto& choir=builtInPresets()[static_cast<size_t>(voiceIndex)].patch;const auto& orch=builtInPresets()[static_cast<size_t>(orchIndex)].patch;const auto& koto=builtInPresets()[static_cast<size_t>(kotoIndex)].patch;
    require(choir.values[attack]==.35&&choir.values[fmRatio]==1.01&&choir.values[fmMix]==.12&&choir.values[pitchLfoWave]==1&&choir.values[fmLfoOn]==1,"VOICE study lost poly vowel entrance/near-unison texture/triangle modulation");
    require(orch.values[pitch]==-12&&orch.values[fmRatio]==6.28&&orch.values[sustain]==.35&&orch.values[filterAttack]==.55&&orch.values[fmRoute1Source]==3&&orch.values[fmRoute1Target]==0&&orch.values[fmRoute1Amount]==.4,"Orchestral chime lost half-carrier mapping/6.28 ratio/two-contour body");
    require(koto.values[fmRatio]==4&&koto.values[attack]==.001&&koto.values[sustain]==0&&koto.values[fmDecay]==.045&&koto.values[pitchLfoWave]==0&&koto.values[pitchLfoDepth]==.05&&koto.values[ampLfoOn]==0,"Koto lost4:1 pluck/reduced undelayed sine vibrato or acquired spurious tremolo");
    for(const auto* patch:{&choir,&orch,&koto})require(!patch->mono&&!patch->chord&&patch->values[spaceMix]==0&&patch->values[stress]==0&&patch->values[autobendOn]==0&&patch->values[glideOn]==0&&patch->values[legato]==0&&patch->values[pitchLfoOn]==1&&patch->values[pitchLfoSync]==0,"Requested classic lost dry poly/free-vibrato conditions");
    const auto whistle=builtInIndex("review:whistle-stem:r1"),jaw=builtInIndex("review:jaw-twang:r1"),vowel=builtInIndex("review:vowel-pick:r1");
    require(whistle==60&&jaw==61&&vowel==62,"FS1R studies displaced historical programs");
    const auto& w=builtInPresets()[static_cast<size_t>(whistle)].patch;const auto& j=builtInPresets()[static_cast<size_t>(jaw)].patch;const auto& v=builtInPresets()[static_cast<size_t>(vowel)].patch;
    require(w.values[pitch]==12&&w.values[fmRatio]==1.003&&w.values[fmMix]==.45&&w.values[attack]==.11&&w.values[pitchLfoOn]==1,"Whistle lost source2xregister/near-unison tap/soft entrance");
    require(j.values[pitch]==-12&&j.values[fmRatio]==1&&j.values[resonance]==.72&&j.values[filterKeyTracking]==0&&j.values[filterAmount]==1.4&&j.values[sustain]==0,"Jaw study lost low glottal root/single fixed-register resonance/decay");
    require(v.values[pitch]==0&&v.values[fmRatio]==2&&v.values[fmShape]==1&&v.values[decay]==1.25&&v.values[fmRoute1Source]==3&&v.values[fmRoute1Target]==0&&v.values[fmRoute1Amount]==.4,"Vowel study lost normal root/odd-harmonic surrogate/dual contour");
    for(const auto* patch:{&w,&j,&v})require(!patch->mono&&!patch->chord&&patch->values[spaceMix]==0&&patch->values[stress]==0&&patch->values[autobendOn]==0&&patch->values[glideOn]==0&&patch->values[legato]==0,"FS1R study acquired unsupported bend/noise/Space/mono decoration");
    struct ReentrantAutomation final:juce::AudioProcessorParameter::Listener
    {
        juce::RangedAudioParameter* parameter=nullptr;bool busy=false;
        void parameterValueChanged(int,float) override{if(!busy){busy=true;parameter->setValueNotifyingHost(parameter->convertTo0to1(.9f));busy=false;}}
        void parameterGestureChanged(int,bool) override{}
    } automation;
    automation.parameter=p.getState().getParameter("autobend_depth");automation.parameter->addListener(&automation);
    p.getInstrumentSession().set(autobendDepth,.7);automation.parameter->removeListener(&automation);
    require(std::abs(p.getInstrumentSession().read().values[autobendDepth]-.9)<1e-5,"Own float-echo guard suppressed genuine reentrant automation");
}
void transitionTransport()
{
    constexpr int samples=12000;
    const auto render=[](const Patch& patch,double bpm,bool position,bool playing)
    {
        RenderHarness h;h.session.apply(patch);h.renderer.prepare(48000);std::vector<float> result(samples);
        for(int offset=0;offset<samples;offset+=64)
        {
            const auto count=std::min(64,samples-offset);juce::AudioBuffer<float> audio(2,count);juce::MidiBuffer midi;
            if(offset==0)midi.addEvent(juce::MidiMessage::noteOn(1,48,.8f),0);
            DSP::Transport transport{bpm,.0625+(playing?offset*bpm/(60*48000):0),4,position,position,playing};
            h.renderer.process(audio,midi,false,transport);
            for(int i=0;i<count;++i){const auto value=audio.getSample(0,i);require(std::isfinite(value),"Transition transport probe became non-finite");result[static_cast<size_t>(offset+i)]=value;}
        }
        return std::pair{std::move(result),h.session.live.lfoPhases[3].load()};
    };
    const auto& shred=builtInPresets()[static_cast<size_t>(builtInIndex("review:shred-relay:r1"))].patch;
    for(const auto bpm:{90.0,150.0})
    {
        const auto stopped=render(shred,bpm,true,false),playing=render(shred,bpm,true,true);
        require(std::abs(stopped.second-.25)<1e-6,"Shred stopped clock failed to freeze at host position");
        const auto cycles=(.0625+(samples-1)*bpm/(60*48000))/.25;
        require(std::abs(playing.second-(cycles-std::floor(cycles)))<1e-6,"Shred playing clock failed to follow host tempo/PPQ");
        require(stopped.first!=playing.first,"Shred transport requirement is not exercised");
    }
    const auto fallback=render(shred,120,false,false);const auto expected=(samples-1)*120.0/(60*48000*.25);
    require(std::abs(fallback.second-(expected-std::floor(expected)))<1e-6,"Shred missing-position fallback stopped advancing");
    for(const auto* id:{"review:pressure-lift:r1","review:sinkhole:r1"})
    {
        const auto& patch=builtInPresets()[static_cast<size_t>(builtInIndex(id))].patch;
        require(render(patch,90,true,false).first==render(patch,150,true,true).first,"Seconds-based transition unexpectedly followed transport");
    }
}
}
void runReviewRegressionTests(){midiChannelModes();interruptedRecall();accessibleUnits();compareReplacement();embeddedReviewPrograms();transitionTransport();std::cout<<"Code-review MIDI, recall, accessibility/text, Compare, embedded programs and transition transport regressions passed\n";}
