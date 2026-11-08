#include "RageFilter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Nitride::DSP
{
namespace
{
constexpr std::array excitationSensitivity{0.0,3.0,.9,2.0,4.0};
constexpr std::array excitationLevel{0.0,.22,.16,.18,.14};
double characterDrive(RageCharacter character,double drive) noexcept
{
    constexpr std::array scale{.8,8.0/15,1.0/3,4.0/15,8.0/15};
    return 1+(drive-1)*scale[static_cast<size_t>(character)];
}
}
void RageFilter::prepare(double rate) noexcept
{
    sampleRate=std::isfinite(rate)&&rate>0?rate:192000;
    dcPole=std::exp(-2*std::numbers::pi*3/sampleRate);
    envelopeAttack=-std::expm1(-1/(sampleRate*.012));envelopeRelease=-std::expm1(-1/(sampleRate*.160));
    tapeAlpha=-std::expm1(-2*std::numbers::pi*2400/sampleRate);
    distortionAlpha=-std::expm1(-2*std::numbers::pi*900/sampleRate);
    transistorAlpha=-std::expm1(-2*std::numbers::pi*1500/sampleRate);
    transistorEnvelopeAlpha=-std::expm1(-1/(sampleRate*.002));
    transitionLength=std::max(1,static_cast<int>(std::ceil(sampleRate*.004)));
    reset();
}
void RageFilter::reset() noexcept
{
    state1=state2=previousBand=tapeEnvelope=dcInput=dcOutput=0;
    tapeTone=distortionTone=transistorLow=transistorEnvelope=0;
    cachedCutoff=-1;transition=0;weights={};weights[static_cast<size_t>(target)]=1;weightStep={};diagnostic={};
}
void RageFilter::setCharacter(RageCharacter character) noexcept
{
    character=static_cast<RageCharacter>(std::clamp(static_cast<int>(character),0,4));
    if(character==target)return;
    target=character;transition=transitionLength;
    for(size_t i=0;i<weights.size();++i)weightStep[i]=((i==static_cast<size_t>(target)?1.0:0.0)-weights[i])/transition;
}
[[gnu::always_inline]] inline RageFilter::Colour RageFilter::colour(RageCharacter character,double z,bool primitive) noexcept
{
    switch(character)
    {
        case RageCharacter::diode:
        {
            const auto knee=z>=0?.28:.85,denominator=1+std::abs(z)/knee;
            const auto magnitude=std::abs(z);
            return{z/denominator,1/(denominator*denominator),primitive?knee*(magnitude-knee*std::log1p(magnitude/knee)):0};
        }
        case RageCharacter::distortion:
        {
            const auto limited=std::clamp(z,-1.0,1.0);
            const auto magnitude=std::abs(z),square=limited*limited;
            return{limited-limited*limited*limited/3,magnitude<1?1-z*z:0,
                primitive?(magnitude<=1?square*(.5-square/12):5.0/12+(2.0/3)*(magnitude-1)):0};
        }
        case RageCharacter::softclip:
        {
            const auto denominator=std::sqrt(1+z*z);
            return{z/denominator,1/(denominator*denominator*denominator),primitive?z*z/(denominator+1):0};
        }
        case RageCharacter::tape:
        {
            const auto angle=std::atan(z);
            return{angle,1/(1+z*z),primitive?z*angle-.5*std::log1p(z*z):0};
        }
        case RageCharacter::transistor:
        {
            // Biased hard plateaus with narrow C2 knees rather than discontinuous
            // slopes. Keeps the switching edge while limiting generated aliases.
            constexpr auto width=.35;const auto magnitude=std::abs(z),knee=z>=0?.85:.45;
            if(magnitude<=knee-width)return{z,1,primitive?.5*z*z:0};
            if(magnitude>=knee+width)return{std::copysign(knee,z),0,primitive?knee*magnitude-.5*knee*knee-.1*width*width:0};
            else
            {
                const auto t=(magnitude-knee+width)/(2*width),t2=t*t,t3=t2*t;
                return{std::copysign(magnitude-width*t3*(2-t),z),1-3*t2+2*t3,
                    primitive?.5*z*z-width*width*t3*t*(1-.4*t):0};
            }
        }
    }
    return{z,1,primitive?.5*z*z:0};
}
[[gnu::always_inline]] inline RageFilter::Shape RageFilter::excitationShape(RageCharacter character,double z) noexcept
{
    switch(character)
    {
        case RageCharacter::distortion:
        {
            // Cascaded balanced cubic stages emphasize odd-harmonic body. Keep
            // the established restoring-force curve separate from this voicing.
            const auto limited=std::clamp(z,-1.0,1.0),square=limited*limited;
            const auto first=limited-limited*square/3,second=1.5*first;
            return{first-.75*first*first*first,std::abs(z)<1?(1-square)*(1-second*second):0};
        }
        case RageCharacter::softclip:
        {
            const auto value=std::tanh(z);return{value,1-value*value};
        }
        case RageCharacter::tape:return{std::atan(z),1/(1+z*z)};
        case RageCharacter::transistor:
        {
            constexpr auto width=.16;const auto magnitude=std::abs(z),knee=z>=0?.95:.24;
            if(magnitude<=knee-width)return{z,1};
            if(magnitude>=knee+width)return{std::copysign(knee,z),0};
            const auto t=(magnitude-knee+width)/(2*width),t2=t*t,t3=t2*t;
            return{std::copysign(magnitude-width*t3*(2-t),z),1-3*t2+2*t3};
        }
        case RageCharacter::diode:break;
    }
    return{z,1};
}
[[gnu::always_inline]] inline RageFilter::Shape RageFilter::shape(RageCharacter character,double value,double previousState) const noexcept
{
    const auto& context=returnContext[static_cast<size_t>(character)];
    const auto next=context.drive*(2*value-previousState),delta=next-context.previous;
    double average=0,derivative=0;
    if(std::abs(delta)<1e-3*(1+std::abs(next)+std::abs(context.previous)))
    {
        // Avoid cancellation in the primitive divided difference for small steps.
        // Two-point Gaussian quadrature has O(delta^4) error, including its slope.
        constexpr auto t0=.21132486540518711775,t1=1-t0;
        const auto left=colour(character,context.previous+t0*delta,false),right=colour(character,context.previous+t1*delta,false);
        average=.5*(left.value+right.value);
        derivative=context.drive*(t0*left.derivative+t1*right.derivative);
    }
    else
    {
        const auto endpoint=colour(character,next);
        average=(endpoint.primitive-context.primitive)/delta;
        derivative=2*context.drive*(endpoint.value-average)/delta;
    }
    return{context.compression*average,context.compression*std::max(0.0,derivative)};
}
[[gnu::always_inline]] inline RageFilter::Shape RageFilter::feedback(double value,double previousState,double amount) const noexcept
{
    if(amount<=0)return{value,1};
    Shape result{};
    if(transition==0)result=shape(target,value,previousState);
    else for(size_t i=0;i<weights.size();++i)if(weights[i]>0)
    {
        const auto part=shape(static_cast<RageCharacter>(i),value,previousState);
        result.value+=weights[i]*part.value;result.derivative+=weights[i]*part.derivative;
    }
    // Average the nonlinear force over the state step (discrete gradient).
    // This preserves its potential-energy change, rather than injecting energy
    // with midpoint sampling or damping the resonance with backward Euler.
    return{value+amount*result.value,1+amount*result.derivative};
}
[[gnu::always_inline]] inline RageFilter::Shape RageFilter::excitation(double input,double band,double amount) const noexcept
{
    if(weights[0]==1||amount<=0)return{input,0};
    // The other characters need their distortion before the resonant integration,
    // not only in a restoring-force return which can linearize low-frequency audio.
    // Band feedback enters the same shaper: this is not a post-filter drive stage.
    Shape result{weights[0]*input,0};
    for(size_t i=1;i<weights.size();++i)if(weights[i]>0)
    {
        const auto& context=returnContext[i];
        const auto gain=context.sourceGain;
        // Positive band regeneration feeds the distortion, rather than cancelling
        // its input. Its small-signal return is at most one quarter of the linear
        // damping, so it cannot create an undriven oscillator or a nonunique root.
        const auto feedbackGain=context.sourceFeedback;
        const auto source=context.sourceInput;
        auto part=excitationShape(static_cast<RageCharacter>(i),gain*(source+feedbackGain*band));
        if(i==1)
        {
            part.value=distortionAlpha*part.value+(1-distortionAlpha)*distortionTone;
            part.derivative*=distortionAlpha;
        }
        else if(i==3)
        {
            part.value=tapeAlpha*part.value+(1-tapeAlpha)*tapeTone;
            part.derivative*=tapeAlpha;
        }
        const auto wet=context.sourceWet;
        // Parallel excitation retains the dry source at high input levels; hard
        // clipping the whole source here would just reintroduce level collapse.
        result.value+=weights[i]*(input+wet*part.value);
        result.derivative+=weights[i]*wet*gain*feedbackGain*part.derivative;
    }
    return result;
}
double RageFilter::process(double input,double cutoff,double resonance,double amount) noexcept
{
    if(!std::isfinite(input)||!std::isfinite(cutoff)||!std::isfinite(resonance)||!std::isfinite(amount)||std::abs(input)>1e6)
    {const auto faults=diagnostic.faults+1;reset();diagnostic.faults=faults;return 0;}
    if(transition>0)
    {
        for(size_t i=0;i<weights.size();++i)weights[i]+=weightStep[i];
        if(--transition==0){weights={};weights[static_cast<size_t>(target)]=1;}
    }
    cutoff=std::clamp(cutoff,std::min(10.0,sampleRate*.45),sampleRate*.45);
    if(cutoff!=cachedCutoff)
    {const auto g=std::tan(std::numbers::pi*cutoff/sampleRate);coefficient=g/(1+g);cachedCutoff=cutoff;}
    const auto a=coefficient,k=1.94*std::clamp(resonance/.85,0.0,1.0);
    amount=std::clamp(amount,0.0,1.0);const auto drive=1+15*amount*amount;
    double band=0,y2=0;
    if(amount<=0)
    {
        // Preserve the exact clean Rage arithmetic and the existing state basis.
        const auto b1=(1-a)*state1,b2=(1-a)*state2;
        const auto b=a*(1-a)*input+(1-a)*b1-b2,c=a*(1-a)*k;
        band=b/(1-c);const auto y1=a*(input+k*band)+b1;y2=a*y1+b2;
        state1=2*y1-state1;state2=2*y2-state2;
    }
    else
    {
        const auto g=a/(1-a),damping=2-k;
        const auto bandState=state1-state2;
        for(size_t i=0;i<weights.size();++i)if(weights[i]>0)
        {
            auto& context=returnContext[i];const auto character=static_cast<RageCharacter>(i);
            context.drive=characterDrive(character,drive);context.previous=context.drive*bandState;
            context.primitive=colour(character,context.previous).primitive;
            context.compression=i==3?1/(1+.1*amount*tapeEnvelope):1;
            context.excitationCompression=i==3?1/std::sqrt(1+4*amount*tapeEnvelope):1;
            if(i>0)
            {
                // Frozen during the implicit solve; retain the exact original
                // arithmetic while avoiding repeated divisions and gain setup.
                context.sourceGain=1+excitationSensitivity[i]*(drive-1);
                context.sourceFeedback=.25*(2-k)/(excitationLevel[i]*context.sourceGain);
                context.sourceInput=i==4?input+amount*(.6*(input-transistorLow)-.35*transistorEnvelope):input;
                context.sourceWet=excitationLevel[i]*amount*context.excitationCompression;
            }
        }
        const auto b=bandState+g*(input-state2),c=1+g*damping;
        // band = B + g*(input - damping*band - low)
        // low  = L + g*F(band), F = band + amount*averageColour(B,2*band-B).
        // Excitation adds a bounded nonlinear regenerative return E(input,band).
        // f = c*band + g*g*F(band) - b - g*(E-input).
        // E' <= damping/4, hence f' >= 1 + .75*g*damping + g*g > 0.
        // The explicit linear path retains restoring force even on clip plateaus.
        band=previousBand;
        // All colour functions are bounded in magnitude by pi/2 (< 2).
        const auto sourceBound=weights[0]==1?0:g*.3;
        const auto bound=(std::abs(b)+g*g*2+sourceBound)/(c+g*g)+1e-9;
        auto lo=-bound,hi=bound;band=std::clamp(band,lo,hi);
        // The solve evaluates pure functions with frozen per-sample context.
        // Reuse the converged evaluation for residual/output instead of running
        // the full nonlinear transfers two more times at exactly the same band.
        struct Evaluation {Shape nonlinear,source;double function;};
        const auto evaluate=[&](double value)
        {
            const auto nonlinear=feedback(value,bandState,amount),source=excitation(input,value,amount);
            return Evaluation{nonlinear,source,c*value-b+g*g*nonlinear.value-g*(source.value-input)};
        };
        Evaluation final{};bool evaluated=false;
        for(int iteration=0;iteration<10;++iteration)
        {
            final=evaluate(band);evaluated=true;const auto f=final.function;
            if(std::abs(f)<1e-12*(1+std::abs(b)))break;
            if(f>0)hi=band;else lo=band;
            const auto next=band-f/(c+g*g*final.nonlinear.derivative-g*final.source.derivative);
            band=next>lo&&next<hi?next:(lo+hi)*.5;evaluated=false;
        }
        if(!evaluated)final=evaluate(band);
        auto residual=std::abs(final.function);
        if(residual>1e-9*(1+std::abs(b)))
        {
            ++diagnostic.solverFallbacks;
            for(int iteration=0;iteration<40;++iteration)
            {
                const auto f=final.function;
                if(std::abs(f)<1e-12*(1+std::abs(b)))break;
                if(f>0)hi=band;else lo=band;band=(lo+hi)*.5;final=evaluate(band);
            }
            residual=std::abs(final.function);
        }
        diagnostic.maximumResidual=std::max(diagnostic.maximumResidual,residual);
        y2=state2+g*final.nonlinear.value;
        state1=2*(band+y2)-state1;state2=2*y2-state2;
    }
    previousBand=band;
    if(weights[1]>0)
    {
        const auto gain=1+excitationSensitivity[1]*(drive-1),feedbackGain=.25*(2-k)/(excitationLevel[1]*gain);
        distortionTone+=distortionAlpha*(excitationShape(RageCharacter::distortion,gain*(input+feedbackGain*band)).value-distortionTone);
    }
    if(weights[3]>0)
    {
        const auto gain=1+excitationSensitivity[3]*(drive-1),feedbackGain=.25*(2-k)/(excitationLevel[3]*gain);
        tapeTone+=tapeAlpha*(std::atan(gain*(input+feedbackGain*band))-tapeTone);
    }
    // Track source history independently of the selected character, so entering
    // Tape on a held note does not restart its level detector. Neither history
    // changes the accepted Diode arithmetic or zero-Rage output.
    const auto magnitude=std::abs(input);
    tapeEnvelope+=(magnitude-tapeEnvelope)*(magnitude>tapeEnvelope?envelopeAttack:envelopeRelease);
    transistorLow+=transistorAlpha*(input-transistorLow);
    transistorEnvelope+=transistorEnvelopeAlpha*(magnitude-transistorEnvelope);
    const auto output=y2-dcInput+dcPole*dcOutput;dcInput=y2;dcOutput=output;
    if(!std::isfinite(output)||!std::isfinite(state1)||!std::isfinite(state2))
    {const auto faults=diagnostic.faults+1;reset();diagnostic.faults=faults;return 0;}
    return output;
}
}
