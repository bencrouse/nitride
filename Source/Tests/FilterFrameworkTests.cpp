#include "PluginProcessor.h"
#include <bit>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
std::uint64_t classicHash(int program,int rate,int block)
{
    NitrideAudioProcessor processor(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-filter-test-"+juce::Uuid().toString()));
    processor.getInstrumentSession().apply(Nitride::builtInPresets()[static_cast<size_t>(program)].patch);
    processor.prepareToPlay(rate,block);
    juce::MidiBuffer performance;
    performance.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8{96}),0);
    performance.addEvent(juce::MidiMessage::noteOn(1,67,juce::uint8{80}),rate*13/100);
    performance.addEvent(juce::MidiMessage::noteOff(1,60),rate*53/100);
    performance.addEvent(juce::MidiMessage::noteOff(1,67),rate*61/100);
    performance.addEvent(juce::MidiMessage::noteOn(1,48,juce::uint8{112}),rate*74/100);
    performance.addEvent(juce::MidiMessage::noteOff(1,48),rate*9/10);
    std::uint64_t hash=14695981039346656037ull;
    for(int offset=0;offset<rate;offset+=block)
    {
        const auto count=std::min(block,rate-offset);juce::AudioBuffer<float> audio(2,count);juce::MidiBuffer midi;
        midi.addEvents(performance,offset,count,-offset);processor.processBlock(audio,midi);
        for(int i=0;i<count;++i)for(int c=0;c<2;++c)
        {
            const auto value=audio.getSample(c,i);require(std::isfinite(value),"Non-finite Classic baseline");
            hash^=std::bit_cast<std::uint32_t>(value);hash*=1099511628211ull;
        }
    }
    processor.releaseResources();return hash;
}
void legacyBaselines(const juce::File& file,bool write)
{
    juce::Array<juce::var> entries;
    if(!write)
    {
        const auto data=juce::JSON::parse(file.loadFileAsString());
        require(data["format"].toString()=="nitride-classic-filter-baselines"&&static_cast<int>(data["version"])==1,"Invalid Classic baseline fixture");
        const auto* array=data["entries"].getArray();require(array&&array->size()==212,"Incomplete Classic baseline fixture");entries=*array;
    }
    int index=0;
    for(const auto rate:{48000,96000})for(int p=0;p<106;++p)
    {
        const auto hash=juce::String::toHexString(static_cast<juce::int64>(classicHash(p,rate,64)));
        if(write)
        {
            auto* entry=new juce::DynamicObject();entry->setProperty("program",p);entry->setProperty("rate",rate);
            entry->setProperty("id",Nitride::builtInPresets()[static_cast<size_t>(p)].id);entry->setProperty("hash",hash);entries.add(juce::var(entry));
        }
        else
        {
            const auto& entry=entries[index];
            if(static_cast<int>(entry["program"])!=p||static_cast<int>(entry["rate"])!=rate||entry["id"].toString()!=Nitride::builtInPresets()[static_cast<size_t>(p)].id||entry["hash"].toString()!=hash)
                throw std::runtime_error("Classic audio changed at program "+std::to_string(p)+" / "+std::to_string(rate));
        }
        ++index;
    }
    if(write)
    {
        require(!file.exists(),"Never replace an existing baseline");auto* root=new juce::DynamicObject();root->setProperty("format","nitride-classic-filter-baselines");root->setProperty("version",1);
        root->setProperty("sourceBuild","0.7.38 / eval-038, before filter framework implementation");root->setProperty("entries",entries);
        require(file.create().wasOk()&&file.replaceWithText(juce::JSON::toString(juce::var(root))),"Cannot write baseline");
    }
    std::cout<<"All 106 Classic programs verified at 48/96 kHz\n";
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialize;
    try
    {
        const auto write=argc==3&&juce::String(argv[1])=="--write-classic-baselines";
        legacyBaselines(juce::File::getCurrentWorkingDirectory().getChildFile(write?argv[2]:NITRIDE_FILTER_BASELINES),write);
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
