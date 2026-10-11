#include "PluginProcessor.h"
#include "Instrument/PresetStore.h"
#include <bit>
#include <iostream>
#include <stdexcept>

namespace
{
using namespace Nitride;
void require(bool ok,const juce::String& message){if(!ok)throw std::runtime_error(message.toStdString());}
juce::var object(){return juce::var(new juce::DynamicObject());}
void put(juce::var& v,const char* key,const juce::var& value){v.getDynamicObject()->setProperty(key,value);}
struct Scenario {juce::String name;Patch patch;juce::MidiBuffer midi;};
std::vector<Scenario> scenarios()
{
    std::vector<Scenario> result;
    for(int i=0;i<8;++i)
    {
        auto patch=factoryPatches()[static_cast<size_t>(i<4?i:2)];
        const std::array names{"pad","keys","lead","hit","stress","combined","tone","legato"};
        if(i==4){patch=factoryPatches()[0];patch.values[coupling]=1;patch.values[stress]=1;patch.values[response]=.01;patch.values[fm]=4;patch.values[release]=.3;}
        if(i>=5){patch.values[glideOn]=1;patch.values[glideCurve]=0;patch.values[glideTime]=.18;patch.values[autobendOn]=1;patch.values[autobendDepth]=12;patch.values[autobendTarget]=3;patch.values[autobendTime]=.22;}
        if(i==6){patch.values[glideTarget]=4;patch.values[autobendTarget]=4;patch.values[cutoff]=1200;patch.values[resonance]=.55;}
        if(i==7){patch.values[legato]=1;patch.values[autobendPhrase]=1;patch.values[autobendDepth]=-7;}
        Scenario s{names[static_cast<size_t>(i)],patch,{}};
        const auto add=[&](double seconds,const juce::MidiMessage& event){s.midi.addEvent(event,static_cast<int>(seconds*48000)+13);};
        if(i==0||i==4)
        {
            for(const auto note:{48,55,59,62})add(0,juce::MidiMessage::noteOn(1,note,.74f));
            add(.6,juce::MidiMessage::controllerEvent(1,64,127));
            for(const auto note:{48,55,59,62})add(1.2,juce::MidiMessage::noteOff(1,note));
            add(1.6,juce::MidiMessage::controllerEvent(1,64,0));
        }
        else
        {
            add(0,juce::MidiMessage::noteOn(1,60,.74f));add(.4,juce::MidiMessage::noteOn(1,67,.52f));add(.5,juce::MidiMessage::noteOff(1,60));
            add(.8,juce::MidiMessage::noteOn(1,72,.9f));add(.9,juce::MidiMessage::noteOff(1,67));
            add(1,juce::MidiMessage::pitchWheel(1,12000));add(1.2,juce::MidiMessage::pitchWheel(1,8192));add(1.4,juce::MidiMessage::noteOff(1,72));
        }
        result.push_back(std::move(s));
    }
    return result;
}
std::vector<float> render(const Patch& patch,const juce::MidiBuffer& events)
{
    const auto empty=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-reference-"+juce::Uuid().toString());
    NitrideAudioProcessor processor(empty);processor.getInstrumentSession().apply(patch);processor.prepareToPlay(48000,128);
    std::vector<float> samples(48000*4*2);juce::AudioBuffer<float> audio(2,128);juce::MidiBuffer midi;midi.ensureSize(4096);
    for(int offset=0;offset<48000*4;offset+=128)
    {
        midi.clear();midi.addEvents(events,offset,128,-offset);processor.processBlock(audio,midi);
        for(int i=0;i<128;++i)for(int c=0;c<2;++c)samples[static_cast<size_t>((offset+i)*2+c)]=audio.getSample(c,i);
    }
    return samples;
}
juce::String hash(const std::vector<float>& samples)
{
    std::uint64_t value=14695981039346656037ull;
    for(const auto sample:samples){value^=std::bit_cast<std::uint32_t>(sample);value*=1099511628211ull;}
    return juce::String::toHexString(static_cast<juce::int64>(value));
}
juce::var contract()
{
    juce::Array<juce::var> controls;
    for(int h=0;h<hostParameterCount;++h)
    {
        auto p=object();put(p,"position",h);
        if(h==monoHostIndex){put(p,"id","mono");put(p,"kind",1);put(p,"min",0);put(p,"max",1);put(p,"skew",1);put(p,"hint",1);put(p,"choices","");}
        else
        {
            const auto index=conditionIndex(h);const auto& definition=hostParameters[static_cast<size_t>(index)];
            put(p,"id",definition.id);put(p,"kind",static_cast<int>(definition.kind));put(p,"min",definition.minimum);put(p,"max",definition.maximum);
            put(p,"skew",definition.skew);put(p,"hint",index<legacyParameterCount?1:2);put(p,"choices",definition.choices);
        }
        controls.add(p);
    }
    return controls;
}
void capture(const juce::File& directory,const juce::String& producer)
{
    require(directory.createDirectory().wasOk(),"Cannot create fixture directory");
    require(producer.length()==40&&producer.containsOnly("0123456789abcdef"),"Capture requires an explicit reviewed producer revision");
    auto manifest=object();put(manifest,"producer",producer);put(manifest,"juce","91ad83ae34a81e0833b1a2b0866f54846370ae53");
    put(manifest,"compiler",__clang_version__);put(manifest,"os",juce::SystemStats::getOperatingSystemName());put(manifest,"format","nitride-float32-le-stereo");
    put(manifest,"sampleRate",48000);put(manifest,"blockSize",128);put(manifest,"controls",contract());juce::Array<juce::var> entries;
    for(const auto& s:scenarios())
    {
        auto entry=object();put(entry,"name",s.name);put(entry,"patch",PresetStore::encode({"reference",s.patch}));
        juce::Array<juce::var> events;
        for(const auto event:s.midi)
        {
            auto value=object();put(value,"sample",event.samplePosition);juce::Array<juce::var> bytes;
            for(int i=0;i<event.numBytes;++i)bytes.add(event.data[i]);put(value,"bytes",bytes);events.add(value);
        }
        put(entry,"events",events);const auto samples=render(s.patch,s.midi);put(entry,"hash",hash(samples));put(entry,"samples",static_cast<int>(samples.size()));
        const auto filename=s.name+".f32";put(entry,"audio",filename);
        require(directory.getChildFile(filename).replaceWithData(samples.data(),samples.size()*sizeof(float)),"Cannot write audio capture");entries.add(entry);
    }
    put(manifest,"performances",entries);require(directory.getChildFile("manifest.json").replaceWithText(juce::JSON::toString(manifest)),"Cannot write manifest");
    const auto compatibility=directory.getSiblingFile("Compatibility");
    NitrideAudioProcessor p(compatibility.getChildFile(".current-capture"));auto& session=p.getInstrumentSession();
    session.apply(scenarios()[5].patch);session.setCompareReference(session.read());session.beginEdit({hostIndex(glideTime)});session.set(glideTime,.33);
    juce::MemoryBlock state;p.getStateInformation(state);require(compatibility.getChildFile("session-v3-active.state").replaceWithData(state.getData(),state.getSize()),"Cannot write active gesture state");
    session.setComparing(true);p.getStateInformation(state);require(compatibility.getChildFile("session-v3-motion.state").replaceWithData(state.getData(),state.getSize()),"Cannot write current state");
    require(compatibility.getChildFile("preset-v2-motion.nitridepreset").replaceWithText(juce::JSON::toString(PresetStore::encode({"current-motion",session.read()}))),"Cannot write current preset");
    session.endAllEdits();
    std::cout<<"Captured reviewed reference performances from "<<producer<<'\n';
}
void verify(const juce::File& directory)
{
    const auto manifest=juce::JSON::parse(directory.getChildFile("manifest.json").loadFileAsString());
    const auto* expected=manifest["controls"].getArray();require(expected!=nullptr,"Missing frozen host contract");const auto actual=contract();
    for(int i=0;i<expected->size();++i)require(i<actual.size()&&juce::JSON::toString((*expected)[i])==juce::JSON::toString(actual[i]),"Published control contract changed");
    const auto* entries=manifest["performances"].getArray();require(entries!=nullptr&&entries->size()==8,"Missing reference performances");
    for(const auto& entry:*entries)
    {
        const auto patch=PresetStore::decode(entry["patch"]);require(patch.has_value(),"Cannot decode reference patch");juce::MidiBuffer events;
        for(const auto& e:*entry["events"].getArray())
        {
            const auto* bytes=e["bytes"].getArray();std::array<juce::uint8,3> data{};require(bytes&&bytes->size()<=3,"Invalid reference MIDI");
            for(int i=0;i<bytes->size();++i)data[static_cast<size_t>(i)]=static_cast<juce::uint8>(static_cast<int>((*bytes)[i]));
            events.addEvent(data.data(),bytes->size(),static_cast<int>(e["sample"]));
        }
        const auto candidate=render(patch->patch,events);juce::MemoryBlock bytes;
        require(directory.getChildFile(entry["audio"].toString()).loadFileAsData(bytes)&&bytes.getSize()==candidate.size()*sizeof(float),"Missing/corrupt reference audio");
        std::vector<float> approved(candidate.size());std::memcpy(approved.data(),bytes.getData(),bytes.getSize());require(hash(approved)==entry["hash"].toString(),"Reference capture hash mismatch");
        double maximum=0,energy=0;for(size_t i=0;i<candidate.size();++i){const auto error=static_cast<double>(candidate[i])-approved[i];maximum=std::max(maximum,std::abs(error));energy+=error*error;}
        const auto rms=std::sqrt(energy/static_cast<double>(candidate.size()));
        std::cout<<entry["name"].toString()<<": max "<<maximum<<", RMS "<<rms<<", exact "<<(hash(candidate)==entry["hash"].toString())<<'\n';
        require(maximum<=.000001&&rms<=.0000001,"Approved reference sound changed: "+entry["name"].toString());
    }
    const auto compatibility=directory.getSiblingFile("Compatibility");
    for(int version:{1,2})for(bool mono:{false,true})
    {
        juce::MemoryBlock state;const auto name=juce::String("session-v")+juce::String(version)+(mono?"-mono.state":"-poly.state");
        require(compatibility.getChildFile(name).loadFileAsData(state),"Missing historical state");
        NitrideAudioProcessor p(compatibility.getChildFile("empty-library"));p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));auto& s=p.getInstrumentSession();
        const auto patch=s.read();require(patch.mono==mono&&std::abs(patch.values[fm]-3.25)<.000001&&std::abs(patch.values[pitch]-5)<.000001,"Historical conditions did not migrate");
        require(std::abs(p.getState().getRawParameterValue("fm_depth")->load()-3.25)<.000001,"Historical host bank disagrees with session");
        require(!s.isComparing()&&patch.values[autobendOn]<.5,"Historical recall resumed an audition or enabled Autobend");
        if(mono)require(patch.values[glideOn]>.5&&std::abs(patch.values[glideTime]-.025)<.000001&&std::abs(patch.values[glideCurve]-2)<.000001,"Legacy mono character changed");
        if(version==2){require(s.presets().size()>4&&s.canUndo(),"Historical portable/history state lost");s.undo();require(std::abs(s.read().values[coupling]-(mono?.75:.55))<.000001,"Historical undo lost its baseline");}
    }
    for(const auto* name:{"preset-v1-poly.nitridepreset","preset-v1-mono.nitridepreset","preset-v2-motion.nitridepreset"})
        require(PresetStore::decode(juce::JSON::parse(compatibility.getChildFile(name).loadFileAsString())).has_value(),"Preserved portable preset failed to decode");
    juce::MemoryBlock state;require(compatibility.getChildFile("session-v3-motion.state").loadFileAsData(state),"Missing current state fixture");
    NitrideAudioProcessor p(compatibility.getChildFile("empty-library"));p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));auto& s=p.getInstrumentSession();
    require(s.read().values[autobendOn]>.5&&!s.isComparing()&&std::abs(s.read().values[glideTime]-.33)<.000001,"Current Compare/gesture recall lost edits");
    s.undo();require(std::abs(s.read().values[glideTime]-.18)<.000001,"Active-gesture fixture lost its undo baseline");
    std::cout<<"Preserved XML/session v1/v2/v3 and preset v1/v2 compatibility passed\n";
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        if(argc==4&&juce::String(argv[1])=="--capture-reviewed")capture(juce::File(argv[2]),argv[3]);
        else verify(juce::File(NITRIDE_AUDIO_FIXTURES));
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
