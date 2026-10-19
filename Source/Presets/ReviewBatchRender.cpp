#include "PluginProcessor.h"
#include "Instrument/PresetStore.h"
#include "BuildInfo.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride;
constexpr double rate=48000;
constexpr int blockSize=64;
void require(bool good,const juce::String& message){if(!good)throw std::runtime_error(message.toStdString());}
struct Sketch
{
    juce::String file,category,intent,revision;StoredPreset preset;
    double seconds;juce::File midiFile;
};
std::vector<Sketch> sketches(const juce::File& directory)
{
    const auto manifest=juce::JSON::parse(directory.getChildFile("batch.json").loadFileAsString());
    require(manifest["format"].toString()=="nitride-review-batch"&&static_cast<int>(manifest["version"])==1,"Unsupported review manifest");
    const auto* entries=manifest["sketches"].getArray();require(entries&&entries->size()>0&&entries->size()<=12,"Review batch must contain 1-12 sketches");
    std::vector<Sketch> result;
    for(const auto& entry:*entries)
    {
        const auto file=entry["file"].toString();require(file.isNotEmpty()&&file.length()<=64&&file.containsOnly("abcdefghijklmnopqrstuvwxyz0123456789-"),"Invalid review file identifier");
        require(std::none_of(result.begin(),result.end(),[&](const auto& s){return s.file==file;}),"Duplicate review file identifier");
        const auto preset=PresetStore::importFile(directory.getChildFile(file+".nitridepreset"));require(preset.has_value(),"Invalid preset: "+file);
        const auto seconds=static_cast<double>(entry["seconds"]);require(std::isfinite(seconds)&&seconds>=1&&seconds<=120,"Review duration must be 1-120 seconds");
        const auto midi=directory.getChildFile(file+".mid");require(midi.existsAsFile()&&midi.getSize()<=1024*1024,"Missing or oversized MIDI performance");
        result.push_back({file,entry["category"].toString(),entry["intent"].toString(),manifest["revision"].toString(),*preset,seconds,midi});
    }
    return result;
}
struct Performance{juce::MidiBuffer events;double quarterSeconds=.5;};
Performance readMidi(const juce::File& file)
{
    juce::FileInputStream stream(file);juce::MidiFile midi;require(midi.readFrom(stream),"Cannot read exported MIDI");
    require(midi.getTimeFormat()>0,"Review MIDI must use a ticks-per-quarter-note clock");Performance performance;int tempos=0;
    for(int t=0;t<midi.getNumTracks();++t)for(int e=0;e<midi.getTrack(t)->getNumEvents();++e)
    {
        const auto& message=midi.getTrack(t)->getEventPointer(e)->message;
        if(message.isTempoMetaEvent()){require(message.getTimeStamp()<=0,"Review performance must have a fixed initial tempo");performance.quarterSeconds=message.getTempoSecondsPerQuarterNote();++tempos;}
        if(message.isTimeSignatureMetaEvent()){int numerator=0,denominator=0;message.getTimeSignatureInfo(numerator,denominator);require(numerator==4&&denominator==4,"Review transport currently supports 4/4 MIDI only");}
    }
    require(tempos==1&&performance.quarterSeconds>=.15&&performance.quarterSeconds<=3,"Review performance must contain one 20-400 BPM tempo");midi.convertTimestampTicksToSeconds();
    for(int t=0;t<midi.getNumTracks();++t)for(int e=0;e<midi.getTrack(t)->getNumEvents();++e)
    {
        const auto& message=midi.getTrack(t)->getEventPointer(e)->message;
        if(!message.isMetaEvent())
        {require(message.getTimeStamp()>=0&&message.getTimeStamp()*rate<std::numeric_limits<int>::max(),"MIDI time out of range");performance.events.addEvent(message,static_cast<int>(std::llround(message.getTimeStamp()*rate)));}
    }
    return performance;
}
struct Playhead final:juce::AudioPlayHead
{
    double quarterSeconds=.5;int sample=0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;const auto ppq=sample/(rate*quarterSeconds);p.setBpm(60/quarterSeconds);p.setPpqPosition(ppq);
        p.setPpqPositionOfLastBarStart(std::floor(ppq/4)*4);p.setBarCount(static_cast<juce::int64>(std::floor(ppq/4)));
        p.setTimeSignature(TimeSignature{4,4});p.setIsPlaying(true);p.setTimeInSamples(sample);p.setTimeInSeconds(sample/rate);return p;
    }
};
struct Render
{
    juce::AudioBuffer<float> audio;std::vector<double> times;int maximumVoices=0;std::uint64_t faults=0,overdue=0;
    double occupied=0;
};
Render render(const Patch& p,const Performance& performance,double seconds,int block)
{
    const auto library=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-offline-"+juce::Uuid().toString());
    NitrideAudioProcessor processor(library);processor.getInstrumentSession().apply(p);
    Playhead playhead;playhead.quarterSeconds=performance.quarterSeconds;processor.setPlayHead(&playhead);processor.prepareToPlay(rate,block);
    const auto samples=static_cast<int>(std::llround(seconds*rate));Render result;result.audio.setSize(2,samples);
    juce::AudioBuffer<float> buffer(2,block);juce::MidiBuffer midi;midi.ensureSize(4096);result.times.reserve(static_cast<size_t>(samples/block+1));
    double elapsed=0;
    for(int offset=0;offset<samples;)
    {
        const auto count=std::min(block,samples-offset);buffer.setSize(2,count,false,false,true);playhead.sample=offset;midi.clear();midi.addEvents(performance.events,offset,count,-offset);
        const auto started=std::chrono::steady_clock::now();processor.processBlock(buffer,midi);
        const auto micros=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-started).count();
        result.times.push_back(micros);elapsed+=micros;if(micros>1e6*count/rate)++result.overdue;
        for(int c=0;c<2;++c)result.audio.copyFrom(c,offset,buffer,c,0,count);
        result.maximumVoices=std::max(result.maximumVoices,processor.getInstrumentSession().live.voices.load());offset+=count;
    }
    result.faults=processor.getInstrumentSession().live.faults.load();require(result.faults==0,"Non-finite DSP state in review render");
    require(processor.getInstrumentSession().live.voices.load()==0,"Performance did not finish its amplitude release");
    processor.releaseResources();processor.setPlayHead(nullptr);result.occupied=elapsed/(seconds*1e6)*100;return result;
}
std::uint64_t hash(const juce::AudioBuffer<float>& audio)
{
    std::uint64_t value=14695981039346656037ull;
    for(int c=0;c<2;++c)for(int i=0;i<audio.getNumSamples();++i){const auto sample=audio.getSample(c,i);require(std::isfinite(sample),"Non-finite audio sample");value^=std::bit_cast<std::uint32_t>(sample);value*=1099511628211ull;}
    return value;
}
double difference(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{double maximum=0;for(int c=0;c<2;++c)for(int i=0;i<a.getNumSamples();++i)maximum=std::max(maximum,std::abs(static_cast<double>(a.getSample(c,i))-b.getSample(c,i)));return maximum;}
double activeRms(const juce::AudioBuffer<float>& audio)
{
    // 100 ms windows, gated at -50 dBFS. Simple active RMS, not LUFS.
    double energy=0;std::uint64_t samples=0;
    for(int offset=0;offset<audio.getNumSamples();offset+=4800)
    {
        const auto size=std::min(4800,audio.getNumSamples()-offset);double window=0;
        for(int c=0;c<2;++c)for(int i=0;i<size;++i){const auto x=audio.getSample(c,offset+i);window+=static_cast<double>(x)*x;}
        if(window/(2*size)>1e-5){energy+=window;samples+=static_cast<std::uint64_t>(2*size);}
    }
    require(samples>0,"Review sound unexpectedly silent");return std::sqrt(energy/static_cast<double>(samples));
}
void writeWave(const juce::File& file,const juce::AudioBuffer<float>& audio,double gain)
{
    juce::WavAudioFormat format;std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();require(stream!=nullptr,"Cannot open review WAV");
    const auto options=juce::AudioFormatWriterOptions{}.withSampleRate(rate).withNumChannels(2).withBitsPerSample(24);
    auto writer=format.createWriterFor(stream,options);require(writer!=nullptr,"Cannot create review WAV writer");
    auto output=audio;output.applyGain(static_cast<float>(gain));require(writer->writeFromAudioSampleBuffer(output,0,output.getNumSamples()),"Cannot write review WAV");
}
double percentile(std::vector<double> values,double p)
{std::sort(values.begin(),values.end());return values[static_cast<size_t>(std::ceil(p*static_cast<double>(values.size())))-1];}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialize;
    try
    {
        require(argc==3,"Usage: nitride_preset_review <input batch directory> <new output directory>");
        const auto input=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);const auto batch=sketches(input);
        const auto directory=juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
        require(!directory.exists(),"Choose a new output directory; existing review revisions are never overwritten");require(directory.createDirectory(),"Cannot create review directory");
        juce::Array<juce::var> reports;
        for(const auto& sketch:batch)
        {
            const auto prefix=directory.getChildFile(sketch.file),presetFile=prefix.withFileExtension(".nitridepreset"),midiFile=prefix.withFileExtension(".mid");
            require(PresetStore::exportFile(presetFile,sketch.preset).wasOk(),"Invalid review preset");require(sketch.midiFile.copyFileTo(midiFile),"Cannot copy review MIDI");
            const auto imported=PresetStore::importFile(presetFile);require(imported.has_value(),"Cannot import exported review preset");const auto performance=readMidi(midiFile);
            require(performance.events.getLastEventTime()<static_cast<int>(sketch.seconds*rate),"MIDI events exceed the review render duration");
            auto audio=render(imported->patch,performance,sketch.seconds,blockSize);const auto repeat=render(imported->patch,performance,sketch.seconds,blockSize);
            require(hash(audio.audio)==hash(repeat.audio),"Review render is not deterministic");const auto otherBlock=render(imported->patch,performance,sketch.seconds,257);
            const auto error=difference(audio.audio,otherBlock.audio);require(error<1e-5,"Review sound depends on host block size; maximum sample difference "+juce::String(error,12));
            const auto peak=audio.audio.getMagnitude(0,audio.audio.getNumSamples());require(peak<.94f,"Review preset hits master output ceiling");
            const auto rms=activeRms(audio.audio),gain=std::min(juce::Decibels::decibelsToGain(-24.0)/rms,juce::Decibels::decibelsToGain(-3.0)/peak);
            writeWave(prefix.withFileExtension(".wav"),audio.audio,gain);writeWave(directory.getChildFile(juce::String(sketch.file)+"-raw.wav"),audio.audio,1);
            auto* report=new juce::DynamicObject();report->setProperty("name",imported->patch.name);report->setProperty("category",sketch.category);report->setProperty("intent",sketch.intent);
            report->setProperty("revision",sketch.revision);report->setProperty("sampleRate",rate);report->setProperty("seconds",sketch.seconds);report->setProperty("bpm",60/performance.quarterSeconds);
            report->setProperty("synthVersion",buildVersion);report->setProperty("evaluationBuild",evaluationBuild);
            report->setProperty("maximumVoices",audio.maximumVoices);report->setProperty("rawPeakDbfs",juce::Decibels::gainToDecibels(peak));report->setProperty("rawActiveRmsDbfs",juce::Decibels::gainToDecibels(rms));
            report->setProperty("previewGainDb",juce::Decibels::gainToDecibels(gain));report->setProperty("previewPeakDbfs",juce::Decibels::gainToDecibels(peak*gain));report->setProperty("previewActiveRmsDbfs",juce::Decibels::gainToDecibels(rms*gain));
            report->setProperty("audioHash",juce::String::toHexString(static_cast<juce::int64>(hash(audio.audio))));report->setProperty("deterministic",true);report->setProperty("otherBlockMaximumError",error);
            auto times=audio.times;times.insert(times.end(),repeat.times.begin(),repeat.times.end());
            report->setProperty("meanPassLoadPercent",(audio.occupied+repeat.occupied)*.5);report->setProperty("p99Micros",percentile(times,.99));report->setProperty("overdueBlocks",static_cast<juce::int64>(audio.overdue+repeat.overdue));report->setProperty("faults",static_cast<juce::int64>(audio.faults));
            const auto tail=audio.audio.getNumSamples()-48000;report->setProperty("lastSecondRmsDbfs",juce::Decibels::gainToDecibels(audio.audio.getRMSLevel(0,tail,48000)));
            reports.add(juce::var(report));std::cout<<sketch.category<<": "<<imported->patch.name<<", voices "<<audio.maximumVoices<<", peak "<<peak<<", preview gain "<<juce::Decibels::gainToDecibels(gain)<<" dB, block error "<<error<<'\n';
        }
        require(directory.getChildFile("report.json").replaceWithText(juce::JSON::toString(juce::var(reports))),"Cannot write review report");
        std::cout<<"Review sketches exported to "<<directory.getFullPathName()<<" (factory/library unchanged)\n";return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
