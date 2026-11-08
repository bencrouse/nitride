#include "RageFilter.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride::DSP;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<double> render(RageCharacter character,double amount,double resonance,double rate=192000)
{
    RageFilter f;f.setCharacter(character);f.prepare(rate);std::vector<double> audio;
    for(int i=0;i<static_cast<int>(rate*.15);++i)
    {
        const auto t=i/rate,input=.45*std::sin(2*std::numbers::pi*997*t)+.15*std::sin(2*std::numbers::pi*2303*t);
        audio.push_back(f.process(input,1400,resonance,amount));
    }
    require(f.diagnostics().faults==0,"Rage generated a numerical fault");
    require(f.diagnostics().maximumResidual<1e-7,"Rage zero-delay solve did not converge");return audio;
}
double gainMatchedDifference(const std::vector<double>& a,const std::vector<double>& b)
{
    double dot=0,power=0,reference=0;
    for(size_t i=0;i<a.size();++i){dot+=a[i]*b[i];power+=b[i]*b[i];reference+=a[i]*a[i];}
    const auto gain=dot/std::max(power,1e-30);double error=0;
    for(size_t i=0;i<a.size();++i){const auto d=a[i]-gain*b[i];error+=d*d;}
    return std::sqrt(error/std::max(reference,1e-30));
}
double harmonics(RageCharacter character,double amount,double level=.6)
{
    constexpr int count=8192,period=128;
    RageFilter filter;filter.setCharacter(character);filter.prepare(192000);
    std::array<double,13> real{},imaginary{};
    for(int n=-count*16;n<count;++n)
    {
        const auto phase=2*std::numbers::pi*n/period;
        const auto y=filter.process(level*std::sin(phase),1500,.8,amount);
        if(n>=0)for(size_t h=1;h<real.size();++h){real[h]+=y*std::cos(phase*h);imaginary[h]+=y*std::sin(phase*h);}
    }
    double harmonic=0;for(size_t h=2;h<real.size();++h)harmonic+=real[h]*real[h]+imaginary[h]*imaginary[h];
    return std::sqrt(harmonic/std::max(1e-30,real[1]*real[1]+imaginary[1]*imaginary[1]));
}
double resonancePeak(RageCharacter character,double amount,double resonance)
{
    double peak=0;
    for(int bin=32;bin<=144;bin+=4)
    {
        RageFilter filter;filter.setCharacter(character);filter.prepare(192000);double energy=0;
        for(int n=-16384;n<8192;++n)
        {
            const auto input=.3*std::sin(2*std::numbers::pi*bin*n/8192);
            const auto y=filter.process(input,1500,resonance,amount);if(n>=0)energy+=y*y;
        }
        require(filter.diagnostics().faults==0,"Resonance sweep needed numerical recovery");
        peak=std::max(peak,std::sqrt(energy/8192));
    }
    return peak;
}
struct HarmonicIdentity {double even=0,odd=0,high=0;};
HarmonicIdentity identity(RageCharacter character)
{
    RageFilter filter;filter.setCharacter(character);filter.prepare(192000);
    std::array<double,17> real{},imaginary{};
    for(int n=-8192*8;n<8192;++n)
    {
        const auto phase=2*std::numbers::pi*n/256;
        const auto y=filter.process(.03*std::sin(phase),5000,.55,1);
        if(n>=0)for(size_t h=1;h<real.size();++h){real[h]+=y*std::cos(phase*h);imaginary[h]+=y*std::sin(phase*h);}
    }
    HarmonicIdentity result;const auto fundamental=real[1]*real[1]+imaginary[1]*imaginary[1];
    for(size_t h=2;h<real.size();++h)
    {
        const auto power=(real[h]*real[h]+imaginary[h]*imaginary[h])/std::max(fundamental,1e-30);
        (h%2==0?result.even:result.odd)+=power;if(h>=5)result.high+=power;
    }
    return result;
}
double conditionedAmplitude(RageCharacter character,bool conditioned)
{
    RageFilter filter;filter.setCharacter(character);filter.prepare(192000);double real=0,imaginary=0;
    for(int n=-53760;n<10496;++n)
    {
        const auto phase=2*std::numbers::pi*n/256;
        const auto level=n<0?(conditioned&&n<-15360?.2:0):.02;
        const auto y=filter.process(level*std::sin(phase),5000,.4,1);
        if(n>=2304){real+=y*std::cos(phase);imaginary+=y*std::sin(phase);}
    }
    require(filter.diagnostics().faults==0,"Character memory probe needed numerical recovery");
    return std::hypot(real,imaginary);
}
void identities()
{
    const auto distortion=identity(RageCharacter::distortion),soft=identity(RageCharacter::softclip);
    const auto tape=identity(RageCharacter::tape),transistor=identity(RageCharacter::transistor);
    require(distortion.odd>50*soft.odd,"Distortion and Softclip lost their odd-harmonic contrast");
    require(transistor.even>.002&&transistor.high>4*distortion.high,"Transistor lost its asymmetric bright edge");
    require(tape.high<.25*distortion.high,"Tape lost its rounded upper-harmonic response");
    for(int c=1;c<5;++c)
    {
        const auto character=static_cast<RageCharacter>(c);
        const auto ratio=conditionedAmplitude(character,true)/conditionedAmplitude(character,false);
        if(character==RageCharacter::tape)require(ratio<.95&&ratio>.7,"Tape lost its distinct, bounded compression memory");
        else require(std::abs(ratio-1)<.005,"Non-Tape character acquired long compression memory");
        std::cout<<"Character "<<c<<" conditioned/fresh amplitude "<<ratio<<'\n';
    }
}
}
int main()
{
    try
    {
        const auto clean=render(RageCharacter::diode,0,.72);
        std::array<std::vector<double>,5> driven;
        for(int c=0;c<5;++c)
        {
            const auto character=static_cast<RageCharacter>(c);
            require(render(character,0,.72)==clean,"Zero Rage changed the linear core between characters");
            // Zero-resonance neutrality was an eval-039/040 topology assumption,
            // not a host contract. The integrator feedback is now nonlinear even
            // without extra regeneration; it is still inside, not after, the LP.
            require(gainMatchedDifference(render(character,0,0),render(character,1,0))>.05,"Integrator return lost its nonlinear colour at zero resonance");
            driven[static_cast<size_t>(c)]=render(character,.8,.72);
            const auto full=render(character,1,.72),half=render(character,.5,.72);
            require(gainMatchedDifference(clean,half)>.10&&gainMatchedDifference(clean,full)>.25,"Rage range became a subtle level change");
            const auto thd=harmonics(character,1);require(thd>.01,"Full Rage failed to generate meaningful feedback harmonics");
            require(harmonics(character,0)<1e-5,"Clean Rage core generated nonlinear harmonics");
            std::cout<<"Character "<<c<<": full-drive THD "<<20*std::log10(thd)<<" dB, gain-matched change "<<gainMatchedDifference(clean,full)<<'\n';
            const auto quietThd=harmonics(character,1,.03);
            require(c==0||quietThd>.01,"Character became inactive at quiet oscillator levels");
            std::cout<<"  Quiet-input THD "<<20*std::log10(quietThd)<<" dB\n";
            // A large waveform residual also passed the unsuccessful damping
            // design. Require a retained resonant peak, allowing the nonlinear
            // return to move its frequency rather than testing one fixed bin.
            const auto peak=resonancePeak(character,1,.8),flat=resonancePeak(character,1,0);
            require(peak>2*flat,"Full Rage erased the resonant peak");
            require(peak>.5*resonancePeak(character,0,.8),"Full Rage collapsed resonant energy");
            std::cout<<"  Resonant peak "<<peak<<", resonance emphasis "<<20*std::log10(peak/flat)<<" dB\n";
            for(int other=0;other<c;++other)
            {
                double energy=0;for(size_t i=0;i<clean.size();++i){const auto d=driven[static_cast<size_t>(c)][i]-driven[static_cast<size_t>(other)][i];energy+=d*d;}
                require(energy/clean.size()>1e-6,"Rage characters collapsed to the same transfer response");
                require(gainMatchedDifference(driven[static_cast<size_t>(c)],driven[static_cast<size_t>(other)])>.02,"Rage character difference disappeared after level alignment");
            }
        }
        for(const auto rate:{8000.0,44100.0,48000.0,96000.0,192000.0,384000.0})for(int c=0;c<5;++c)
        {
            RageFilter f;f.setCharacter(static_cast<RageCharacter>(c));f.prepare(rate);
            std::uint32_t random=17;
            for(int i=0;i<12000;++i)
            {
                random=random*1664525u+1013904223u;const auto x=8*(static_cast<double>(random)/4294967295.0-.5);
                const auto cutoff=10*std::pow(std::max(1.0,rate*.04),static_cast<double>(i%101)/100);
                const auto y=f.process(x,cutoff,static_cast<double>(i%86)/100,static_cast<double>(i%101)/100);
                require(std::isfinite(y)&&std::abs(y)<100,"Extreme Rage controls became unstable");
                if(i%31==0)f.setCharacter(static_cast<RageCharacter>((i/31)%5));
            }
            require(f.diagnostics().faults==0,"Ordinary extremes required emergency resets");
            require(f.diagnostics().maximumResidual<1e-6,"Extreme Rage solver residual exceeded guard");
        }
        for(const auto rate:{48000.0,192000.0,384000.0})for(int c=0;c<5;++c)for(const auto cutoff:{40.0,1800.0,18000.0})
        {
            RageFilter filter;filter.setCharacter(static_cast<RageCharacter>(c));filter.prepare(rate);
            double tail=0;
            for(int n=0;n<static_cast<int>(rate*3);++n)
            {
                const auto y=filter.process(n<256?.6*std::sin(n*.13):0,cutoff,.85,1);
                if(n>rate*3-4096)tail=std::max(tail,std::abs(y));
            }
            if(filter.diagnostics().faults||tail>=1e-7)std::cerr<<"Tail character "<<c<<", cutoff "<<cutoff<<": "<<tail<<'\n';
            require(filter.diagnostics().faults==0&&tail<1e-7,"Stronger feedback retained a self-oscillating/unstable tail");
        }
        identities();
        RageFilter reset;reset.prepare(192000);const auto value=reset.process(.5,1000,.7,.8);reset.reset();
        require(reset.process(.5,1000,.7,.8)==value,"Rage reset is nondeterministic");
        require(reset.process(std::numeric_limits<double>::quiet_NaN(),1000,.7,.8)==0&&reset.diagnostics().faults==1,"Rage fault containment failed");
        std::cout<<"Rage core passed: five distinct feedback characters, neutral-drive agreement, bounded solve, switching, extremes and deterministic reset\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
