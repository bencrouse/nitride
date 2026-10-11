#include "PluginProcessor.h"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
// Fresh 0.5.0 baseline: Apple Silicon, pinned JUCE 8.0.15, 48 kHz,
// 96,000 mono output samples hashed as float words in timeline order.
constexpr std::array<std::uint64_t,4> factoryHashes{
    0xe0029c75078aa31full,0xd0f816a8c30371d2ull,0x6477724fe25c1ddaull,0x463a9c23bd376aaaull
};
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
std::vector<float> render(int program,bool zeroDepthEnabled,int block)
{
    const auto library=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-current-"+juce::Uuid().toString());
    NitrideAudioProcessor p(library);auto patch=Nitride::factoryPatches()[static_cast<size_t>(program)];
    if(zeroDepthEnabled)for(const auto& lane:Nitride::lfoParameters){patch.values[lane.enabled]=1;patch.values[lane.wave]=4;}
    p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,block);juce::MidiBuffer events;
    for(const auto note:{48,55,60,67})events.addEvent(juce::MidiMessage::noteOn(1,note,.8f),13);
    events.addEvent(juce::MidiMessage::allNotesOff(1),48013);std::vector<float> result(96000);
    for(int offset=0;offset<96000;offset+=block)
    {
        const auto size=std::min(block,96000-offset);juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;midi.addEvents(events,offset,size,-offset);p.processBlock(audio,midi);
        for(int i=0;i<size;++i){require(std::isfinite(audio.getSample(0,i)),"Non-finite current factory render");result[static_cast<size_t>(offset+i)]=audio.getSample(0,i);}
    }
    require(p.getInstrumentSession().live.faults.load()==0,"Current factory caused numerical faults");return result;
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        for(int program=0;program<4;++program)
        {
            const auto baseline=render(program,false,64);require(baseline==render(program,false,64),"Current factory rendering is not deterministic");
            require(baseline==render(program,true,64),"Zero-depth enabled LFOs changed the sound");
            const auto other=render(program,false,257);double error=0,energy=0;std::uint64_t hash=14695981039346656037ull;
            for(size_t i=0;i<baseline.size();++i){error=std::max(error,std::abs(static_cast<double>(baseline[i])-other[i]));energy+=baseline[i]*baseline[i];hash^=std::bit_cast<std::uint32_t>(baseline[i]);hash*=1099511628211ull;}
            require(error<1e-6&&energy>.01,"Current factory block timing or audible output failed");
            require(hash==factoryHashes[static_cast<size_t>(program)],"Current-schema factory sound changed from the 0.5.0 baseline");
            std::cout<<"Current factory "<<program<<": hash "<<std::hex<<hash<<std::dec<<", block error "<<error<<", energy "<<energy<<'\n';
        }
        std::cout<<"Current-schema sound regression passed; historical fixtures are archival, not a greenfield compatibility gate\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
