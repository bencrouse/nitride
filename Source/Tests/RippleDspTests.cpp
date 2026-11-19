#include "RippleFilter.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

using namespace Nitride::DSP;
namespace
{
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
void linear()
{
    constexpr int size=65536;constexpr double rate=192000;
    for(int type=0;type<3;++type)
    {
        RippleFilter f;f.prepare(rate,static_cast<RippleType>(type));
        double energy=0;std::array<std::complex<double>,6> bins{};
        constexpr std::array frequencies{100.,500.,1000.,3000.,12000.,40000.};
        for(int n=0;n<size;++n)
        {
            const auto y=f.process(n==0?1:0,3000,.5,0,0,0);energy+=y*y;
            for(size_t i=0;i<bins.size();++i)bins[i]+=y*std::polar(1.,-2*std::numbers::pi*frequencies[i]*n/rate);
        }
        require(std::abs(energy-1)<1e-10,"Static all-pass energy changed");
        for(const auto value:bins)require(std::abs(std::abs(value)-1)<1e-9,"Static all-pass magnitude is not unity");
        require(f.diagnostics().faults==0,"Linear all-pass fault");
    }
}
void modulation()
{
    for(int type=0;type<3;++type)for(int mode=0;mode<2;++mode)
    {
        RippleFilter f;f.prepare(192000,static_cast<RippleType>(type),static_cast<RippleMode>(mode));
        double energy=0,peak=0;
        for(int n=0;n<400000;++n)
        {
            if(n%5000==0)f.configure(static_cast<RippleType>((type+n/5000)%3),static_cast<RippleMode>((mode+n/5000)%2));
            const auto x=n<200000?.4*std::sin(n*.073)+.2*std::sin(n*.031):0.;
            const auto y=f.process(x,2000*std::exp2(4*std::sin(n*.013)),.85,.5,.8,.99*std::sin(n*.0009));
            require(std::isfinite(y),"Nonfinite modulated Ripple");peak=std::max(peak,std::abs(y));if(n>390000)energy+=y*y;
        }
        require(peak>.05&&peak<4,"Ripple silent or excessive level");require(energy<1e-8,"Ripple tail did not decay");
        require(f.diagnostics().faults==0&&f.diagnostics().maximumResidual<1e-9,"Ripple modulation/feedback recovery or residual failure");
    }
}
void movingEnergy()
{
    RippleFilter f;f.prepare(192000,RippleType::hard);double input=0,output=0;
    for(int n=0;n<300000;++n)
    {
        const auto x=n<100000?.4*std::sin(n*.17):0.;input+=x*x;
        const auto y=f.process(x,1000*std::exp2(4*std::sin(n*.04)),.85,0,0,0);output+=y*y;
    }
    require(std::abs(input-output)/input<1e-10,"Moving normalized lattice created energy");
}
void transfer()
{
    constexpr double rate=192000,center=3000,resonance=.5;
    for(int type=0;type<3;++type)for(const auto feedback:{-1.,0.,1.})for(const auto dry:{0.,.5,1.})
    {
        RippleFilter f;f.prepare(rate,static_cast<RippleType>(type),RippleMode::feedback);
        constexpr std::array frequencies{100.,1000.,3000.,7000.,20000.};std::array<std::complex<double>,5> actual{};
        for(int n=0;n<1048576;++n)
        {
            const auto y=f.process(n==0?1:0,center,resonance,dry,0,feedback);
            for(size_t i=0;i<actual.size();++i)actual[i]+=y*std::polar(1.,-2*std::numbers::pi*frequencies[i]*n/rate);
        }
        const auto angle=2*std::numbers::pi*center/rate,q=.5+15.5*std::pow(resonance/.85,2),alpha=std::sin(angle)/(2*q);
        const auto k=(1-alpha)/(1+alpha),b=-2*std::cos(angle)/(1+alpha),g=.94*feedback;
        for(size_t i=0;i<actual.size();++i)
        {
            const auto z=std::polar(1.,-2*std::numbers::pi*frequencies[i]/rate);
            const auto a=std::pow((k+b*z+z*z)/(1.+b*z+k*z*z),RippleFilter::stages(static_cast<RippleType>(type)));
            const auto expected=RippleFilter::makeupGain(RippleMode::feedback,dry,0,feedback)*((1-std::abs(g))*a/(1.-g*a)+dry);
            if(std::abs(actual[i]-expected)>=1e-7)std::cerr<<"Transfer type="<<type<<" feedback="<<feedback<<" dry="<<dry<<" Hz="<<frequencies[i]<<" error="<<std::abs(actual[i]-expected)<<'\n';
            require(std::abs(actual[i]-expected)<1e-7,"Ripple phase/dry/signed-feedback analytical transfer differs");
        }
        require(f.diagnostics().maximumResidual<1e-10,"Algebraic feedback solve residual");
    }
}
void gainStaging()
{
    require(RippleFilter::makeupGain(RippleMode::waveshape,0,0,0)==1,"Linear wet-only path is not unity gain");
    for(int mode=0;mode<2;++mode)for(int d=0;d<=100;++d)for(const auto dry:{0.,.05,.25,.5,1.})
    {
        const auto drive=d/100.,feedback=2*drive-1;
        const auto gain=RippleFilter::makeupGain(static_cast<RippleMode>(mode),dry,drive,feedback);
        require(gain>=.7&&gain<=4,"Makeup gain escaped its fixed bounds");
        if(mode==1)
        {
            const auto g=.94*std::abs(feedback),power=(1-g)/(1+g);
            require(std::abs(gain-1/std::sqrt(std::max(.0625,power+dry*dry)))<1e-14,"Feedback power calibration differs");
            require(gain==RippleFilter::makeupGain(RippleMode::feedback,dry,1-drive,-feedback),"Dormant Drive or feedback sign changed calibration");
        }
        else require(gain==RippleFilter::makeupGain(RippleMode::waveshape,dry,drive,-feedback),"Dormant Feedback changed waveshaper calibration");
    }
    // The gain law is calibrated, not an RMS detector. Verify the actual unchanged
    // waveshaper response between table knots and across all cascade depths.
    for(int type=0;type<3;++type)for(int step=0;step<=32;++step)
    {
        const auto drive=step/32.+(step<32?1./128:0.);
        RippleFilter f;f.prepare(192000,static_cast<RippleType>(type));double energy=0;
        for(int n=-16384;n<8192;++n)
        {
            const auto x=std::sin(2*std::numbers::pi*n/128);
            const auto y=f.process(x,3000,.5,0,drive,0);if(n>=0)energy+=y*y;
        }
        require(std::abs(10*std::log10(energy/4096))<.002,"Nominal waveshaper gain calibration lost RMS level");
    }
    // Linear feedback must scale identically for quiet/loud input and through
    // silence/tails. An audio-dependent makeup/AGC would violate this identity.
    for(const auto feedback:{-1.,-.9,0.,.9,1.})
    {
        RippleFilter loud,quiet;loud.prepare(192000,RippleType::hard,RippleMode::feedback);quiet.prepare(192000,RippleType::hard,RippleMode::feedback);
        for(int n=0;n<60000;++n)
        {
            const auto x=n<30000?.7*std::sin(n*.073):0.;const auto frequency=500*std::exp2(2*std::sin(n*.0001));
            const auto a=loud.process(x,frequency,.7,.1,0,feedback),b=quiet.process(x*.1,frequency,.7,.1,0,feedback);
            require(std::abs(b-.1*a)<1e-11,"Makeup follows input level or pumps through silence");
        }
    }
}
void boundaries()
{
    for(const auto rate:{44100.,192000.,384000.,768000.})for(int type=0;type<3;++type)for(int mode=0;mode<2;++mode)
    {
        RippleFilter f;f.prepare(rate,static_cast<RippleType>(type),static_cast<RippleMode>(mode));
        for(int n=0;n<20000;++n)
        {
            const auto y=f.process(.9*std::sin(n*.17),n%2?10:rate*.45,n%3?.85:0,n%2,n%2,n%2?-1:1);
            require(std::isfinite(y)&&std::abs(y)<8,"Ripple extreme controls/rate failed");
        }
        require(f.diagnostics().faults==0,"Ripple extremes triggered safety recovery");
    }
    RippleFilter linear,tiny,driven;linear.prepare(192000);tiny.prepare(192000);driven.prepare(192000);
    double difference=0;
    for(int n=0;n<40000;++n)
    {
        const auto x=.8*std::sin(n*.073),a=linear.process(x,1700,.6,.5,0,0),b=tiny.process(x,1700,.6,.5,1e-9,0);
        const auto c=driven.process(x,1700,.6,.5,1,0);
        require(std::abs(a-b)<1e-9,"Drive does not approach identity continuously at zero");difference+=std::abs(c-a);
    }
    require(difference>10,"Drive has no audible-scale effect");
    require(linear.process(std::numeric_limits<double>::quiet_NaN(),1000,0,0,0,0)==0&&linear.diagnostics().faults==1,"Invalid input recovery not reported");
}
}
int main()
{
    try{linear();movingEnergy();transfer();gainStaging();boundaries();modulation();std::cout<<"Ripple unity all-pass, analytical phase/dry/signed feedback, calibrated bounded control-only makeup/no AGC, zero-drive continuity, extreme controls/rates, moving-state energy, rapid switching and tails passed\n";}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}
