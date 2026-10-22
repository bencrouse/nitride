// Exact production Socket Dance r2 / original MIDI grids. No sound data is rewritten.
#include "PluginProcessor.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
struct Performance {juce::MidiBuffer events;double quarter=.5;};
Performance read(const juce::File& file,double rate)
{
    juce::FileInputStream stream(file);juce::MidiFile midi;require(midi.readFrom(stream),"Cannot read qualification MIDI");
    require(midi.getTimeFormat()>0,"MIDI must use ticks per quarter");Performance result;int tempos=0;
    for(int t=0;t<midi.getNumTracks();++t)for(int i=0;i<midi.getTrack(t)->getNumEvents();++i)
    {
        const auto& m=midi.getTrack(t)->getEventPointer(i)->message;
        if(m.isTempoMetaEvent()){require(m.getTimeStamp()==0,"Tempo must be fixed at the MIDI origin");result.quarter=m.getTempoSecondsPerQuarterNote();++tempos;}
    }
    require(tempos==1&&result.quarter>.15&&result.quarter<3,"Invalid qualification MIDI tempo");midi.convertTimestampTicksToSeconds();
    for(int t=0;t<midi.getNumTracks();++t)for(int i=0;i<midi.getTrack(t)->getNumEvents();++i)
    {
        const auto& m=midi.getTrack(t)->getEventPointer(i)->message;
        if(!m.isMetaEvent()){require(m.getTimeStamp()>=0&&m.getTimeStamp()<21,"MIDI exceeds the frozen22second qualification window");result.events.addEvent(m,static_cast<int>(std::llround(m.getTimeStamp()*rate)));}
    }
    return result;
}
struct Playhead final:juce::AudioPlayHead
{
    double rate=48000,quarter=.5;int sample=0;bool transitions=false;
    juce::Optional<PositionInfo> getPosition()const override
    {
        const auto time=sample/rate;double ppq=time/quarter;bool playing=true;
        if(transitions&&time>=14)ppq=2.125+(time-14)/quarter;
        else if(transitions&&time>=12){ppq=2.125;playing=false;}
        else if(transitions&&time>=8)ppq=-1.25+(time-8)/quarter;
        PositionInfo p;p.setBpm(60/quarter);p.setPpqPosition(ppq);p.setIsPlaying(playing);
        p.setPpqPositionOfLastBarStart(std::floor(ppq/4)*4);p.setBarCount(static_cast<juce::int64>(std::floor(ppq/4)));
        p.setTimeSignature(TimeSignature{4,4});p.setTimeInSamples(sample);p.setTimeInSeconds(time);return p;
    }
};
struct Render {std::vector<float> audio;std::uint64_t hash=14695981039346656037ull;int maximumVoices=0;double peak=0;};
Render render(const Performance& performance,double rate,int block,bool transitions)
{
    const auto library=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-socket-qualification-"+juce::Uuid().toString());
    NitrideAudioProcessor processor(library);const auto index=Nitride::builtInIndex("review:socket-dance:r2");require(index==32,"Socket historical program moved");
    processor.getInstrumentSession().applyProgram(index);Playhead playhead;playhead.rate=rate;playhead.quarter=performance.quarter;playhead.transitions=transitions;
    processor.setPlayHead(&playhead);processor.prepareToPlay(rate,block);
    const auto frames=static_cast<int>(rate*22);Render result;result.audio.reserve(static_cast<size_t>(frames)*2);
    juce::AudioBuffer<float> buffer(2,block);juce::MidiBuffer midi;midi.ensureSize(4096);
    for(int offset=0;offset<frames;)
    {
        auto count=std::min(block,frames-offset);
        // Make host transport discontinuities sample-identical across block partitions.
        if(transitions)for(int second:{8,12,14}){const auto boundary=static_cast<int>(rate*second);if(boundary>offset)count=std::min(count,boundary-offset);}
        buffer.setSize(2,count,false,false,true);midi.clear();midi.addEvents(performance.events,offset,count,-offset);playhead.sample=offset;
        processor.processBlock(buffer,midi);
        result.maximumVoices=std::max(result.maximumVoices,processor.getInstrumentSession().live.voices.load());
        for(int i=0;i<count;++i)for(int c=0;c<2;++c)
        {
            const auto x=buffer.getSample(c,i);require(std::isfinite(x),"Non-finite Socket output");result.audio.push_back(x);
            result.hash^=std::bit_cast<std::uint32_t>(x);result.hash*=1099511628211ull;result.peak=std::max(result.peak,std::abs(static_cast<double>(x)));
        }
        offset+=count;
    }
    require(processor.getInstrumentSession().live.faults.load()==0,"Socket DSP fault");
    require(processor.getInstrumentSession().live.voices.load()==0,"Socket left an active voice after release");
    require(result.maximumVoices>0&&result.peak>.000001,"Socket qualification unexpectedly silent");
    processor.releaseResources();processor.setPlayHead(nullptr);return result;
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        require(argc==3||argc==4,"Usage: nitride_socket_tests MIDI120 MIDI118 [NEW-report.json]");
        const juce::File fast=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]),control=juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
        juce::Array<juce::var> cases;bool passed=true;
        for(const auto rate:{48000.0,96000.0})for(int context=0;context<3;++context)
        {
            const auto performance=read(context==1?control:fast,rate);const auto transitions=context==2;
            require(std::abs(60/performance.quarter-(context==1?118:120))<.001,"Frozen MIDI tempo differs from its documented grid");
            const auto baseline=render(performance,rate,64,transitions),repeat=render(performance,rate,64,transitions);
            auto* row=new juce::DynamicObject();row->setProperty("sampleRate",rate);row->setProperty("bpm",60/performance.quarter);
            row->setProperty("transport",transitions?"seek-stop-resume":"normal");row->setProperty("seconds",22);
            const auto deterministic=baseline.hash==repeat.hash&&baseline.audio==repeat.audio;row->setProperty("deterministic",deterministic);passed&=deterministic;
            row->setProperty("rawPeak",baseline.peak);row->setProperty("maximumVoices",baseline.maximumVoices);juce::Array<juce::var> comparisons;
            for(int block:{257,1024})
            {
                const auto other=render(performance,rate,block,transitions);double maximum=0;size_t sample=0;
                require(other.audio.size()==baseline.audio.size(),"Socket render size mismatch");
                for(size_t i=0;i<baseline.audio.size();++i){const auto error=std::abs(static_cast<double>(baseline.audio[i])-other.audio[i]);if(error>maximum){maximum=error;sample=i/2;}}
                auto* item=new juce::DynamicObject();item->setProperty("blockFrames",block);item->setProperty("maximumError",maximum);item->setProperty("maximumErrorFrame",static_cast<juce::int64>(sample));comparisons.add(juce::var(item));passed&=maximum<1e-5;
            }
            row->setProperty("comparisons",comparisons);cases.add(juce::var(row));
        }
        auto* document=new juce::DynamicObject();document->setProperty("format","nitride-socket-qualification");document->setProperty("version",1);
        document->setProperty("passed",passed);document->setProperty("acceptance",1e-5);document->setProperty("cases",cases);
        const auto text=juce::JSON::toString(juce::var(document));std::cout<<text<<'\n';
        if(argc==4){const auto path=juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]);require(!path.exists(),"Choose a NEW Socket report path");require(path.replaceWithText(text),"Cannot write Socket report");}
        return passed?0:1;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
