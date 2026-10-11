#include "FilterEnvelopeTests.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include "Instrument/SupportingControls.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride;
using Stage=DSP::FilterEnvelope::Stage;
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
juce::File library(){return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-filter-"+juce::Uuid().toString());}
void stages()
{
    for(const auto rate:{44100,48000,96000})
    {
        DSP::FilterEnvelope envelope;envelope.prepare(rate);envelope.configure(.01,.1,.25,.02);
        envelope.noteOn();for(int i=0;i<rate/100;++i)envelope.next();
        require(std::abs(envelope.level()-1)<1e-10,"Filter attack did not reach its peak at the requested time");
        for(int i=0;i<rate/10;++i)envelope.next();
        require(std::abs(envelope.level()-(.25+.75/std::exp(1.0)))<1e-10,"Filter decay does not match the amplitude exponential shape");
        const auto before=envelope.level();envelope.noteOff();
        require(std::abs(envelope.level()-before)<1e-12,"Filter note-off jumped its level");
        for(int i=0;i<rate/100;++i)envelope.next();require(std::abs(envelope.level()-before*.5)<1e-10,"Filter release timing changed");
        const auto released=envelope.level();envelope.noteOn();require(std::abs(envelope.level()-released)<1e-12,"Filter retrigger jumped its level");
        envelope.next();const auto current=envelope.level();envelope.configure(.2,.01,.9,.5);
        require(std::abs(envelope.level()-current)<1e-12,"Filter timing automation jumped the level");envelope.noteOff();
        for(int i=0;i<rate/2+1;++i)envelope.next();require(std::abs(envelope.level())<1e-12&&envelope.currentStage()==Stage::idle,"Filter release failed to finish");
        envelope.configure(NAN,NAN,NAN,NAN);envelope.noteOn();require(std::isfinite(envelope.next()),"Invalid filter configuration became non-finite");
        envelope.reset();require(std::abs(envelope.level())<1e-12&&envelope.currentStage()==Stage::idle,"Filter reset retained its envelope");
    }
}
struct Harness
{
    InstrumentSession session{library()};InstrumentRenderer renderer{session};
    Harness(bool mono=false,bool legatoMode=false)
    {
        auto patch=factoryPatches()[0];patch.mono=mono;patch.values[legato]=legatoMode?1:0;
        patch.values[filterAttack]=.001;patch.values[filterDecay]=.01;patch.values[filterSustain]=.25;patch.values[filterRelease]=.02;
        patch.values[filterAmount]=3;session.apply(patch);renderer.prepare(48000);
    }
    void tick(int samples=1,juce::MidiMessage message={})
    {
        juce::AudioBuffer<float> audio(2,samples);juce::MidiBuffer midi;if(message.getRawDataSize()>0)midi.addEvent(message,0);
        renderer.process(audio,midi);require(std::isfinite(audio.getMagnitude(0,samples)),"Filter ownership case produced non-finite audio");
    }
    void on(int channel,int note){tick(1,juce::MidiMessage::noteOn(channel,note,.8f));}
    void off(int channel,int note){tick(1,juce::MidiMessage::noteOff(channel,note));}
    void settled(){tick(6000);}
    Stage stage()const{return renderer.filterEnvelopeStage();}
};
void ownership()
{
    Harness poly;poly.on(1,60);poly.settled();poly.on(16,64);require(poly.stage()==Stage::attack,"Poly notes failed to retrigger shared filter");
    poly.off(1,60);require(poly.stage()!=Stage::release,"A partial chord release released the filter");
    poly.off(16,64);require(poly.stage()==Stage::release,"Final host release did not release the filter");poly.tick(1000);require(poly.stage()==Stage::idle,"Released filter stayed active");
    poly.on(15,60);poly.on(15,60);poly.off(15,60);require(poly.stage()!=Stage::release,"First repeated-note release closed the shared gate");
    poly.tick(1,juce::MidiMessage::controllerEvent(15,64,127));poly.off(15,60);require(poly.stage()!=Stage::release,"Pedal did not hold filter gate");
    poly.tick(1,juce::MidiMessage::controllerEvent(15,64,0));require(poly.stage()==Stage::release,"Pedal-up did not release filter gate");
    Harness mono(true,true);mono.on(1,60);mono.settled();const auto level=mono.renderer.filterEnvelopeLevel();mono.on(1,72);
    require(mono.stage()!=Stage::attack&&std::abs(mono.renderer.filterEnvelopeLevel()-level)<.00001,"Mono legato retriggered filter envelope");
    mono.off(1,72);require(mono.stage()!=Stage::attack&&mono.stage()!=Stage::release,"Return to a held mono note restarted filter envelope");
    mono.off(1,60);require(mono.stage()==Stage::release,"Mono final release left filter gate open");
    Harness retrigger(true,false);retrigger.on(1,60);retrigger.settled();retrigger.on(1,72);require(retrigger.stage()==Stage::attack,"Non-legato mono failed to retrigger filter");
    Harness capture;capture.on(16,60);capture.settled();capture.session.keyboard.noteOn(16,60,.8f);capture.tick();
    require(capture.stage()!=Stage::attack,"Hold capture retriggered an existing filter envelope");capture.off(16,60);
    require(capture.stage()!=Stage::release,"Host release lost captured filter ownership");capture.session.detachEditor();capture.tick();
    require(capture.stage()==Stage::release,"Editor teardown retained captured filter gate");
    Harness overflow;overflow.on(1,60);overflow.session.keyboard.noteOn(15,64,.8f);overflow.tick();
    for(int i=0;i<3000;++i)overflow.session.editorMidi.push(0x9e,67,96);overflow.tick();
    require(overflow.stage()!=Stage::release,"Editor overflow released a host filter gate");overflow.off(1,60);
    require(overflow.stage()==Stage::release,"Overflow left an editor filter gate stuck");
    poly.on(1,60);poly.on(16,67);poly.tick(1,juce::MidiMessage::allNotesOff(1));require(poly.stage()!=Stage::release,"Channel panic released another channel's filter gate");
    poly.tick(1,juce::MidiMessage::allSoundOff(16));require(poly.stage()==Stage::release,"Final channel panic left filter gate open");
    poly.renderer.reset();require(poly.stage()==Stage::idle,"Render reset retained filter envelope");
}
std::vector<float> performance(double amount,int block,bool pitchTone=false)
{
    InstrumentSession session(library());auto patch=factoryPatches()[1];patch.values[cutoff]=800;patch.values[resonance]=.4;
    patch.values[filterAmount]=amount;patch.values[filterAttack]=.02;patch.values[filterDecay]=.1;patch.values[filterRelease]=.3;
    if(pitchTone){patch.values[autobendOn]=1;patch.values[autobendTarget]=4;patch.values[autobendDepth]=12;}
    session.apply(patch);InstrumentRenderer renderer(session);renderer.prepare(48000);
    juce::MidiBuffer events;events.addEvent(juce::MidiMessage::noteOn(1,60,.8f),13);events.addEvent(juce::MidiMessage::noteOn(16,67,.7f),713);
    events.addEvent(juce::MidiMessage::noteOff(1,60),3413);events.addEvent(juce::MidiMessage::noteOff(16,67),6013);
    std::vector<float> result(16000);
    for(int offset=0;offset<16000;offset+=block)
    {
        const auto size=std::min(block,16000-offset);juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;midi.addEvents(events,offset,size,-offset);
        renderer.process(audio,midi);for(int i=0;i<size;++i){require(std::isfinite(audio.getSample(0,i)),"Filter performance produced non-finite samples");result[static_cast<size_t>(offset+i)]=audio.getSample(0,i);}
    }
    return result;
}
double energy(const std::vector<float>& samples){double result=0;for(auto sample:samples)result+=sample*sample;return result;}
void sound()
{
    const auto neutral=performance(0,64),open=performance(3,64),closed=performance(-3,64);
    require(energy(open)>energy(neutral)*1.1&&energy(closed)<energy(neutral)*.9,"Filter Amount did not open/close the cutoff audibly");
    const auto other=performance(3,257);double maximum=0;for(size_t i=0;i<open.size();++i)maximum=std::max(maximum,std::abs(static_cast<double>(open[i])-other[i]));
    require(maximum<.00001,"Filter envelope rendering depends on host block size");
    require(performance(3,64)==open,"Filter performance is not deterministic");require(performance(3,64,true)!=open,"Pitch + Tone failed to combine with filter envelope");
    std::cout<<"Filter envelope energies: closed "<<energy(closed)<<", neutral "<<energy(neutral)<<", open "<<energy(open)<<"; block error "<<maximum<<'\n';
}
void extremes()
{
    for(const auto rate:{44100,48000,96000})for(const auto amount:{-6,6})
    {
        InstrumentSession session(library());auto patch=factoryPatches()[0];patch.values[filterAmount]=amount;
        patch.values[cutoff]=amount<0?40:18000;patch.values[resonance]=.85;patch.values[filterAttack]=.001;
        patch.values[filterDecay]=.01;patch.values[filterSustain]=1;patch.values[filterRelease]=.02;
        patch.values[glideOn]=1;patch.values[glideTarget]=4;patch.values[autobendOn]=1;patch.values[autobendTarget]=4;patch.values[autobendDepth]=amount<0?-36:36;
        session.apply(patch);InstrumentRenderer renderer(session);renderer.prepare(rate);juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;
        for(int i=0;i<12;++i)midi.addEvent(juce::MidiMessage::noteOn(1,48+i,.8f),0);
        for(int block=0;block<200;++block)
        {
            if(block==100)midi.addEvent(juce::MidiMessage::allNotesOff(1),13);
            renderer.process(audio,midi);midi.clear();
            for(int c=0;c<2;++c)for(int i=0;i<64;++i)require(std::isfinite(audio.getSample(c,i)),"Extreme combined filter modulation produced non-finite audio");
        }
        require(session.live.faults.load()==0&&renderer.filterEnvelopeStage()==Stage::idle,"Extreme filter release failed");
    }
}
template<class T>T* child(juce::Component& root,const juce::String& name)
{for(auto* c:root.getChildren()){if(auto* v=dynamic_cast<T*>(c);v&&c->getName()==name)return v;if(auto* v=child<T>(*c,name))return v;}return nullptr;}
void stateAndUi()
{
    const auto directory=library();NitrideAudioProcessor p(directory);auto& s=p.getInstrumentSession();
    for(const auto parameter:{filterAttack,filterDecay,filterSustain,filterRelease,filterAmount})
    {
        auto* host=p.getState().getParameter(hostParameters[static_cast<size_t>(parameter)].id);
        require(host&&p.getParameters()[hostIndex(parameter)]==host&&host->getVersionHint()==3,"Filter host parameters did not append with their stable version hint");
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    auto* graph=child<UI::EnvelopeControl>(*editor,"Filter envelope");require(graph,"Filter graph missing");
    graph->onBegin();graph->onChange({.2,.4,.3,.8});graph->onEnd();require(std::abs(s.read().values[attack]-factoryPatches()[0].values[attack])<1e-12,"Filter ADSR changed amplitude ADSR");
    auto* amount=child<UI::ValueControl>(*editor,"Filter envelope amount");require(amount,"Filter amount control missing");
    require(amount->getAccessibilityHandler()&&amount->getAccessibilityHandler()->getValueInterface(),"Filter amount accessibility missing");
    amount->getAccessibilityHandler()->getValueInterface()->setValue(-4);
    juce::MemoryBlock state;p.getStateInformation(state);NitrideAudioProcessor restored(directory);restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));auto& r=restored.getInstrumentSession();
    require(std::abs(r.read().values[filterAmount]+4)<.00001&&std::abs(r.read().values[filterAttack]-.2)<.00001,"Filter session lost new conditions");
    auto* cutoffParameter=restored.getState().getParameter("cutoff");cutoffParameter->setValueNotifyingHost(cutoffParameter->convertTo0to1(700));r.undo();
    require(std::abs(r.read().values[filterAmount])<1e-12&&std::abs(r.read().values[cutoff]-700)<.001,"High-bit filter undo mask damaged unrelated automation");r.redo();
    require(std::abs(r.read().values[filterAmount]+4)<.00001,"High-bit filter redo lost Amount");r.undo();r.undo();
    require(std::abs(r.read().values[filterAttack]-.01)<.00001&&std::abs(r.read().values[filterRelease]-.4)<.00001,"Grouped filter undo lost ADSR baseline");
    s.setComparing(true);require(std::abs(s.audioValue(filterAmount))<1e-12&&std::abs(s.read().values[filterAmount]+4)<.00001,"Filter Compare destroyed edits");s.setComparing(false);
    const auto file=directory.getChildFile("filter.nitridepreset");require(s.exportPreset(file).wasOk(),"Filter preset export failed");
    const auto decoded=PresetStore::importFile(file);require(decoded&&std::abs(decoded->patch.values[filterAmount]+4)<1e-12,"Filter preset round trip failed");
    auto malformed=PresetStore::encode(*decoded);malformed["values"].getDynamicObject()->removeProperty("filter_amount");require(!PresetStore::decode(malformed),"New preset accepted missing filter Amount");
    for(int version:{1,2})
    {
        auto old=PresetStore::encode(*decoded);old.getDynamicObject()->setProperty("version",version);
        const auto legacy=PresetStore::decode(old);require(legacy&&std::abs(legacy->patch.values[filterAmount])<1e-12,"Old preset unexpectedly enabled filter envelope");
    }
    auto* hostAmount=p.getState().getParameter("filter_amount");hostAmount->setValueNotifyingHost(hostAmount->convertTo0to1(2));
    for(int i=0;i<10;++i)juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    require(std::abs(amount->getValue()-2)<.001,"Filter host automation did not update editor");
    editor.reset();directory.deleteRecursively();
}
}
void runFilterEnvelopeTests(){stages();ownership();sound();extremes();stateAndUi();std::cout<<"Filter envelope timing, polarity, shared ownership, legato, block timing, extreme routing, automation, UI, history and migration passed\n";}
