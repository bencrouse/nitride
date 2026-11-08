#pragma once
#include "RageFilter.h"
#include "FilterEnvelope.h"
#include "FilterModulation.h"
#include <array>
#include <cmath>
#include <numbers>

namespace Nitride::DSP
{
// Stable family IDs. Classic is an explicit legacy bus route, not a voice filter.
// Future families (e.g. all-pass Ripple) own a different core and parameter set.
enum class FilterFamily { classic=0,rage=1 };
struct FilterSettings
{
    FilterFamily family=FilterFamily::classic;
    RageCharacter character=RageCharacter::diode;
    double amount=0,cutoff=5500,resonance=.18;
    std::array<double,4> envelope{.01,.3,.5,.4};
    double envelopeAmount=0,keyTracking=0,velocityAmount=0,fmDepth=0;
    bool operator==(const FilterSettings&) const = default;
};
struct FilterControls {double cutoff=5500,resonance=.18,amount=0,envelopeAmount=0,keyTracking=0,velocityAmount=0,fmDepth=0;};
struct FilterProcessingContract
{
    static constexpr int rageRateMultiplier=4;
    static constexpr bool rageBeforeVoiceAmplitude=true;
    static constexpr int bufferingLatency=0; // Minimum-phase IIR decimation; no block delay.
};
class VoiceFilter
{
public:
    void invalidate() noexcept {ready=false;}
    bool prepared()const noexcept{return ready;}
    void prepare(double rate,const FilterSettings& settings,double age=0,bool released=false,double releaseAge=0) noexcept
    {
        core.setCharacter(settings.character);core.prepare(rate);
        envelope.prepare(rate);configure(settings);envelope.resume(age,released,releaseAge);
        noteOffset=0;noteSmoothing=-std::expm1(-1/(rate*.025));ready=true;
    }
    void configure(const FilterSettings& settings) noexcept
    {envelope.configure(settings.envelope[0],settings.envelope[1],settings.envelope[2],settings.envelope[3]);core.setCharacter(settings.character);}
    void noteOn() noexcept {if(ready)envelope.noteOn();}
    void noteOff() noexcept {if(ready)envelope.noteOff();}
    double process(double input,const FilterControls& controls,int note,double velocity,double pitchTone,double cutoffLfo,double hostRate) noexcept
    {
        noteOffset+=(filterNoteOctaves(note,controls.keyTracking,velocity,controls.velocityAmount)-noteOffset)*noteSmoothing;
        const auto octaves=noteOffset+pitchTone/12+cutoffLfo+controls.envelopeAmount*envelope.next();
        lastCutoff=modulatedCutoff(controls.cutoff,octaves,controls.fmDepth,input,hostRate);
        return core.process(input,lastCutoff,controls.resonance,controls.amount);
    }
    double cutoff()const noexcept{return lastCutoff;}
    double envelopeLevel()const noexcept{return envelope.level();}
    RageFilter::Diagnostics diagnostics()const noexcept{return core.diagnostics();}
private:
    RageFilter core;FilterEnvelope envelope;
    double noteOffset=0,noteSmoothing=0,lastCutoff=5500;
    bool ready=false;
};

// Rage-only eighth-order Butterworth anti-alias low-pass. Classic retains its
// original decimator and arithmetic. No allocation, FIR delay or host-latency switch.
class FilterDecimator
{
public:
    void prepare(double hostRate,double internalRate) noexcept
    {
        const auto g=std::tan(std::numbers::pi*hostRate*.40/internalRate);
        constexpr std::array q{.5097955791041592,.6013448869350453,.8999762231364156,2.562915447741506};
        for(size_t i=0;i<stages.size();++i)
        {
            auto& s=stages[i];const auto norm=1/(1+g/q[i]+g*g);
            s.b0=g*g*norm;s.b1=2*s.b0;s.b2=s.b0;s.a1=2*(g*g-1)*norm;s.a2=(1-g/q[i]+g*g)*norm;s.z1=s.z2=0;
        }
    }
    double process(double input) noexcept
    {
        for(auto& s:stages){const auto y=s.b0*input+s.z1;s.z1=s.b1*input-s.a1*y+s.z2;s.z2=s.b2*input-s.a2*y;input=y;}
        return input;
    }
private:
    struct Stage {double b0=0,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;};
    std::array<Stage,4> stages;
};
}
