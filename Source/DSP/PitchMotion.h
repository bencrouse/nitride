#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace SoundStudies
{
enum class PitchTarget { carrier, modulators, all, opposed, tone };
enum class GlideCurve { smooth, linear, classic };
struct GlideSettings
{
    bool enabled=false;
    double seconds=.15;
    PitchTarget target=PitchTarget::all;
    bool legatoOnly=false;
    GlideCurve curve=GlideCurve::smooth;
};
struct AutobendSettings
{
    bool enabled=false;
    double seconds=.18, depth=0;
    PitchTarget target=PitchTarget::all;
    bool phraseOnly=false;
};
struct PitchSettings { GlideSettings glide; AutobendSettings autobend; bool legato=false; };
inline bool samePitchValue(double a,double b) noexcept { return std::bit_cast<std::uint64_t>(a)==std::bit_cast<std::uint64_t>(b); }
inline bool pitchIsZero(double value) noexcept { return !(value>0||value<0); }
inline double noteFrequency(double note) noexcept { return 440.0*std::exp2((note-69)/12); }
inline double frequencyNote(double hz) noexcept { return 69+12*std::log2(std::max(hz,1.0e-9)/440.0); }
inline PitchSettings validPitchSettings(PitchSettings settings) noexcept
{
    const auto time=[](double v,double fallback){return std::isfinite(v)?std::clamp(v,.001,5.0):fallback;};
    settings.glide.seconds=time(settings.glide.seconds,.15);settings.autobend.seconds=time(settings.autobend.seconds,.18);
    settings.autobend.depth=std::isfinite(settings.autobend.depth)?std::clamp(settings.autobend.depth,-36.0,36.0):0;
    settings.glide.target=static_cast<PitchTarget>(std::clamp(static_cast<int>(settings.glide.target),0,4));
    settings.autobend.target=static_cast<PitchTarget>(std::clamp(static_cast<int>(settings.autobend.target),0,4));
    settings.glide.curve=static_cast<GlideCurve>(std::clamp(static_cast<int>(settings.glide.curve),0,2));return settings;
}
inline bool samePitchSettings(const PitchSettings& a,const PitchSettings& b) noexcept
{
    return a.glide.enabled==b.glide.enabled&&samePitchValue(a.glide.seconds,b.glide.seconds)&&a.glide.target==b.glide.target
        &&a.glide.legatoOnly==b.glide.legatoOnly&&a.glide.curve==b.glide.curve&&a.autobend.enabled==b.autobend.enabled
        &&samePitchValue(a.autobend.seconds,b.autobend.seconds)&&samePitchValue(a.autobend.depth,b.autobend.depth)
        &&a.autobend.target==b.autobend.target&&a.autobend.phraseOnly==b.autobend.phraseOnly&&a.legato==b.legato;
}

// All motion clocks advance once per HOST sample, independently of render blocks.
class GlideMotion
{
public:
    void begin(double note,double origin,double duration,GlideCurve shape,double sampleRate) noexcept
    {
        targetHz=noteFrequency(note);targetNote=note;currentHz=origin>0?origin:targetHz;
        startNote=frequencyNote(currentHz);elapsed=0;rate=sampleRate;seconds=duration;curve=shape;
        coefficient=1-std::exp(-1/(rate*seconds));
        running=!samePitchValue(currentHz,targetHz);
    }
    void retime(double duration,GlideCurve shape) noexcept
    {
        if(samePitchValue(seconds,duration)&&curve==shape)return;
        // Time changes preserve progress; changing the curve rebases at the
        // current pitch rather than jumping to a different point on that curve.
        if(curve==shape)elapsed*=duration/seconds;
        else{startNote=frequencyNote(currentHz);elapsed=0;}
        seconds=duration;curve=shape;
        coefficient=1-std::exp(-1/(rate*seconds));
    }
    void advance() noexcept
    {
        if(!running)return;
        if(curve==GlideCurve::classic)
        {
            currentHz+=coefficient*(targetHz-currentHz);
            running=!samePitchValue(currentHz,targetHz);return;
        }
        elapsed+=1/rate;const auto progress=std::min(1.0,elapsed/seconds);
        const auto position=curve==GlideCurve::linear?progress:progress*progress*(3-2*progress);
        currentHz=progress>=1?targetHz:noteFrequency(startNote+(targetNote-startNote)*position);
        if(progress>=1)running=false;
    }
    double frequency() const noexcept { return currentHz; }
    double destination() const noexcept { return targetHz; }
private:
    double currentHz=261.625565,targetHz=261.625565,startNote=60,targetNote=60;
    double elapsed=0,seconds=.15,rate=48000,coefficient=0;
    GlideCurve curve=GlideCurve::smooth;
    bool running=false;
};

class AutobendMotion
{
public:
    void trigger(double duration,double sampleRate) noexcept { seconds=duration;rate=sampleRate;elapsed=0;amount=1; }
    void retime(double duration) noexcept
    {
        if(!samePitchValue(seconds,duration)){elapsed*=duration/seconds;seconds=duration;}
    }
    void advance() noexcept
    {
        if(pitchIsZero(amount))return;
        elapsed+=1/rate;const auto remaining=std::max(0.0,1-elapsed/seconds);
        amount=remaining*remaining*remaining;
    }
    double value() const noexcept { return amount; }
private:
    double seconds=.18,rate=48000,elapsed=0,amount=0;
};

// Destination/enable changes on held voices interpolate routing, not oscillator phase.
class PitchRouting
{
public:
    static std::array<double,3> weights(bool enabled,PitchTarget target) noexcept
    {
        if(!enabled)return{};
        switch(target)
        {
            case PitchTarget::carrier:return{1,0,0};
            case PitchTarget::modulators:return{0,1,0};
            case PitchTarget::opposed:return{1,-1,0};
            case PitchTarget::tone:return{1,1,1};
            case PitchTarget::all:return{1,1,0};
        }
        return{};
    }
    void reset(bool enabled,PitchTarget target) noexcept { current=destination=weights(enabled,target);remaining=0; }
    void set(bool enabled,PitchTarget target,double rate) noexcept
    {
        const auto next=weights(enabled,target);if(next==destination)return;
        destination=next;remaining=std::max(1,static_cast<int>(std::ceil(rate*.003)));
        for(size_t i=0;i<3;++i)step[i]=(destination[i]-current[i])/remaining;
    }
    const std::array<double,3>& advance() noexcept
    {
        if(remaining>0){for(size_t i=0;i<3;++i)current[i]+=step[i];if(--remaining==0)current=destination;}
        return current;
    }
    bool silent() const noexcept { return remaining==0&&pitchIsZero(current[0])&&pitchIsZero(current[1])&&pitchIsZero(current[2]); }
private:
    std::array<double,3> current{},destination{},step{};
    int remaining=0;
};
}
