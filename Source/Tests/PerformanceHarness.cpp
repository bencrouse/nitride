#include "PluginProcessor.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>

namespace
{
using Clock = std::chrono::steady_clock;
constexpr int workloadVersion = 1;
struct Workload { const char* name; int patch, voices; bool stress = false, automation = false, retrigger = false, release = false; int pitchMotion=0; };
constexpr std::array workloads {
    Workload{"idle",0,0}, Workload{"lead",2,1}, Workload{"pad4",0,4}, Workload{"pad12",0,12},
    Workload{"stress12",0,12,true}, Workload{"automation12",0,12,true,true},
    Workload{"retrigger12",0,12,true,false,true}, Workload{"release12",0,12,false,false,false,true}
};
// Paired, identical MIDI performances; negative motion values disable the new
// effects. Keep the original version-1 matrix unchanged for before/after hashes.
constexpr std::array pitchWorkloads {
    Workload{"glide1-off",2,1,false,false,false,false,-1},Workload{"glide1-on",2,1,false,false,false,false,1},
    Workload{"autobend1-off",2,1,false,false,false,false,-2},Workload{"autobend1-on",2,1,false,false,false,false,2},
    Workload{"combined1-off",2,1,false,false,false,false,-3},Workload{"combined1-on",2,1,false,false,false,false,3},
    Workload{"combined4-off",0,4,false,false,false,false,-3},Workload{"combined4-on",0,4,false,false,false,false,3},
    Workload{"glide12-off",0,12,true,false,false,false,-1},Workload{"glide12-on",0,12,true,false,false,false,1},
    Workload{"autobend12-off",0,12,true,false,false,false,-2},Workload{"autobend12-on",0,12,true,false,false,false,2},
    Workload{"combined12-off",0,12,true,false,false,false,-3},Workload{"combined12-on",0,12,true,false,false,false,3},
    Workload{"tone12-off",0,12,false,false,false,false,-4},Workload{"tone12-on",0,12,false,false,false,false,4}
};
struct Options
{
    int passes = 3;
    double seconds = 2, warmup = .5;
    std::vector<int> rates {48000,96000}, blocks {64,256};
    juce::String only, label, output, compare;
    bool pitchMotion=false;
};
struct Result
{
    Result(Workload w,int sampleRate,int blockSize):workload(w),rate(sampleRate),block(blockSize){}
    Workload workload;
    int rate, block;
    std::vector<double> times, passLoads;
    double totalMicros = 0, energy = 0, peak = 0;
    std::uint64_t hash = 14695981039346656037ull, samples = 0, overdue = 0;
    int minimumVoices = 12, maximumVoices = 0;
};
void require(bool good, const juce::String& message) { if(!good)throw std::runtime_error(message.toStdString()); }
int integer(const juce::String& text)
{
    require(text.containsOnly("0123456789") && text.isNotEmpty(),"Expected positive integer: "+text);
    return text.getIntValue();
}
double number(const juce::String& text)
{
    require(text.containsOnly("0123456789.") && text.isNotEmpty(),"Expected positive number: "+text);
    const auto value = text.getDoubleValue();require(std::isfinite(value) && value > 0,"Expected positive number: "+text);return value;
}
Options parse(int argc, char** argv)
{
    Options options;
    for(int i=1;i<argc;++i)
    {
        const juce::String argument(argv[i]);
        if(argument=="--quick") { options.passes=1;options.seconds=.25;options.rates={48000};options.blocks={256};continue; }
        if(argument=="--pitch-motion"){options.pitchMotion=true;continue;}
        require(i+1<argc,"Missing value for "+argument);const juce::String value(argv[++i]);
        if(argument=="--passes")options.passes=integer(value);
        else if(argument=="--seconds")options.seconds=number(value);
        else if(argument=="--warmup")options.warmup=number(value);
        else if(argument=="--rate")options.rates={integer(value)};
        else if(argument=="--block")options.blocks={integer(value)};
        else if(argument=="--case")options.only=value;
        else if(argument=="--label")options.label=value;
        else if(argument=="--output")options.output=value;
        else if(argument=="--compare")options.compare=value;
        else throw std::runtime_error("Unknown option: "+argument.toStdString());
    }
    require(options.passes>=1 && options.passes<=20,"Passes must be 1–20");
    require(options.seconds<=120 && options.warmup<=10,"Duration out of range");
    for(const auto rate:options.rates)require(rate>=11025 && rate<=192000,"Sample rate out of range");
    for(const auto block:options.blocks)require(block>=16 && block<=4096,"Block size out of range");
    require(options.compare.isEmpty() || options.output.isEmpty() || juce::File(options.compare)!=juce::File(options.output),"Do not overwrite the baseline with the comparison output");
    return options;
}
double percentile(std::vector<double> values, double fraction)
{
    require(!values.empty(),"No timing observations");std::sort(values.begin(),values.end());
    return values[static_cast<size_t>(std::ceil(fraction*static_cast<double>(values.size())))-1];
}
juce::var object() { return juce::var(new juce::DynamicObject()); }
void property(juce::var& data,const juce::Identifier& name,const juce::var& value) { data.getDynamicObject()->setProperty(name,value); }
juce::String key(const Result& result) { return juce::String(result.workload.name)+"/"+juce::String(result.rate)+"/"+juce::String(result.block); }

void observe(Result& result, const juce::AudioBuffer<float>& audio, int voices)
{
    result.minimumVoices=std::min(result.minimumVoices,voices);result.maximumVoices=std::max(result.maximumVoices,voices);
    for(int channel=0;channel<audio.getNumChannels();++channel)
        for(int i=0;i<audio.getNumSamples();++i)
        {
            const auto sample=audio.getSample(channel,i);
            require(std::isfinite(sample),"Non-finite audio in "+key(result));
            result.peak=std::max(result.peak,std::abs(static_cast<double>(sample)));
            result.energy+=static_cast<double>(sample)*sample;++result.samples;
            result.hash^=std::bit_cast<std::uint32_t>(sample);result.hash*=1099511628211ull;
        }
}

void runPass(Result& result, const Options& options)
{
    // No audio device, editor, preset writes or preparation inside measured blocks.
    const auto emptyLibrary=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-perf-"+juce::Uuid().toString());
    NitrideAudioProcessor processor(emptyLibrary);
    auto& session=processor.getInstrumentSession();auto patch=Nitride::factoryPatches()[static_cast<size_t>(result.workload.patch)];
    if(result.workload.stress)
    {
        patch.values[Nitride::coupling]=1;patch.values[Nitride::stress]=1;patch.values[Nitride::response]=.01;
        patch.values[Nitride::fm]=4;patch.values[Nitride::attack]=.004;patch.values[Nitride::sustain]=.8;
        patch.values[Nitride::cutoff]=18000;patch.values[Nitride::resonance]=.85;
        patch.values[Nitride::motionRate]=8;patch.values[Nitride::motionDepth]=1;
        patch.values[Nitride::spaceMix]=.65;patch.values[Nitride::spaceSize]=1;
    }
    if(result.workload.pitchMotion!=0)
    {
        const auto motion=std::abs(result.workload.pitchMotion);const auto enabled=result.workload.pitchMotion>0;
        patch.values[Nitride::glideOn]=enabled&&motion!=2?1:0;patch.values[Nitride::glideTime]=.25;patch.values[Nitride::glideCurve]=0;
        patch.values[Nitride::autobendOn]=enabled&&motion!=1?1:0;patch.values[Nitride::autobendTime]=.25;patch.values[Nitride::autobendDepth]=12;
        patch.values[Nitride::autobendTarget]=motion==4?4:3;patch.values[Nitride::glideTarget]=motion==4?4:2;
    }
    session.apply(patch);processor.prepareToPlay(result.rate,result.block);
    juce::AudioBuffer<float> audio(2,result.block);juce::MidiBuffer midi;midi.ensureSize(4096);
    constexpr std::array pitches {48,55,59,62,64,67,71,74,76,79,83,86};
    for(int voice=0;voice<result.workload.voices;++voice)midi.addEvent(juce::MidiMessage::noteOn(1,pitches[static_cast<size_t>(voice)],static_cast<juce::uint8>(96)),0);
    const auto warmBlocks=static_cast<int>(std::ceil(options.warmup*result.rate/result.block));
    for(int block=0;block<warmBlocks;++block)processor.processBlock(audio,midi);
    require(session.live.voices.load()==result.workload.voices,"Workload did not reach its requested voice count: "+key(result));
    const auto blocks=static_cast<int>(std::ceil(options.seconds*result.rate/result.block));
    const auto interval=std::max(1,result.rate/(8*result.block));
    auto* coupling=processor.getState().getParameter("coupling");
    auto* stress=processor.getState().getParameter("stress");
    auto* response=processor.getState().getParameter("response");
    double passMicros=0;
    int previousTranspose=0;
    for(int block=0;block<blocks;++block)
    {
        // Input preparation and output auditing are excluded; real processor MIDI
        // dispatch, host parameter callbacks, rendering and live publication are timed.
        if(result.workload.release && block==0)midi.addEvent(juce::MidiMessage::allNotesOff(1),0);
        if(result.workload.retrigger && block%interval==0)
        {
            const auto voice=(block/interval)%result.workload.voices;
            const auto note=pitches[static_cast<size_t>(voice)];
            midi.addEvent(juce::MidiMessage::noteOff(1,note),result.block/4);
            midi.addEvent(juce::MidiMessage::noteOn(1,note,static_cast<juce::uint8>(96)),result.block/2);
        }
        if(result.workload.pitchMotion!=0&&block%interval==0)
        {
            constexpr std::array transpositions{7,-5,3,0};const auto transpose=transpositions[static_cast<size_t>((block/interval)%4)];
            for(int voice=0;voice<result.workload.voices;++voice)midi.addEvent(juce::MidiMessage::noteOff(1,pitches[static_cast<size_t>(voice)]+previousTranspose),result.block/4);
            for(int voice=0;voice<result.workload.voices;++voice)midi.addEvent(juce::MidiMessage::noteOn(1,pitches[static_cast<size_t>(voice)]+transpose,static_cast<juce::uint8>(96)),result.block/2);
            previousTranspose=transpose;
        }
        const auto phase=juce::MathConstants<double>::twoPi*static_cast<double>(block*result.block)/result.rate;
        const auto position=static_cast<float>(.5+.45*std::sin(phase));
        const auto responseValue=response->convertTo0to1(.01f+.59f*position);
        const auto start=Clock::now();
        if(result.workload.automation)
        {
            coupling->setValueNotifyingHost(position);stress->setValueNotifyingHost(1-position);
            response->setValueNotifyingHost(responseValue);
        }
        processor.processBlock(audio,midi);
        const auto micros=std::chrono::duration<double,std::micro>(Clock::now()-start).count();
        result.times.push_back(micros);result.totalMicros+=micros;passMicros+=micros;
        if(micros>1.0e6*result.block/result.rate)++result.overdue;
        observe(result,audio,session.live.voices.load());
    }
    require(session.live.faults.load()==0,"Numerical faults in "+key(result));
    result.passLoads.push_back(passMicros/(static_cast<double>(blocks)*1.0e6*result.block/result.rate)*100);
    processor.releaseResources();
}

juce::var report(const std::vector<Result>& results,const Options& options)
{
    auto root=object();property(root,"format","nitride-performance");property(root,"version",1);property(root,"workloadVersion",workloadVersion);
    property(root,"label",options.label);property(root,"timestamp",juce::Time::getCurrentTime().toISO8601(true));
    auto machine=object();property(machine,"cpu",juce::SystemStats::getCpuModel());property(machine,"os",juce::SystemStats::getOperatingSystemName());
    property(machine,"physicalCpus",juce::SystemStats::getNumPhysicalCpus());property(machine,"logicalCpus",juce::SystemStats::getNumCpus());
    property(machine,"configuration",NITRIDE_BENCH_CONFIG);property(machine,"compiler",__clang_version__);property(root,"machine",machine);
    auto settings=object();property(settings,"passes",options.passes);property(settings,"seconds",options.seconds);property(settings,"warmup",options.warmup);
    property(root,"settings",settings);juce::Array<juce::var> cases;
    for(const auto& result:results)
    {
        auto data=object();const auto budget=1.0e6*result.block/result.rate;
        property(data,"key",key(result));property(data,"sampleRate",result.rate);property(data,"blockSize",result.block);property(data,"workload",result.workload.name);
        property(data,"heldNotes",result.workload.voices);
        property(data,"blocks",static_cast<juce::int64>(result.times.size()));property(data,"audioSeconds",static_cast<double>(result.times.size())*result.block/result.rate);
        property(data,"renderMicros",result.totalMicros);property(data,"budgetMicros",budget);
        property(data,"meanLoadPercent",result.totalMicros/(static_cast<double>(result.times.size())*budget)*100);
        property(data,"medianPassLoadPercent",percentile(result.passLoads,.5));
        juce::Array<juce::var> loads;for(const auto load:result.passLoads)loads.add(load);property(data,"passLoadPercent",loads);
        for(const auto fraction:{.5,.95,.99})property(data,"p"+juce::String(static_cast<int>(fraction*100))+"Micros",percentile(result.times,fraction));
        property(data,"maxMicros",*std::max_element(result.times.begin(),result.times.end()));
        property(data,"overBudgetBlocks",static_cast<juce::int64>(result.overdue));
        property(data,"minimumVoices",result.minimumVoices);property(data,"maximumVoices",result.maximumVoices);
        property(data,"peak",result.peak);property(data,"rms",std::sqrt(result.energy/static_cast<double>(result.samples)));
        property(data,"audioHash",juce::String::toHexString(static_cast<juce::int64>(result.hash)));cases.add(data);
    }
    property(root,"cases",cases);return root;
}
void compare(juce::var& current,const Options& options)
{
    const auto file=juce::File(options.compare);require(file.existsAsFile(),"Baseline report not found");
    const auto baseline=juce::JSON::parse(file.loadFileAsString());
    require(baseline["format"]==current["format"] && baseline["version"]==current["version"] && baseline["workloadVersion"]==current["workloadVersion"],"Incompatible baseline format/workloads");
    require(juce::JSON::toString(baseline["settings"])==juce::JSON::toString(current["settings"]),"Baseline passes/duration/warmup differ");
    require(juce::JSON::toString(baseline["machine"])==juce::JSON::toString(current["machine"]),"Baseline machine/compiler/build configuration differ");
    const auto* previous=baseline["cases"].getArray();auto* cases=current.getDynamicObject()->getProperty("cases").getArray();
    require(previous!=nullptr && cases!=nullptr && previous->size()>=cases->size(),"Baseline does not cover this workload matrix");
    bool identical=true;
    std::cout<<"\nComparison: median pass load (lower is better); positive change means faster.\n";
    for(auto& data:*cases)
    {
        const auto found=std::find_if(previous->begin(),previous->end(),[&](const auto& p){return p["key"]==data["key"];});
        require(found!=previous->end(),"Missing baseline case: "+data["key"].toString());
        const auto before=static_cast<double>((*found)["medianPassLoadPercent"]),after=static_cast<double>(data["medianPassLoadPercent"]);
        require(std::isfinite(before) && before>0,"Invalid baseline timing");
        const auto change=100*(1-after/before);const auto audioSame=(*found)["audioHash"]==data["audioHash"];
        property(data,"baselineMedianPassLoadPercent",before);property(data,"improvementPercent",change);property(data,"identicalAudio",audioSame);
        identical &= audioSame;
        std::cout<<std::setw(28)<<data["key"].toString()<<"  "<<std::setw(6)<<before<<" -> "<<std::setw(6)<<after<<"%  "<<std::showpos<<change<<std::noshowpos<<"%  audio "<<(audioSame?"identical":"CHANGED")<<'\n';
    }
    property(current,"baseline",options.compare);require(identical,"Audio fingerprint changed; do not accept this optimization without investigating");
}
}

int main(int argc,char** argv)
{
    if(argc==2 && juce::String(argv[1])=="--help")
    {
        std::cout<<"Nitride offline processor benchmark\n"
            "--output report.json --label name --compare baseline.json\n"
            "--passes 3 --seconds 2 --warmup 0.5 --rate 48000 --block 64 --case stress12\n"
            "--quick runs one short pass at 48 kHz / 256 frames.\n"
            "--pitch-motion selects paired Glide/Autobend on/off performances.\n";return 0;
    }
    juce::ScopedJuceInitialiser_GUI initialiser;
    try
    {
        const auto options=parse(argc,argv);std::vector<Result> results;
        const auto add=[&](const auto& matrix) {
            for(const auto rate:options.rates)for(const auto block:options.blocks)for(const auto& workload:matrix)
                if(options.only.isEmpty()||options.only==workload.name)results.push_back({workload,rate,block});
        };
        if(options.pitchMotion)add(pitchWorkloads);else add(workloads);
        require(!results.empty(),"Unknown workload: "+options.only);
        for(auto& result:results)result.times.reserve(static_cast<size_t>(std::ceil(options.seconds*result.rate/result.block))*static_cast<size_t>(options.passes));
        std::cout<<"Nitride "<<NITRIDE_BENCH_CONFIG<<" benchmark: "<<juce::SystemStats::getCpuModel()<<", "<<options.passes<<" passes, "<<options.seconds<<" audio seconds per case/pass\n";
        std::vector<size_t> order(results.size());std::iota(order.begin(),order.end(),0);std::mt19937 random(0x4e545244);
        for(int pass=0;pass<options.passes;++pass)
        {
            std::shuffle(order.begin(),order.end(),random);
            for(const auto index:order)runPass(results[index],options);
            std::cout<<"Pass "<<pass+1<<" complete\n"<<std::flush;
        }
        auto document=report(results,options);std::cout<<std::fixed<<std::setprecision(2);
        std::cout<<"\n                        Case  load %   p50 us   p95 us   p99 us   max us  overdue\n";
        for(const auto& data:*document["cases"].getArray())
            std::cout<<std::setw(28)<<data["key"].toString()<<"  "<<std::setw(6)<<static_cast<double>(data["medianPassLoadPercent"])
                <<"  "<<std::setw(7)<<static_cast<double>(data["p50Micros"])<<"  "<<std::setw(7)<<static_cast<double>(data["p95Micros"])
                <<"  "<<std::setw(7)<<static_cast<double>(data["p99Micros"])<<"  "<<std::setw(7)<<static_cast<double>(data["maxMicros"])
                <<"  "<<data["overBudgetBlocks"].toString()<<'\n';
        if(options.compare.isNotEmpty())compare(document,options);
        if(options.output.isNotEmpty())
        {
            const juce::File output(options.output);require(output.getParentDirectory().createDirectory().wasOk(),"Cannot create report directory");
            require(output.replaceWithText(juce::JSON::toString(document)),"Cannot write benchmark report");
        }
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
