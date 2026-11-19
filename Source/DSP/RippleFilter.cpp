#include "RippleFilter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Nitride::DSP
{
namespace
{
// RMS of the existing waveshaper for a unit-peak sine, divided by sqrt(.5).
// Baked 8192-phase quadrature at 65 Drive positions; linear interpolation is
// only a gain-control calibration, not an approximation of the audio path.
constexpr std::array<double,65> shapedRms{
    1,.99706465455770921,.99416998713140392,.99134995863647557,
    .98862701902639061,.98600766396627659,.98347949169127813,.98101004201622199,
    .97854753205064537,.97602341470954701,.97335649382888634,.9704581573269736,
    .96723816505152238,.96361037577712227,.95949783226316798,.95483674067637958,
    .94957905857309943,.94369360833580374,.93716582168883233,.9299963643970246,
    .92219897169053011,.91379784354989346,.90482491649605779,.89531726335037698,
    .88531479363329002,.87485835041906324,.86398823470201658,.8527431403941591,
    .84115945245119939,.82927084509205473,.81710811313369358,.80469917337610386,
    .79206918142622107,.77924071974790765,.76623402324775558,.7530672182361835,
    .73975655857660116,.72631664908821503,.71276065088929741,.69910046658521141,
    .68534690530039977,.67150982880609533,.65759828065332115,.64362060048501413,
    .62958452572466383,.61549728273106319,.60136566934337021,.5871961305639829,
    .5729948289667709,.55876771128940583,.54452057257885922,.53025911921145252,
    .51598903210562075,.50171603148842836,.48744594466930796,.47318477842158752,
    .45893879778201202,.44471461336127843,.43051927962948477,.41636040711801936,
    .40224629208824342,.38818606798690247,.37418988397504566,.36026911702320935,
    .34643662555692228};
}
double RippleFilter::makeupGain(RippleMode mode,double dry,double drive,double feedback) noexcept
{
    dry=std::clamp(dry,0.,1.);drive=std::clamp(drive,0.,1.);feedback=std::clamp(feedback,-1.,1.);
    double wetPower=0;
    if(mode==RippleMode::feedback)
    {
        // Phase-averaged power of the existing (1-|g|)*A/(1-g*A) loop.
        const auto g=.94*std::abs(feedback);wetPower=(1-g)/(1+g);
    }
    else
    {
        const auto position=drive*64;const auto index=std::min(static_cast<size_t>(position),shapedRms.size()-2);
        const auto ratio=shapedRms[index]+(position-static_cast<double>(index))*(shapedRms[index+1]-shapedRms[index]);
        wetPower=ratio*ratio;
    }
    // Additive Dry is included in the energy budget. One scalar multiplies BOTH
    // branches, preserving their ratio, phase cancellation and spectral shape.
    // Unity wet-only linear gain; <=4x post-sum makeup, not per-sample limiting.
    return 1/std::sqrt(std::max(.0625,wetPower+dry*dry));
}
void RippleFilter::prepare(double rate,RippleType type,RippleMode mode) noexcept
{
    sampleRate=std::isfinite(rate)&&rate>0?rate:192000;
    transitionLength=std::max(1,static_cast<int>(std::ceil(sampleRate*.005)));
    target=2*std::clamp(static_cast<int>(type),0,2)+std::clamp(static_cast<int>(mode),0,1);reset();
}
void RippleFilter::reset() noexcept
{banks={};weights={};weights[static_cast<size_t>(target)]=1;steps={};remaining=0;lastFrequency=lastResonance=-1;lastDry=lastDrive=-1;lastFeedback=2;makeup={1,1};diagnostic={};}
void RippleFilter::configure(RippleType type,RippleMode mode) noexcept
{
    const auto next=2*std::clamp(static_cast<int>(type),0,2)+std::clamp(static_cast<int>(mode),0,1);
    if(next==target)return;
    target=next;
    if(weights[static_cast<size_t>(target)]==0)banks[static_cast<size_t>(target)]={};
    remaining=transitionLength;
    for(size_t i=0;i<weights.size();++i)steps[i]=((i==static_cast<size_t>(target)?1.:0.)-weights[i])/remaining;
}
double RippleFilter::cascade(Bank& bank,int count,double x) noexcept
{
    for(int i=0;i<count;++i)
    {
        auto& s=bank.state[static_cast<size_t>(i)];
        const auto v=k2*s.outer+t2*s.inner;
        const auto inner=t2*s.outer-k2*s.inner;
        const auto y=k1*x+t1*v;
        s.outer=t1*x-k1*v;s.inner=inner;x=y;
    }
    return x;
}
double RippleFilter::process(double input,double frequency,double resonance,double dry,double drive,double feedback) noexcept
{
    if(!std::isfinite(input)||!std::isfinite(frequency)||!std::isfinite(resonance)||!std::isfinite(dry)
        ||!std::isfinite(drive)||!std::isfinite(feedback)||std::abs(input)>1e6)
    {const auto faults=diagnostic.faults+1;reset();diagnostic.faults=faults;return 0;}
    frequency=std::clamp(frequency,10.,sampleRate*.45);resonance=std::clamp(resonance,0.,.85);
    if(frequency!=lastFrequency||resonance!=lastResonance)
    {
        const auto angle=2*std::numbers::pi*frequency/sampleRate;
        const auto r=resonance/.85,q=.5+15.5*r*r;
        t2=std::sin(angle);k2=-std::cos(angle);
        const auto alpha=t2/(2*q);k1=(1-alpha)/(1+alpha);t1=2*std::sqrt(alpha)/(1+alpha);
        lastFrequency=frequency;lastResonance=resonance;
    }
    dry=std::clamp(dry,0.,1.);drive=std::clamp(drive,0.,1.);feedback=std::clamp(feedback,-1.,1.);
    if(dry!=lastDry||drive!=lastDrive||feedback!=lastFeedback)
    {
        makeup={makeupGain(RippleMode::waveshape,dry,drive,feedback),makeupGain(RippleMode::feedback,dry,drive,feedback)};
        lastDry=dry;lastDrive=drive;lastFeedback=feedback;
    }
    if(remaining>0)
    {
        for(size_t i=0;i<weights.size();++i)weights[i]+=steps[i];
        if(--remaining==0){weights={};weights[static_cast<size_t>(target)]=1;}
    }
    double output=0;
    for(size_t b=0;b<banks.size();++b)if(weights[b]>0)
    {
        auto& bank=banks[b];const auto count=2<<static_cast<int>(b/2);double value=0;
        if(b%2==1)
        {
            // A(u)=D*u+S. Solve u=(1-|g|)*x+g*A(u) without an added delay.
            // The explicit input attenuation bounds the static wet gain by one;
            // it is a documented feedback gain law, not signal-dependent AGC.
            const auto g=.94*feedback;double d=1,s=0;
            for(int i=0;i<count;++i)
            {
                const auto& state=bank.state[static_cast<size_t>(i)];
                s=k1*s+t1*(k2*state.outer+t2*state.inner);d*=k1;
            }
            const auto excitation=((1-std::abs(g))*input+g*s)/(1-g*d);
            value=cascade(bank,count,excitation);
            diagnostic.maximumResidual=std::max(diagnostic.maximumResidual,std::abs(excitation-((1-std::abs(g))*input+g*value)));
        }
        else
        {
            value=cascade(bank,count,input);
            if(drive>0)
            {
                const auto gain=1+15*drive*drive;
                value+=drive*(std::tanh(gain*value)/std::sqrt(gain)-value);
            }
        }
        output+=weights[b]*makeup[b%2]*(value+dry*input);
    }
    // Complete gain-staged banks crossfade, avoiding a mode-change gain step.
    // No input-level detector, AGC, notch fill, new limiter or drive reduction.
    if(!std::isfinite(output)||std::abs(output)>1e8)
    {const auto faults=diagnostic.faults+1;reset();diagnostic.faults=faults;return 0;}
    return output;
}
}
