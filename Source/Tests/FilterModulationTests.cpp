#include "FilterModulationTests.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include "Instrument/SupportingControls.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride;
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double a,double b,double tolerance,const char* message){require(std::abs(a-b)<=tolerance,message);}
juce::File library(){return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-filter-mod-"+juce::Uuid().toString());}
void math()
{
    for(const auto rate:{44100,48000,96000})
    {
        for(const auto tracking:{0.0,.5,1.0})
        {
            const auto a=DSP::modulatedCutoff(1000,DSP::filterNoteOctaves(60,tracking,0,0),0,0,rate);
            const auto b=DSP::modulatedCutoff(1000,DSP::filterNoteOctaves(72,tracking,0,0),0,0,rate);
            near(b/a,std::exp2(tracking),1e-12,"Key tracking does not follow the requested octave ratio");
            near(DSP::modulatedCutoff(1000,DSP::filterNoteOctaves(48,tracking,0,0),0,0,rate),1000/std::exp2(tracking),1e-10,"Key tracking below MIDI 60 is wrong");
        }
        for(const auto velocity:{0.0,.25,.5,1.0})for(const auto amount:{-3.0,0.0,3.0})
            near(DSP::modulatedCutoff(1000,DSP::filterNoteOctaves(60,0,velocity,amount),0,0,rate),1000*std::exp2(velocity*amount),1e-10,"Velocity polarity/scaling is wrong");
        // Note + velocity + ADSR + LFO + Pitch + Tone are octave sums, not Hz sums.
        const auto control=DSP::filterNoteOctaves(72,1,.5,2)+3*.5-.75+3.0/12;
        near(DSP::modulatedCutoff(1000,control,0,0,rate),8000,1e-10,"Control sources did not sum in logarithmic frequency space");
        for(const auto sample:{-1.0,1.0,-1.0,0.0,1.0})
            near(DSP::modulatedCutoff(1000,1,1,sample,rate),2000*std::exp2(sample),1e-10,"Audio-rate FM was smoothed or combined in the wrong units");
        near(DSP::modulatedCutoff(1000,0,6,100,rate),rate*.42,1e-10,"Upper FM cutoff bound failed");
        near(DSP::modulatedCutoff(40,-20,6,-100,rate),10,0,"Lower combined cutoff bound failed");
        near(DSP::modulatedCutoff(18000,1e100,6,1,rate),rate*.42,1e-10,"Extreme finite modulation overflowed the clamp");
    }
    near(DSP::modulatedCutoff(1000,0,0,0,10),4.2,1e-12,"Very low sample rate inverted filter bounds");
}
struct Harness
{
    InstrumentSession session{library()};InstrumentRenderer renderer{session};
    Harness(bool mono=false)
    {
        auto patch=factoryPatches()[1];patch.mono=mono;patch.values[glideOn]=0;patch.values[legato]=1;
        patch.values[cutoff]=1000;patch.values[filterKeyTracking]=1;patch.values[filterVelocity]=2;patch.values[filterRelease]=.02;
        session.apply(patch);renderer.prepare(48000);
    }
    void tick(int samples=1,juce::MidiMessage message={})
    {
        juce::AudioBuffer<float> audio(2,samples);juce::MidiBuffer midi;if(message.getRawDataSize()>0)midi.addEvent(message,0);
        renderer.process(audio,midi);require(std::isfinite(audio.getMagnitude(0,samples)),"Note-owned filter modulation produced non-finite audio");
    }
    void on(int channel,int note,int velocity){tick(1,juce::MidiMessage::noteOn(channel,note,static_cast<juce::uint8>(velocity)));}
    void off(int channel,int note){tick(1,juce::MidiMessage::noteOff(channel,note));}
    void check(int note,int velocity)
    {require(renderer.filterNoteNumber()==note,"Shared filter selected the wrong note owner");near(renderer.filterNoteVelocity(),velocity/127.0,1e-7,"Shared filter selected the wrong note velocity");}
};
void ownership()
{
    for(const auto mono:{false,true})
    {
        Harness h(mono);h.on(1,60,32);h.on(16,72,100);h.check(72,100);h.off(16,72);h.check(60,32);
        h.on(1,60,64);h.off(1,60);h.check(60,64);h.off(1,60);h.check(60,64);
        h.tick(1400);near(h.renderer.filterCutoffFrequency(),1000*std::exp2(2*static_cast<double>(static_cast<float>(64)/127)),.001,"Last note's velocity was not retained through release");
        h.on(1,60,16);h.on(1,72,127);h.tick(1,juce::MidiMessage::controllerEvent(1,64,127));h.off(1,72);h.check(60,16);
        h.off(1,60);h.check(72,127);h.tick(1,juce::MidiMessage::controllerEvent(1,64,0));h.check(72,127);
        h.on(1,48,32);h.on(16,67,100);h.tick(1,juce::MidiMessage::allNotesOff(16));h.check(48,32);
        h.tick(1,juce::MidiMessage::allSoundOff(1));h.check(48,32);h.renderer.reset();h.tick();h.check(60,0);
    }
    Harness capture;capture.on(16,60,32);capture.on(1,67,96);
    capture.session.keyboard.noteOn(16,60,.95f);capture.tick();capture.check(67,96); // Capture is not a new press.
    capture.off(1,67);capture.off(16,60);capture.check(60,32); // Original velocity, not Hold's audition velocity.
    capture.session.detachEditor();capture.tick();capture.check(60,32);
    Harness overflow;overflow.on(1,60,32);overflow.session.keyboard.noteOn(15,72,.9f);overflow.tick();
    for(int i=0;i<3000;++i)overflow.session.editorMidi.push(0x9e,67,96);overflow.tick();overflow.check(60,32);
}
void smoothingAndComposition()
{
    Harness h;h.session.set(filterKeyTracking,0);h.session.set(filterVelocity,0);h.tick(1400);
    h.session.set(cutoff,4000);h.tick(600);near(h.renderer.filterCutoffFrequency(),2000,.1,"Base cutoff ramp is not logarithmic");h.tick(600);near(h.renderer.filterCutoffFrequency(),4000,.001,"Base cutoff ramp failed to settle");
    h.session.set(cutoff,1000);h.session.set(filterKeyTracking,1);h.session.set(filterVelocity,2);
    h.session.set(filterAmount,2);h.session.set(filterAttack,.001);h.session.set(filterDecay,.01);h.session.set(filterSustain,1);
    h.on(1,72,64);h.tick(2400);
    near(h.renderer.filterCutoffFrequency(),1000*std::exp2(1+2*h.renderer.filterNoteVelocity()+2*h.renderer.filterEnvelopeLevel()),.001,"Note, velocity and existing ADSR did not compose additively");
    require(h.renderer.filterEnvelopeStage()!=DSP::FilterEnvelope::Stage::idle,"Filter modulation bypassed the existing envelope");
    const auto level=h.renderer.filterEnvelopeLevel();h.session.set(filterVelocity,-2);h.tick(600);
    require(h.renderer.filterEnvelopeLevel()>=level-.00001,"Editing modulation retriggered/reset the existing filter envelope");
    h.tick(600);near(h.renderer.filterCutoffFrequency(),1000*std::exp2(1-2*h.renderer.filterNoteVelocity()+2*h.renderer.filterEnvelopeLevel()),.001,"Signed velocity automation failed to settle");
    h.session.set(cutoffLfoOn,1);h.session.set(cutoffLfoWave,3);h.session.set(cutoffLfoRate,.01);h.session.set(cutoffLfoDepth,-1);
    h.session.set(autobendOn,1);h.session.set(autobendTarget,4);h.session.set(autobendDepth,12);h.on(1,72,64);h.tick(4800);
    const auto pitchTone=h.renderer.pitchSnapshot(72).autobendSemitones/12;
    near(h.renderer.filterCutoffFrequency(),1000*std::exp2(1-2*h.renderer.filterNoteVelocity()+2*h.renderer.filterEnvelopeLevel()-1+pitchTone),.001,
        "Renderer failed to sum note/velocity, ADSR, cutoff LFO and Pitch + Tone in octave space");
}
void audioRateTap()
{
    for(const auto rate:{44100,48000,96000})
    {
        InstrumentSession session(library());auto patch=factoryPatches()[1];patch.values[cutoff]=1000;patch.values[attack]=.001;patch.values[filterFmDepth]=6;
        patch.values[spaceMix]=.65;patch.values[output]=-24; // These must not affect the FM source.
        patch.values[ampLfoOn]=1;patch.values[ampLfoDepth]=1;patch.values[ampLfoWave]=3;patch.values[ampLfoRate]=20;
        session.apply(patch);InstrumentRenderer renderer(session);renderer.prepare(rate);
        SoundStudies::Engine oscillator;oscillator.setTreatment(SoundStudies::Treatment::expressiveNetwork);
        oscillator.setPitchMotion({});oscillator.setFmIndex(patch.values[fm]);
        oscillator.setAmplitudeEnvelope(patch.values[attack],patch.values[decay],static_cast<float>(patch.values[sustain]),patch.values[release]);
        oscillator.setNetworkDimensions(static_cast<float>(patch.values[coupling]),static_cast<float>(patch.values[stress]),patch.values[response]);
        oscillator.prepare(rate);oscillator.beginNoteGroup();const auto note=juce::MidiMessage::noteOn(1,84,static_cast<juce::uint8>(100));
        oscillator.noteOn(84,note.getFloatVelocity(),SoundStudies::Articulation::pad,false);
        juce::AudioBuffer<float> audio(2,1);juce::MidiBuffer midi;midi.addEvent(note,0);double minimum=1e9,maximum=0;
        for(int sample=0;sample<1024;++sample)
        {
            float l=0,r=0;oscillator.render(&l,&r,1);renderer.process(audio,midi,false);midi.clear();
            const auto expected=DSP::modulatedCutoff(1000,0,6,(static_cast<double>(l)+r)*.625,rate);
            near(renderer.filterCutoffFrequency(),expected,1e-9,"Filter FM did not use the instantaneous pre-filter oscillator sample");
            minimum=std::min(minimum,expected);maximum=std::max(maximum,expected);
        }
        require(maximum-minimum>50,"Audio-rate tap did not vary cutoff audibly");near(session.read().values[cutoff],1000,0,"Filter FM overwrote base cutoff");
    }
}
std::vector<float> render(int features,int block,int rate=48000,bool extreme=false)
{
    InstrumentSession session(library());auto patch=factoryPatches()[1];patch.values[cutoff]=extreme?18000:1200;
    patch.values[resonance]=extreme?.85:.3;patch.values[spaceMix]=0;
    patch.values[filterKeyTracking]=(features&1)?1:0;patch.values[filterVelocity]=(features&2)?(extreme?-6:2):0;
    patch.values[filterFmDepth]=(features&4)?6:0;patch.values[filterAmount]=(features&8)?(extreme?6:3):0;
    patch.values[filterAttack]=.001;patch.values[filterDecay]=.01;patch.values[filterSustain]=.5;patch.values[filterRelease]=.02;
    patch.values[cutoffLfoOn]=(features&16)?1:0;patch.values[cutoffLfoWave]=3;patch.values[cutoffLfoRate]=20;patch.values[cutoffLfoDepth]=extreme?-6:2;
    if(features&32){patch.values[autobendOn]=1;patch.values[autobendTarget]=4;patch.values[autobendDepth]=extreme?36:12;}
    session.apply(patch);InstrumentRenderer renderer(session);renderer.prepare(rate);juce::MidiBuffer events;
    events.addEvent(juce::MidiMessage::noteOn(1,48,static_cast<juce::uint8>(32)),13);events.addEvent(juce::MidiMessage::noteOn(16,72,static_cast<juce::uint8>(100)),1013);
    if(extreme)for(int i=0;i<10;++i)events.addEvent(juce::MidiMessage::noteOn(1,84+i*4,static_cast<juce::uint8>(127)),2013);
    events.addEvent(juce::MidiMessage::allNotesOff(1),10013);events.addEvent(juce::MidiMessage::allNotesOff(16),12013);
    std::vector<float> result(20000);
    for(int offset=0;offset<20000;offset+=block)
    {
        const auto size=std::min(block,20000-offset);juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;midi.addEvents(events,offset,size,-offset);renderer.process(audio,midi,false);
        require(renderer.filterCutoffFrequency()>=10&&renderer.filterCutoffFrequency()<=rate*.42,"Integrated cutoff escaped valid bounds");
        for(int c=0;c<2;++c)for(int i=0;i<size;++i)require(std::isfinite(audio.getSample(c,i)),"Extreme combined modulation produced non-finite audio");
        for(int i=0;i<size;++i)result[static_cast<size_t>(offset+i)]=audio.getSample(0,i);
    }
    require(session.live.faults.load()==0&&renderer.filterEnvelopeStage()==DSP::FilterEnvelope::Stage::idle,"Combined modulation caused faults or retained filter release");
    for(size_t i=0;i<parameterCount;++i)near(session.read().values[i],patch.values[i],1e-12,"Filter modulation rewrote stored conditions");
    return result;
}
void sound()
{
    const auto baseline=render(0,64);
    for(const auto features:{1,2,4,7,63})
    {
        const auto audio=render(features,64),other=render(features,257);double difference=0,error=0;
        for(size_t i=0;i<audio.size();++i){difference+=std::abs(static_cast<double>(audio[i])-baseline[i]);error=std::max(error,std::abs(static_cast<double>(audio[i])-other[i]));}
        require(difference>.01,"A filter modulation destination was not audible");require(error<1e-5,"Filter modulation timing depends on host block size");
        require(audio==render(features,64),"Filter modulation render is not deterministic");
    }
    for(const auto rate:{44100,48000,96000})render(63,64,rate,true);
}
template<class T>T* child(juce::Component& root,const juce::String& name)
{for(auto* c:root.getChildren()){if(auto* v=dynamic_cast<T*>(c);v&&c->getName()==name)return v;if(auto* v=child<T>(*c,name))return v;}return nullptr;}
void stateAndUi()
{
    const auto directory=library();NitrideAudioProcessor p(directory);auto& s=p.getInstrumentSession();
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    for(const auto parameter:{filterKeyTracking,filterVelocity,filterFmDepth})
    {
        const auto& d=hostParameters[static_cast<size_t>(parameter)];auto* host=p.getState().getParameter(d.id);
        require(host&&p.getParameters()[hostIndex(parameter)]==host&&host->getVersionHint()==5,"Filter modulation host metadata/mapping is incomplete");
        auto* control=child<UI::ValueControl>(*editor,d.name);require(control&&control->isShowing(),"Native Tone modulation control missing");
        require(control->getAccessibilityHandler()&&control->getAccessibilityHandler()->getValueInterface(),"Tone modulation accessibility missing");
        control->getAccessibilityHandler()->getValueInterface()->setValue(parameter==filterKeyTracking?1:3);
    }
    juce::MemoryBlock state;p.getStateInformation(state);NitrideAudioProcessor restored(directory);restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));auto& r=restored.getInstrumentSession();
    near(r.read().values[filterKeyTracking],1,1e-6,"State lost key tracking");near(r.read().values[filterVelocity],3,1e-6,"State lost velocity sensitivity");near(r.read().values[filterFmDepth],3,1e-6,"State lost Filter FM depth");
    auto* cutoffParameter=restored.getState().getParameter("cutoff");cutoffParameter->setValueNotifyingHost(cutoffParameter->convertTo0to1(700));r.undo();
    near(r.read().values[filterFmDepth],0,0,"High-index Filter FM undo failed");near(r.read().values[cutoff],700,.001,"Filter FM undo reverted unrelated automation");r.redo();near(r.read().values[filterFmDepth],3,1e-6,"Filter FM redo failed");
    s.setComparing(true);near(s.audioValue(filterFmDepth),0,0,"Compare failed to audition neutral filter modulation");near(s.read().values[filterFmDepth],3,1e-6,"Compare destroyed Filter FM edits");s.setComparing(false);
    require(s.keep("Filter modulation test").wasOk(),"Keep failed with filter modulation");
    const auto file=directory.getChildFile("filter-mod.nitridepreset");require(s.exportPreset(file).wasOk(),"Filter modulation export failed");const auto preset=PresetStore::importFile(file);
    require(preset.has_value(),"Preset round trip lost filter modulation");near(preset->patch.values[filterKeyTracking],1,0,"Preset lost key tracking");
    near(preset->patch.values[filterVelocity],3,0,"Preset lost velocity sensitivity");near(preset->patch.values[filterFmDepth],3,0,"Preset lost Filter FM depth");
    auto malformed=PresetStore::encode(*preset);malformed["values"].getDynamicObject()->removeProperty("filter_fm_depth");require(!PresetStore::decode(malformed),"Preset accepted a missing Filter FM control");
    auto* velocity=child<UI::ValueControl>(*editor,"Filter velocity sensitivity");auto* hostVelocity=p.getState().getParameter("filter_velocity");hostVelocity->setValueNotifyingHost(hostVelocity->convertTo0to1(-2));
    for(int i=0;i<10;++i)juce::MessageManager::getInstance()->runDispatchLoopUntil(20);near(velocity->getValue(),-2,.001,"Filter velocity automation did not update native UI");
    auto* depth=child<UI::ValueControl>(*editor,"Filter FM depth");depth->onBegin();p.getStateInformation(state);p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));depth->onChange(6);depth->onEnd();
    near(s.read().values[filterFmDepth],3,1e-6,"Stale Tone gesture overwrote recalled Filter FM depth");editor.reset();directory.deleteRecursively();
}
}
void runFilterModulationTests(){math();ownership();smoothingAndComposition();audioRateTap();sound();stateAndUi();std::cout<<"Filter tracking, velocity, instantaneous oscillator FM, logarithmic composition, ownership, bounds, UI and state/history passed\n";}
