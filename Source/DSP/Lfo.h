#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

namespace Nitride::DSP
{
enum class LfoWave { sine,triangle,saw,square,sampleHold };
enum class LfoDestination { pitch,fm,cutoff,amplitude,coupling,stress };
inline constexpr size_t lfoCount=6;
struct Subdivision {const char* name;double beats;bool bars=false;};
inline constexpr std::array subdivisions {
    Subdivision{"4 bars",4,true},Subdivision{"2 bars",2,true},Subdivision{"1 bar",1,true},
    Subdivision{"1/1",4},Subdivision{"1/1 dotted",6},Subdivision{"1/1 triplet",8.0/3},
    Subdivision{"1/2",2},Subdivision{"1/2 dotted",3},Subdivision{"1/2 triplet",4.0/3},
    Subdivision{"1/4",1},Subdivision{"1/4 dotted",1.5},Subdivision{"1/4 triplet",2.0/3},
    Subdivision{"1/8",.5},Subdivision{"1/8 dotted",.75},Subdivision{"1/8 triplet",1.0/3},
    Subdivision{"1/16",.25},Subdivision{"1/16 dotted",.375},Subdivision{"1/16 triplet",1.0/6},
    Subdivision{"1/32",.125},Subdivision{"1/32 dotted",.1875},Subdivision{"1/32 triplet",1.0/12},
    Subdivision{"1/64",.0625},Subdivision{"1/64 dotted",.09375},Subdivision{"1/64 triplet",1.0/24}
};
struct LfoSettings {bool enabled=false,sync=false;LfoWave wave=LfoWave::sine;double rate=.25,depth=0;int division=9;};
struct Transport
{
    double bpm=120,ppq=0,beatsPerBar=4;
    bool hasTempo=false,hasPosition=false,playing=false;
    double barStart=0;std::int64_t barCount=0;bool hasBarStart=false,hasBarCount=false;
};
inline double beatsPerCycle(int division,double beatsPerBar=4) noexcept
{
    const auto& d=subdivisions[static_cast<size_t>(std::clamp(division,0,static_cast<int>(subdivisions.size())-1))];
    return d.beats*(d.bars?beatsPerBar:1);
}
inline double lfoWave(LfoWave wave,double phase,std::int64_t cycle=0,std::uint64_t seed=1) noexcept
{
    switch(wave)
    {
        case LfoWave::sine:return std::sin(phase*2*std::numbers::pi);
        case LfoWave::triangle:return 1-4*std::abs(phase-.25-std::floor(phase+.25));
        case LfoWave::saw:return 2*phase-1;
        case LfoWave::square:return phase<.5?1:-1;
        case LfoWave::sampleHold:
        {
            // Stateless cycle hashing: seeking back to a beat reproduces its value.
            auto x=static_cast<std::uint64_t>(cycle)+seed*0x9e3779b97f4a7c15ull;
            x=(x^(x>>30))*0xbf58476d1ce4e5b9ull;x=(x^(x>>27))*0x94d049bb133111ebull;x^=x>>31;
            return static_cast<double>(x>>11)*(2.0/9007199254740992.0)-1;
        }
    }
    return 0;
}
class LfoBank
{
public:
    void prepare(double sampleRate) noexcept
    {
        rate=std::isfinite(sampleRate)&&sampleRate>0?sampleRate:48000;slew=-std::expm1(-1/(rate*.003));lanes={};transport={};position=0;positionSamples=0;
    }
    void configure(const std::array<LfoSettings,lfoCount>& settings,const Transport& host,double fallbackTempo) noexcept
    {
        transport=host;transport.bpm=host.hasTempo&&std::isfinite(host.bpm)&&host.bpm>=20&&host.bpm<=400?host.bpm:std::clamp(std::isfinite(fallbackTempo)?fallbackTempo:120.0,20.0,300.0);
        transport.hasTempo=host.hasTempo&&std::isfinite(host.bpm)&&host.bpm>=20&&host.bpm<=400;
        transport.hasPosition=host.hasPosition&&std::isfinite(host.ppq)&&std::abs(host.ppq)<1e12;
        transport.hasBarStart=host.hasBarStart&&std::isfinite(host.barStart)&&std::abs(host.barStart)<1e12;
        transport.hasBarCount=host.hasBarCount&&host.barCount>-1000000000000ll&&host.barCount<1000000000000ll;
        transport.beatsPerBar=std::isfinite(host.beatsPerBar)&&host.beatsPerBar>=.01&&host.beatsPerBar<=1024?host.beatsPerBar:4;
        position=transport.hasPosition?transport.ppq:0;positionSamples=0;
        for(size_t i=0;i<lfoCount;++i)
        {
            auto& lane=lanes[i];lane.settings=settings[i];
            lane.settings.rate=std::clamp(std::isfinite(lane.settings.rate)?lane.settings.rate:.25,.01,20.0);
            constexpr std::array maxima{24.0,4.0,6.0,1.0,1.0,1.0};
            lane.settings.depth=std::clamp(std::isfinite(lane.settings.depth)?lane.settings.depth:0.0,i==3?0.0:-maxima[i],maxima[i]);
            lane.settings.wave=static_cast<LfoWave>(std::clamp(static_cast<int>(lane.settings.wave),0,4));
            lane.settings.division=std::clamp(lane.settings.division,0,static_cast<int>(subdivisions.size())-1);
            const auto target=lane.settings.enabled?lane.settings.depth:0;
            if(target<lane.targetDepth||target>lane.targetDepth)
            {lane.targetDepth=target;lane.remaining=std::max(1,static_cast<int>(rate*.02));lane.depthStep=(target-lane.depth)/lane.remaining;}
        }
    }
    std::array<double,lfoCount> next() noexcept
    {
        std::array<double,lfoCount> result{};
        for(size_t i=0;i<lfoCount;++i)
        {
            auto& lane=lanes[i];const auto& s=lane.settings;
            if(lane.remaining>0){lane.depth+=lane.depthStep;if(--lane.remaining==0)lane.depth=lane.targetDepth;}
            double phase=lane.phase;auto cycle=lane.cycle;
            const auto beats=beatsPerCycle(s.division,transport.beatsPerBar);
            if(s.sync&&transport.hasPosition)
            {
                // A discrete cycle hash must not change one sample earlier/later
                // with host block size due to accumulated PPQ addition error.
                // Continuous waves retain their existing interpolation arithmetic.
                const auto discrete=s.wave==LfoWave::sampleHold;
                const auto syncedPosition=discrete&&transport.playing?transport.ppq+static_cast<double>(positionSamples)*transport.bpm/(60*rate):position;
                auto cycles=syncedPosition/beats;
                const auto& subdivision=subdivisions[static_cast<size_t>(s.division)];
                if(subdivision.bars&&transport.hasBarStart)
                {
                    const auto bars=transport.hasBarCount?static_cast<double>(transport.barCount):std::floor(transport.barStart/transport.beatsPerBar);
                    cycles=(bars+(syncedPosition-transport.barStart)/transport.beatsPerBar)/subdivision.beats;
                }
                if(discrete)
                {
                    const auto boundary=std::round(cycles);
                    const auto tolerance=std::min(1e-12,8*std::numeric_limits<double>::epsilon()*std::max(1.0,std::abs(cycles)));
                    if(std::abs(cycles-boundary)<=tolerance)cycles=boundary;
                }
                cycle=static_cast<std::int64_t>(std::floor(cycles));phase=cycles-std::floor(cycles);
            }
            if(std::abs(lane.depth)>0||s.enabled)
            {
                lane.value+=slew*(lfoWave(s.wave,phase,cycle,i+1)-lane.value);
                lane.value=std::clamp(lane.value,-1.0,1.0);
            }
            result[i]=i==static_cast<size_t>(LfoDestination::amplitude)?1-std::abs(lane.depth)*(.5+.5*lane.value):lane.value*lane.depth;
            if(i==1)lane.sourceGain+=slew*((s.enabled?1.0:0.0)-lane.sourceGain);
            const auto increment=(s.sync?transport.bpm/(60*beats):s.rate)/rate;
            lane.phase+=increment;if(lane.phase>=1){const auto wraps=std::floor(lane.phase);lane.phase-=wraps;lane.cycle+=static_cast<std::int64_t>(wraps);}
            lane.displayPhase=phase;
        }
        if(transport.playing){position+=transport.bpm/(60*rate);++positionSamples;}
        return result;
    }
    double phase(size_t lane) const noexcept {return lanes[lane].displayPhase;}
    double fmWave() const noexcept {return lanes[1].value*lanes[1].sourceGain;}
    double bpm() const noexcept {return transport.bpm;}
    bool hostTempo() const noexcept {return transport.hasTempo;}
    bool engineActive() const noexcept
    {for(const auto i:{0,1,4,5})if(std::abs(lanes[static_cast<size_t>(i)].depth)>0||std::abs(lanes[static_cast<size_t>(i)].targetDepth)>0)return true;return false;}
private:
    struct Lane
    {
        LfoSettings settings;double phase=0,displayPhase=0,value=0,depth=0,targetDepth=0,depthStep=0,sourceGain=0;
        std::int64_t cycle=0;int remaining=0;
    };
    std::array<Lane,lfoCount> lanes{};Transport transport;double rate=48000,slew=0,position=0;std::uint64_t positionSamples=0;
};
}
