// Offline coherent-tone qualification. Reference residual includes discretization
// and decimator differences; it must not be labelled a pure alias measurement.
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
struct Render {Spectrum spectrum;Nitride::DSP::RageFilter::Diagnostics diagnostic;};
Render render(int rate,int multiplier,int character,int bin,bool modulation,double resonance)
{
    Nitride::DSP::RageFilter core;core.setCharacter(static_cast<Nitride::DSP::RageCharacter>(character));core.prepare(rate*multiplier);
    Nitride::DSP::FilterDecimator decimator;decimator.prepare(rate,rate*multiplier);
    Spectrum input{},output{};
    for(int n=-count*8*multiplier;n<count*multiplier;++n)
    {
        const auto time=static_cast<double>(n)/multiplier;
        const auto signal=.65*std::sin(juce::MathConstants<double>::twoPi*bin*time/count);
        const auto cutoff=modulation?5000*std::exp2(1.5*std::sin(juce::MathConstants<double>::twoPi*97*time/count)+2*signal):12000.0;
        const auto value=decimator.process(core.process(signal,cutoff,resonance,1));
        if(n>=0&&n%multiplier==0)input[static_cast<size_t>(n/multiplier)]={static_cast<float>(value),0};
    }
    if(core.diagnostics().faults)throw std::runtime_error("Spectral probe needed numerical recovery");
    if(core.diagnostics().maximumResidual>1e-6)throw std::runtime_error("Spectral probe failed the implicit-solve residual guard");
    juce::dsp::FFT fft(order);fft.perform(input.data(),output.data(),false);return{output,core.diagnostics()};
}
double db(double energy,double reference){return 10*std::log10(std::max(energy,1e-30)/std::max(reference,1e-30));}
}
int main(int argc,char** argv)
{
    juce::Array<juce::var> results;
    try
    {
        if(argc==2&&std::string(argv[1])=="--identity")
        {
            std::cout<<"{\"format\":\"nitride-spectral-probe-identity\",\"version\":1,\"synthVersion\":\""<<Nitride::buildVersion
                <<"\",\"evaluationBuild\":\""<<Nitride::evaluationBuild<<"\"}\n";return 0;
        }
        if(argc!=1)throw std::runtime_error("Usage: nitride_rage_spectral_tests [--identity]");
        for(const auto rate:{48000,96000})for(int character=0;character<5;++character)for(int probe=0;probe<5;++probe)
        {
            const auto high=probe==1||probe==4;const auto bin=high?997:173;const auto modulated=probe==2;
            const auto resonance=probe>=3?.70:.85;
            const auto production=render(rate,4,character,bin,modulated,resonance),reference=render(rate,32,character,bin,modulated,resonance);
            double total=0,nonHarmonic=0,error=0,referenceEnergy=0,referenceNonHarmonic=0;
            for(int k=1;k<count/2;++k)
            {
                const auto energy=static_cast<double>(std::norm(production.spectrum[static_cast<size_t>(k)]));total+=energy;
                if(k%bin!=0){nonHarmonic+=energy;referenceNonHarmonic+=std::norm(reference.spectrum[static_cast<size_t>(k)]);}
                error+=std::norm(production.spectrum[static_cast<size_t>(k)]-reference.spectrum[static_cast<size_t>(k)]);
                referenceEnergy+=std::norm(reference.spectrum[static_cast<size_t>(k)]);
            }
            auto* row=new juce::DynamicObject();row->setProperty("sampleRate",rate);row->setProperty("character",character);
            row->setProperty("probe",modulated?"cutoff-lfo-and-audio-fm":high?"high-tone":"low-tone");
            row->setProperty("resonance",resonance);
            row->setProperty("fundamentalHz",static_cast<double>(bin)*rate/count);
            row->setProperty("maximumSolverResidual",production.diagnostic.maximumResidual);
            row->setProperty("reference32xMaximumSolverResidual",reference.diagnostic.maximumResidual);
            if(!modulated)
            {
                row->setProperty("nonHarmonicEnergyDb",db(nonHarmonic,total));
                row->setProperty("reference32xNonHarmonicEnergyDb",db(referenceNonHarmonic,referenceEnergy));
            }
            row->setProperty("reference32xResidualDb",db(error,referenceEnergy));results.add(juce::var(row));
        }
        auto* root=new juce::DynamicObject();root->setProperty("format","nitride-rage-spectral-probes");root->setProperty("version",2);
        root->setProperty("synthVersion",Nitride::buildVersion);root->setProperty("evaluationBuild",Nitride::evaluationBuild);
        root->setProperty("note","Coherent steady tones, full Rage at 0.70/0.85 resonance. Non-harmonic energy includes aliases, numerical leakage AND intrinsic nonlinear subharmonics/nonperiodic response: compare the 32x non-harmonic result. 32x residual includes response/phase and trajectory differences, not pure alias energy or a Pigments comparison.");
        root->setProperty("results",results);std::cout<<juce::JSON::toString(juce::var(root))<<'\n';return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
