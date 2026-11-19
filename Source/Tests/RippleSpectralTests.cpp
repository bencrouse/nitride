// Independent reference-rate diagnostics, not an Arturia sonic comparison.
#include "FilterFramework.h"
#include "BuildInfo.h"
#include <juce_dsp/juce_dsp.h>
#include <complex>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr int order=13,count=1<<order;
using Spectrum=std::array<std::complex<float>,count>;
struct Render {Spectrum spectrum;Nitride::DSP::RippleFilter::Diagnostics diagnostic;};
Render render(int rate,int multiplier,int type,int variant,int bin,bool modulation)
{
    Nitride::DSP::RippleFilter core;core.prepare(rate*multiplier,static_cast<Nitride::DSP::RippleType>(type),variant==0?Nitride::DSP::RippleMode::waveshape:Nitride::DSP::RippleMode::feedback);
    Nitride::DSP::FilterDecimator decimator;decimator.prepare(rate,rate*multiplier);Spectrum input{},output{};
    for(int n=-count*8*multiplier;n<count*multiplier;++n)
    {
        const auto time=static_cast<double>(n)/multiplier;
        const auto signal=.65*std::sin(juce::MathConstants<double>::twoPi*bin*time/count);
        const auto cutoff=modulation?3000*std::exp2(1.5*std::sin(juce::MathConstants<double>::twoPi*97*time/count)+2*signal):3000.;
        const auto value=decimator.process(core.process(signal,cutoff,.85,.75,.85,variant==1?-.95:.95));
        if(n>=0&&n%multiplier==0)input[static_cast<size_t>(n/multiplier)]={static_cast<float>(value),0};
    }
    if(core.diagnostics().faults||core.diagnostics().maximumResidual>1e-9)throw std::runtime_error("Ripple spectral numerical guard failed");
    juce::dsp::FFT fft(order);fft.perform(input.data(),output.data(),false);return{output,core.diagnostics()};
}
double db(double energy,double reference){return 10*std::log10(std::max(energy,1e-30)/std::max(reference,1e-30));}
}
int main()
{
    juce::ScopedNoDenormals noDenormals;juce::Array<juce::var> results;
    try
    {
        for(const auto rate:{48000,96000})for(int type=0;type<3;++type)for(int variant=0;variant<3;++variant)for(int probe=0;probe<3;++probe)
        {
            const auto bin=probe==1?997:173;const auto modulated=probe==2;
            const auto production=render(rate,4,type,variant,bin,modulated),reference=render(rate,32,type,variant,bin,modulated);
            double total=0,nonHarmonic=0,error=0,referenceEnergy=0,referenceNonHarmonic=0;
            for(int k=1;k<count/2;++k)
            {
                const auto i=static_cast<size_t>(k);const auto energy=static_cast<double>(std::norm(production.spectrum[i]));total+=energy;
                if(k%bin!=0){nonHarmonic+=energy;referenceNonHarmonic+=std::norm(reference.spectrum[i]);}
                error+=std::norm(production.spectrum[i]-reference.spectrum[i]);referenceEnergy+=std::norm(reference.spectrum[i]);
            }
            auto* row=new juce::DynamicObject();row->setProperty("sampleRate",rate);row->setProperty("type",type);
            row->setProperty("mode",variant==0?"waveshape":variant==1?"negative-feedback":"positive-feedback");
            row->setProperty("probe",modulated?"cutoff-lfo-and-audio-fm":probe==1?"high-tone":"low-tone");
            row->setProperty("maximumSolverResidual",production.diagnostic.maximumResidual);
            row->setProperty("reference32xMaximumSolverResidual",reference.diagnostic.maximumResidual);
            if(!modulated){row->setProperty("nonHarmonicEnergyDb",db(nonHarmonic,total));row->setProperty("reference32xNonHarmonicEnergyDb",db(referenceNonHarmonic,referenceEnergy));}
            row->setProperty("reference32xResidualDb",db(error,referenceEnergy));results.add(juce::var(row));
        }
        auto* root=new juce::DynamicObject();root->setProperty("format","nitride-ripple-spectral-probes");root->setProperty("version",1);
        root->setProperty("synthVersion",Nitride::buildVersion);root->setProperty("evaluationBuild",Nitride::evaluationBuild);
        root->setProperty("note","4x production versus 32x independent-rate reference; three types, waveshaping and both feedback signs. Nonharmonic energy includes leakage/aliases and transient residual. Reference residual includes phase, discretization, trajectory and decimator differences, NOT pure alias energy, a listening verdict or Pigments equivalence.");
        root->setProperty("results",results);std::cout<<juce::JSON::toString(juce::var(root))<<'\n';return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
