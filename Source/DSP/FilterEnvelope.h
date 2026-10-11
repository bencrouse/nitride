#pragma once
#include <algorithm>
#include <cmath>

namespace Nitride::DSP
{
// Shared-filter ADSR: linear attack/release and exponential decay, matching the
// amplitude envelope's shapes. Timing edits change slopes, never jump the level.
class FilterEnvelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };
    void prepare(double sampleRate) noexcept
    {
        const auto decay=decaySeconds>0?decaySeconds:.3;
        rate=sampleRate;decaySeconds=0;configure(attackSeconds,decay,sustainLevel,releaseSeconds);reset();
    }
    void configure(double attack,double decay,double sustain,double release) noexcept
    {
        attackSeconds=safe(attack,.001,4,.01);releaseSeconds=safe(release,.02,8,.4);
        sustainLevel=safe(sustain,0,1,.5);
        const auto d=safe(decay,.01,4,.3);
        if(d<decaySeconds||d>decaySeconds){decaySeconds=d;decayStep=-std::expm1(-1/(rate*d));}
    }
    void noteOn() noexcept {attackStart=value;stage=Stage::attack;}
    void noteOff() noexcept
    {
        if(stage==Stage::idle||stage==Stage::release)return;
        releaseStart=value;stage=Stage::release;
    }
    void reset() noexcept {value=attackStart=releaseStart=0;stage=Stage::idle;}
    double next() noexcept
    {
        switch(stage)
        {
            case Stage::idle:break;
            case Stage::attack:
                value=std::min(1.0,value+(1-attackStart)/(rate*attackSeconds));
                if(value>=1-1e-12){value=1;stage=Stage::decay;}break;
            case Stage::decay:
            case Stage::sustain:
                value+=(sustainLevel-value)*decayStep;
                stage=std::abs(value-sustainLevel)<1e-6?Stage::sustain:Stage::decay;break;
            case Stage::release:
                value=std::max(0.0,value-releaseStart/(rate*releaseSeconds));
                if(value<1e-12){value=0;stage=Stage::idle;}break;
        }
        return value;
    }
    double level() const noexcept {return value;}
    Stage currentStage() const noexcept {return stage;}
private:
    static double safe(double v,double low,double high,double fallback) noexcept
    {return std::isfinite(v)?std::clamp(v,low,high):fallback;}
    double rate=48000,attackSeconds=.01,decaySeconds=0,sustainLevel=.5,releaseSeconds=.4,decayStep=0;
    double value=0,attackStart=0,releaseStart=0;
    Stage stage=Stage::idle;
};
}
