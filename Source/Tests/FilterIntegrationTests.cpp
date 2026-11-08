#include "FilterIntegrationTests.h"
#include "PluginEditor.h"
#include "Instrument/FilterPanel.h"
#include "Instrument/PresetStore.h"
#include "Instrument/AuParameterState.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
juce::File library(){return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-filter-integration-"+juce::Uuid().toString());}
Patch patch()
{
    auto p=factoryPatches()[1];p.mono=false;p.chord=false;p.values[filterFamily]=1;p.values[rageAmount]=.65;
    p.values[rageCharacter]=2;p.values[cutoff]=1300;p.values[resonance]=.65;
    p.values[filterAttack]=.25;p.values[filterDecay]=.3;p.values[filterSustain]=.65;p.values[filterAmount]=1.5;
    p.values[filterKeyTracking]=1;p.values[filterVelocity]=.5;p.values[release]=.1;p.values[spaceMix]=0;
    return p;
}
struct Harness
{
    InstrumentSession session{library()};InstrumentRenderer renderer{session};
    Harness(){session.apply(patch());renderer.prepare(48000);}
    void tick(int count,juce::MidiMessage message={})
    {juce::AudioBuffer<float> audio(2,count);juce::MidiBuffer midi;if(message.getRawDataSize()>0)midi.addEvent(message,0);renderer.process(audio,midi);require(std::isfinite(audio.getMagnitude(0,count))&&session.live.faults.load()==0,"Rage rendering fault");}
};
void voices()
{
    Harness h;h.tick(4800,juce::MidiMessage::noteOn(1,60,.8f));const auto first=h.renderer.voiceFilterSnapshot(60);
    require(first.envelope>.39&&first.envelope<.41,"Per-voice filter attack timing is wrong");
    h.tick(128,juce::MidiMessage::noteOn(1,72,.4f));const auto old=h.renderer.voiceFilterSnapshot(60),fresh=h.renderer.voiceFilterSnapshot(72);
    require(old.envelope>first.envelope&&fresh.envelope<.02,"New chord note retriggered an existing voice's filter");
    h.tick(2000,juce::MidiMessage::noteOff(1,72));require(h.renderer.voiceFilterSnapshot(60).envelope>old.envelope,"Unrelated note release closed another voice's filter");
    h.tick(6000);h.tick(4000,juce::MidiMessage::allSoundOff(1));require(h.session.live.voices.load()==0,"Rage panic retained a filter voice");
    for(int n=0;n<16;++n)h.tick(64,juce::MidiMessage::noteOn(1,48+n,.7f));
    require(h.session.live.voices.load()==12,"Rage changed the voice limit or broke stealing");
    h.tick(12000,juce::MidiMessage::allNotesOff(1));require(h.session.live.voices.load()==0,"Rage release failed after voice stealing");
    Harness rapid;rapid.session.set(filterAmount,0);rapid.session.set(filterVelocity,0);
    rapid.tick(12000,juce::MidiMessage::noteOn(1,72,.8f));const auto tracked=rapid.renderer.voiceFilterSnapshot(72).cutoff;
    rapid.session.set(filterFamily,0);rapid.tick(64);rapid.session.set(filterFamily,1);rapid.tick(64);
    require(rapid.renderer.voiceFilterSnapshot(72).cutoff>tracked*.99,"Rapid family reversal reset a still-running voice filter/tracking state");
}
std::vector<float> render(int rate,int block,bool switching)
{
    InstrumentSession s(library());auto p=patch();p.values[filterFmDepth]=3;
    p.values[cutoffLfoOn]=1;p.values[cutoffLfoRate]=3.1;p.values[cutoffLfoDepth]=.8;
    s.apply(p);InstrumentRenderer r(s);r.prepare(rate);std::vector<float> result;
    const auto total=rate/2;juce::MidiBuffer performance;
    performance.addEvent(juce::MidiMessage::noteOn(1,60,.8f),7);performance.addEvent(juce::MidiMessage::noteOn(1,67,.7f),rate/10+3);
    performance.addEvent(juce::MidiMessage::noteOff(1,60),rate*3/10);performance.addEvent(juce::MidiMessage::noteOff(1,67),rate*3/10+11);
    for(int offset=0;offset<total;)
    {
        // Fixed sample boundaries for family/character automation, independent of
        // the host's block partitioning.
        const auto boundary=offset<rate/5?rate/5:offset<rate*3/10?rate*3/10:total;
        if(switching&&offset==rate/5){s.set(rageCharacter,4);s.set(filterFamily,0);}
        if(switching&&offset==rate*3/10){s.set(filterFamily,1);s.set(rageCharacter,3);}
        const auto count=std::min({block,total-offset,boundary-offset});juce::AudioBuffer<float> audio(2,count);juce::MidiBuffer midi;
        midi.addEvents(performance,offset,count,-offset);r.process(audio,midi);
        require(s.live.faults.load()==0,"Rage modulation or switching caused a fault");
        for(int i=0;i<count;++i)result.push_back(audio.getSample(0,i));offset+=count;
    }
    return result;
}
void partitioning()
{
    for(const auto rate:{44100,48000,96000})for(const auto switches:{false,true})
    {
        const auto a=render(rate,64,switches),repeat=render(rate,64,switches),b=render(rate,257,switches);
        require(a==repeat,"Rage rendering is nondeterministic");double maximum=0;
        for(size_t i=0;i<a.size();++i)maximum=std::max(maximum,std::abs(static_cast<double>(a[i])-b[i]));
        require(maximum<1e-5,"Rage depends on host block partitioning");
    }
}
void state()
{
    std::vector<std::uint8_t> table(12+hostParameterCount*8);
    const auto write=[&](size_t at,std::uint32_t v){for(int i=0;i<4;++i)table[at+static_cast<size_t>(i)]=static_cast<std::uint8_t>(v>>(24-i*8));};
    const auto hash=[](const char* text){std::uint32_t v=0;while(*text)v=31*v+static_cast<unsigned char>(*text++);return v&0x7fffffffu;};
    write(8,hostParameterCount);
    for(int h=0;h<hostParameterCount;++h)write(12+static_cast<size_t>(h)*8,hash(h==monoHostIndex?monoParameterId:hostParameters[conditionIndex(h)].id));
    const auto trimmed=legacyAuParameterData(table.data(),table.size());require(trimmed&&trimmed->size()==12+v6HostParameterCount*8&&(*trimmed)[11]==89,"Legacy AU redundant table was not preserved");
    write(12+89*8+4,1);require(!legacyAuParameterData(table.data(),table.size()),"AU adapter discarded a nondefault new value");
    require(!legacyAuParameterData(table.data(),table.size()-1),"AU adapter accepted an unknown SDK layout");
    NitrideAudioProcessor p(library());auto& s=p.getInstrumentSession();
    require(hostParameterCount==92&&hostIndex(filterFamily)==89&&hostIndex(rageCharacter)==90&&hostIndex(rageAmount)==91,"New filter host indices displaced old automation");
    for(const auto parameter:{filterFamily,rageCharacter,rageAmount})require(p.getState().getParameter(hostParameters[parameter].id)->getVersionHint()==7,"Filter host version hint is wrong");
    const auto legacy=PresetStore::encode({"legacy",factoryPatches()[1]});require(static_cast<int>(legacy["version"])==6&&legacy["values"].getDynamicObject()->getProperties().size()==88,"Legacy preset serialization changed");
    require(PresetStore::decode(legacy).has_value(),"Frozen v6 preset no longer imports");
    s.apply(patch());s.setCompareReference(factoryPatches()[0]);s.setComparing(true);
    const auto encoded=PresetStore::encode({"rage",s.read()});require(static_cast<int>(encoded["version"])==7&&encoded["values"].getDynamicObject()->getProperties().size()==91,"Rage preset has wrong schema");
    require(static_cast<int>(s.saveDocument()["version"])==8,"Rage session did not select v8");
    juce::MemoryBlock saved;p.getStateInformation(saved);const auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()));require(xml&&xml->getIntAttribute("version")==8,"Rage XML did not select v8");
    NitrideAudioProcessor restored(library());restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));auto& loaded=restored.getInstrumentSession();
    require(!loaded.isComparing()&&loaded.read().values[filterFamily]==1&&loaded.read().values[rageCharacter]==2&&std::abs(loaded.read().values[rageAmount]-.65)<1e-12,"Rage state/Compare restore lost edited conditions");
    require(loaded.compareReference().values[filterFamily]==0,"Rage state lost Classic compare reference");
    loaded.beginEdit({hostIndex(rageAmount)});loaded.set(rageAmount,.9);loaded.endEdit({hostIndex(rageAmount)});
    loaded.undo();require(loaded.read().values[rageAmount]==.65,"Rage undo failed");loaded.redo();require(loaded.read().values[rageAmount]==.9,"Rage redo failed");
    loaded.set(filterFamily,0);require(static_cast<int>(loaded.saveDocument()["version"])==8&&loaded.read().values[rageAmount]==.9,"Classic selection discarded dormant Rage settings");
    auto invalid=juce::JSON::parse(juce::JSON::toString(encoded));invalid["values"].getDynamicObject()->removeProperty("rage_amount");require(!PresetStore::decode(invalid),"Incomplete v7 preset was silently defaulted");
    invalid=juce::JSON::parse(juce::JSON::toString(encoded));invalid.getDynamicObject()->setProperty("version",6);require(!PresetStore::decode(invalid),"New conditions under v6 were silently discarded");
    auto invalidDocument=juce::JSON::parse(juce::JSON::toString(loaded.saveDocument()));invalidDocument.getDynamicObject()->setProperty("version",7);
    require(loaded.restoreDocument(invalidDocument).failed()&&loaded.read().values[rageAmount]==.9,"Mislabeled legacy session changed the current sound");
    NitrideAudioProcessor legacyProcessor(library());legacyProcessor.getStateInformation(saved);
    const auto legacyXml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()));
    require(legacyXml&&legacyXml->getIntAttribute("version")==7&&legacyXml->getChildByName("NITRIDE_PARAMETERS")->getNumChildElements()==89,"Legacy XML wire format acquired new fields");
    restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
    require(loaded.read().values[filterFamily]==0&&loaded.read().values[rageCharacter]==0&&loaded.read().values[rageAmount]==0,"Loading legacy state retained stale Rage controls");
}
template<class T>T* child(juce::Component& root,const juce::String& name)
{for(auto* c:root.getChildren()){if(auto* found=dynamic_cast<T*>(c);found&&c->getName()==name)return found;if(auto* found=child<T>(*c,name))return found;}return nullptr;}
std::vector<float> liveCharacter(int selected,bool throughUi)
{
    NitrideAudioProcessor p(library());auto& s=p.getInstrumentSession();s.applyProgram(111);
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::ComboBox* choice=nullptr;
    if(throughUi)
    {
        editor.reset(p.createEditor());auto* panel=child<FilterPanel>(*editor,"Advanced filter");
        require(panel!=nullptr,"Missing live character panel");panel->sync(s.readVisible(),s.restoreEpoch.load());
        choice=child<juce::ComboBox>(*panel,"Rage character");require(choice!=nullptr,"Missing live character choice");
    }
    InstrumentRenderer renderer(s);renderer.prepare(48000);std::vector<float> result;
    for(int frame=0;frame<60032;frame+=64)
    {
        if(frame==24000)
        {
            if(throughUi)choice->setSelectedId(selected+1,juce::sendNotificationSync);
            else p.getState().getParameter(hostParameters[rageCharacter].id)->setValueNotifyingHost(selected/4.0f);
            require(s.audioValue(rageCharacter)==selected,"Live character did not reach the audio snapshot");
        }
        juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;
        if(frame==0)midi.addEvent(juce::MidiMessage::noteOn(1,48,96.0f/127),0);
        renderer.process(audio,midi);require(s.live.faults.load()==0,"Live character switch faulted");
        for(int n=0;n<64;++n)result.push_back(audio.getSample(0,n));
    }
    return result;
}
void liveCharacters()
{
    const auto diode=liveCharacter(0,false);
    for(int selected=0;selected<5;++selected)
    {
        const auto ui=liveCharacter(selected,true),host=liveCharacter(selected,false);
        require(ui==host,"UI character switching differs from host automation audio");
        require(std::equal(ui.begin(),ui.begin()+24000,diode.begin()),"Character switch changed samples before its event");
        if(selected>0)
        {
            double difference=0,reference=0;
            for(size_t n=48000;n<ui.size();++n){const auto d=ui[n]-diode[n];difference+=d*d;reference+=diode[n]*diode[n];}
            require(difference>reference*1e-5,"Live selection remained stuck on Diode");
        }
    }
}
void ui()
{
    NitrideAudioProcessor p(library());auto& s=p.getInstrumentSession();std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    auto* panel=child<FilterPanel>(*editor,"Advanced filter");auto* disclosure=child<juce::TextButton>(*editor,"Open advanced filter");require(panel&&disclosure&&!panel->isShowing(),"Filter controls did not start progressively hidden");
    disclosure->setToggleState(true,juce::dontSendNotification);disclosure->onClick();require(panel->isShowing(),"Filter disclosure did not open");
    auto* family=child<juce::ComboBox>(*panel,"Filter family");auto* character=child<juce::ComboBox>(*panel,"Rage character");auto* amount=child<UI::ValueControl>(*panel,"Rage");
    require(family&&character&&amount&&!amount->isEnabled(),"Classic exposed active Rage controls");
    family->setSelectedId(2,juce::sendNotificationSync);character->setSelectedId(5,juce::sendNotificationSync);amount->getAccessibilityHandler()->getValueInterface()->setValue(.7);
    require(s.read().values[filterFamily]==1&&s.read().values[rageCharacter]==4&&std::abs(s.read().values[rageAmount]-.7)<1e-12,"Native filter controls did not edit host state");
    juce::MemoryBlock saved;p.getStateInformation(saved);amount->onBegin();p.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
    amount->onChange(.1);amount->onEnd();require(std::abs(s.read().values[rageAmount]-.7)<1e-12,"Stale filter gesture overwrote recalled state");
    panel->sync(s.readVisible(),s.restoreEpoch.load());family->setSelectedId(1,juce::sendNotificationSync);require(s.read().values[rageCharacter]==4&&std::abs(s.read().values[rageAmount]-.7)<1e-12,"Family switch discarded character settings");
    panel->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));require(!panel->isShowing(),"Escape did not close filter panel");
}
}
void runFilterIntegrationTests(){voices();partitioning();state();ui();liveCharacters();std::cout<<"Filter framework passed per-voice ownership, note lifetime, partitioning, migration, dormant settings, history, host metadata, native UI and five-character live audio switching\n";}
