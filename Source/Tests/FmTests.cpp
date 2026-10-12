#include "FmTests.h"
#include "PluginEditor.h"
#include "Instrument/FmPanel.h"
#include "Instrument/PresetStore.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride;
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
void near(double a,double b,double tolerance,const char* message){require(std::abs(a-b)<=tolerance,message);}
juce::File library(){return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-fm-"+juce::Uuid().toString());}
struct EngineHarness
{
    SoundStudies::Engine engine;DSP::FmSettings settings;
    EngineHarness(DSP::FmSettings s={},int rate=48000):settings(s)
    {
        engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);engine.setPitchMotion({});engine.setFmIndex(1.5);
        engine.setAmplitudeEnvelope(.001,.1,.8f,.2);engine.setFmSettings(s);engine.prepare(rate);
    }
    void on(int n=60,bool overlap=false,bool returning=false,SoundStudies::Articulation articulation=SoundStudies::Articulation::pad)
    {engine.beginNoteGroup();engine.noteOn(n,.8f,articulation,overlap,returning);}
    void tick(int samples,const DSP::ModulationFrame* frames=nullptr)
    {std::vector<float> l(static_cast<size_t>(samples)),r(l.size());engine.render(l.data(),r.data(),samples,nullptr,frames);for(auto v:l)require(std::isfinite(v),"Extended FM produced non-finite audio");}
};
void composition()
{
    DSP::FmSettings s;s.ratio=2;s.envelopeAmount=3;s.shapeAmount=.2;s.mix=.1;
    s.routes={DSP::FmRoute{DSP::FmSource::lfo,DSP::FmTarget::depth,-2},DSP::FmRoute{DSP::FmSource::filter,DSP::FmTarget::ratio,1},DSP::FmRoute{DSP::FmSource::network,DSP::FmTarget::mix,2}};
    const auto v=DSP::combineFm(s,4.5,.5,{0,1,.5,1,.3});near(v.index,4,0,"FM sources did not sum before the final clamp");near(v.ratio,4,0,"Ratio routes are not octave offsets");near(v.mix,.7,1e-12,"Network mix route did not add normalized offsets");
    s.routes[0]={DSP::FmSource::amplitude,DSP::FmTarget::shape,-2};near(DSP::combineFm(s,0,0,{0,0,1,0,0}).shape,0,0,"Shape route escaped bounds");
    s.routes[1].amount=4;near(DSP::combineFm(s,0,0,{0,0,0,1,0}).ratio,8,0,"Ratio upper bound failed");
    EngineHarness h(s);h.on();std::vector<DSP::ModulationFrame> frames(200000);for(auto& f:frames){f.fm=3;f.fmWave=1;f.filterEnvelope=1;}h.tick(10000,frames.data());
    const auto state=h.engine.getFmSnapshot();near(state.index,4,1e-8,"Engine did not clamp composed FM index");near(state.ratio,8,1e-8,"Engine did not route the shared filter envelope");
    s.envelopeAmount=-3;s.routes[0]={DSP::FmSource::lfo,DSP::FmTarget::depth,-2};s.routes[1]={};s.routes[2]={};h.engine.setFmSettings(s);
    h.tick(200000,frames.data());near(h.engine.getFmSnapshot().index,1,1e-5,"Base + LFO + signed envelope was prematurely clamped");
}
void ratiosAndEnvelope()
{
    for(const auto rate:{44100,48000,96000})for(const auto ratio:{.25,1.0,2.0,3.0,std::sqrt(2.0),8.0})
    {
        DSP::FmSettings s;s.ratio=ratio;s.envelopeAmount=2;s.envelope={.01,.02,.25,.05};EngineHarness h(s,rate);h.on();h.tick(rate/100);
        auto v=h.engine.getFmSnapshot();near(v.primaryHz,SoundStudies::noteFrequency(60)*ratio,1e-9,"Primary modulator tuning is wrong");near(v.envelope,1,.00001,"Independent FM attack timing is wrong");
        h.tick(rate/2);v=h.engine.getFmSnapshot();near(v.envelope,.25,.001,"Independent FM sustain did not settle");near(v.index,2,.002,"FM envelope did not modulate depth");
        h.engine.noteOff(60);h.tick(rate/20);near(h.engine.getFmSnapshot().envelope,0,.00001,"Independent FM release timing is wrong");
        require(h.engine.getNetworkSnapshot().activeVoices==1,"FM envelope release ended the amplitude voice early");h.tick(rate);require(h.engine.getNetworkSnapshot().activeVoices==0,"FM envelope kept an inaudible voice alive");
    }
    DSP::FmSettings s;s.ratio=1.7;s.envelopeAmount=2;s.envelope={.1,.2,.25,.5};EngineHarness mono(s);SoundStudies::PitchSettings pitch;pitch.legato=true;
    mono.engine.setPitchMotion(pitch);mono.on(60,false,false,SoundStudies::Articulation::lead);mono.tick(3000);const auto level=mono.engine.getFmSnapshot().envelope;
    mono.on(67,true,false,SoundStudies::Articulation::lead);mono.tick(1);near(mono.engine.getFmSnapshot().envelope,level,1.0/(48000*.1)+1e-8,"Legato retriggered independent FM envelope");
    mono.on(60,false,true,SoundStudies::Articulation::lead);mono.tick(1);require(mono.engine.getFmSnapshot().envelope>=level,"Return note restarted FM envelope");
    pitch.legato=false;mono.engine.setPitchMotion(pitch);mono.on(72,true,false,SoundStudies::Articulation::lead);mono.tick(4800);near(mono.engine.getFmSnapshot().envelope,1,1e-5,"Mono retrigger did not restart FM attack from current level");
    pitch.autobend={true,.2,12,SoundStudies::PitchTarget::modulators,false};mono.engine.setPitchMotion(pitch);mono.on(72,false,false,SoundStudies::Articulation::lead);mono.tick(1);
    const auto p=mono.engine.getPitchSnapshot();near(mono.engine.getFmSnapshot().primaryHz,p.modulatorHz*1.7,1e-8,"FM ratio did not compose with modulator pitch gestures");
    for(int n=30;n<60;++n)mono.on(n);mono.tick(64);require(mono.engine.getNetworkSnapshot().activeVoices<=12&&mono.engine.getNumericalFaults()==0,"FM envelope broke voice stealing");
    EngineHarness continuous(s);continuous.on();continuous.tick(1000);auto before=continuous.engine.getFmSnapshot();s.ratio=3.17;s.shape=DSP::FmShape::saturated;s.shapeAmount=1;continuous.engine.setFmSettings(s);continuous.tick(1);
    const auto after=continuous.engine.getFmSnapshot();auto delta=after.primaryPhase-before.primaryPhase;delta-=2*std::numbers::pi*std::floor(delta/(2*std::numbers::pi));
    near(delta,2*std::numbers::pi*after.primaryHz/48000,1e-12,"Editing FM ratio or shape reset the modulator phase");
}
void referenceSpectra()
{
    for(const auto shape:{DSP::FmShape::sine,DSP::FmShape::triangle,DSP::FmShape::saturated})for(const auto amount:{0.0,.37,1.0})
    {
        const auto harmonics=DSP::fmHarmonics(shape,amount);
        for(int p=0;p<2048;++p){const auto phase=2*std::numbers::pi*p/2048;double direct=0;for(size_t n=0;n<harmonics.size();++n)direct+=harmonics[n]*std::sin(phase*static_cast<double>(2*n+1));near(DSP::fmWave(phase,harmonics),direct,1e-14,"Odd-harmonic recurrence changed the modulator waveform");}
    }
    const auto& tables=DSP::fmTables();double worstReconstruction=0,worstAlias=0;
    for(const auto shape:{DSP::FmShape::sine,DSP::FmShape::triangle,DSP::FmShape::saturated})for(const auto amount:{0.0,.37,1.0})for(const auto index:{.0,.7,2.3,4.0})
    {
        const auto coefficients=tables.coefficients(shape,amount,index);const auto harmonics=DSP::fmHarmonics(shape,amount);
        for(size_t n=0;n<13;++n)if(shape==DSP::FmShape::sine)near(coefficients[n],SoundStudies::Engine::bessel(static_cast<int>(n),index),.00025,"Sine FM reference table is not the generalized Bessel expansion");
        // Independent 16x phase-sampling reference. Fourier-transform the direct
        // shaped PM signal, then discard above-Nyquist bins. Not another lookup.
        constexpr int size=1024,high=size*16;constexpr int carrierBin=37,modulatorBin=113;
        std::vector<double> direct(static_cast<size_t>(high));
        for(int p=0;p<high;++p){const auto phase=2*std::numbers::pi*p/high;direct[static_cast<size_t>(p)]=std::sin(carrierBin*phase+index*DSP::fmWave(modulatorBin*phase,harmonics));}
        double errorEnergy=0,referenceEnergy=0;
        for(size_t n=0;n<DSP::fmSidebands;++n)
        {
            const auto frequency=carrierBin+static_cast<int>(n)*modulatorBin;
            double coefficient=0;for(int p=0;p<high;++p)coefficient+=direct[static_cast<size_t>(p)]*std::sin(2*std::numbers::pi*frequency*p/high)*2/high;
            near(coefficient,coefficients[n],.0008,"Shaped FM lookup disagrees with independently oversampled reference");
            if(std::abs(frequency)<size*.47){errorEnergy+=std::pow(coefficient-coefficients[n],2);referenceEnergy+=coefficient*coefficient;}
            if(n>0)
            {
                const auto negative=carrierBin-static_cast<int>(n)*modulatorBin;double c=0;
                for(int p=0;p<high;++p)c+=direct[static_cast<size_t>(p)]*std::sin(2*std::numbers::pi*negative*p/high)*2/high;
                const auto expected=coefficients[n]*(n%2==0?1:-1);near(c,expected,.0008,"Negative-frequency FM sidebands folded incorrectly");
                if(std::abs(negative)<size*.47){errorEnergy+=std::pow(c-expected,2);referenceEnergy+=c*c;}
            }
        }
        worstAlias=std::max(worstAlias,std::sqrt(errorEnergy/std::max(referenceEnergy,1e-12)));
        // Low-frequency reconstruction includes both signs of all retained bands.
        for(int p=0;p<256;++p)
        {
            const auto phase=2*std::numbers::pi*p/256;double value=coefficients[0]*std::sin(.73*phase);
            for(size_t n=1;n<coefficients.size();++n)value+=coefficients[n]*(std::sin((.73+static_cast<double>(n))*phase)+(n%2==0?1:-1)*std::sin((.73-static_cast<double>(n))*phase));
            const auto exact=std::sin(.73*phase+index*DSP::fmWave(phase,harmonics));worstReconstruction=std::max(worstReconstruction,std::abs(value-exact));
        }
    }
    require(worstReconstruction<.02&&worstAlias<.002,"Shaped FM reference exceeded spectral error budget");
    std::cout<<"FM reference: maximum waveform error "<<worstReconstruction<<", relative retained-band error "<<worstAlias<<" versus 16x direct reference\n";
    double worstOutOfBand=0;
    for(const auto shape:{DSP::FmShape::sine,DSP::FmShape::triangle,DSP::FmShape::saturated})for(const auto ratio:{3.0,8.0})
    {
        // Coherent high-frequency render: unmasked direct PM would fold many
        // sidebands into unexpected bins. Test the actual production renderer,
        // including decimation and its fixed output soft ceiling.
        constexpr int size=1024,carrierBin=37;const auto rate=SoundStudies::noteFrequency(60)*size/carrierBin;
        DSP::FmSettings s;s.ratio=ratio;s.shape=shape;s.shapeAmount=1;
        SoundStudies::Engine e;e.setTreatment(SoundStudies::Treatment::fm);e.setPitchMotion({});e.setFmIndex(4);e.setFmSettings(s);e.setAmplitudeEnvelope(.001,.01,1,.2);e.prepare(rate);e.noteOn(60,1);
        std::vector<float> l(16384),r(l.size());e.render(l.data(),r.data(),static_cast<int>(l.size()));
        std::array<float,size> audio{};e.render(audio.data(),r.data(),size);
        std::array<bool,size/2+1> retained{};const auto step=static_cast<int>(ratio)*carrierBin;
        for(int n=0;n<static_cast<int>(DSP::fmSidebands);++n)for(const auto frequency:{carrierBin+n*step,carrierBin-n*step})if(std::abs(frequency)<size*.47)retained[static_cast<size_t>(std::abs(frequency))]=true;
        double outside=0,total=0;for(size_t bin=1;bin<retained.size();++bin){double real=0,imaginary=0;for(int p=0;p<size;++p){const auto phase=2*std::numbers::pi*static_cast<double>(bin)*p/size;real+=audio[static_cast<size_t>(p)]*std::cos(phase);imaginary+=audio[static_cast<size_t>(p)]*std::sin(phase);}const auto power=real*real+imaginary*imaginary;total+=power;if(!retained[bin])outside+=power;}
        worstOutOfBand=std::max(worstOutOfBand,outside/std::max(total,1e-20));
    }
    require(worstOutOfBand<1e-5,"Actual shaped FM renderer folded excessive above-Nyquist energy into unexpected bins");
    std::cout<<"FM high-frequency reference output: unexpected-bin energy "<<10*std::log10(std::max(worstOutOfBand,1e-20))<<" dB (includes soft-ceiling distortion)\n";
}
std::vector<float> render(DSP::FmSettings settings,int block,double coupling=.65,double stress=.2,double response=.025,int rate=48000)
{
    EngineHarness h(settings,rate);h.engine.setNetworkDimensions(static_cast<float>(coupling),static_cast<float>(stress),response);h.on(72);
    std::vector<float> result(14000),right(static_cast<size_t>(block));
    for(int i=0;i<14000;){const auto n=std::min(block,(i<9000?9000:14000)-i);h.engine.render(result.data()+i,right.data(),n);i+=n;if(i==9000)h.engine.noteOff(72);}
    require(h.engine.getNumericalFaults()==0,"Advanced FM caused network faults");return result;
}
void soundAndNetwork()
{
    DSP::FmSettings s;s.ratio=1.414;s.shape=DSP::FmShape::triangle;s.shapeAmount=.8;s.envelopeAmount=2;s.mix=.25;
    const auto a=render(s,64),b=render(s,64);require(a==b,"Advanced FM is not deterministic");
    require(a==render(s,257),"Advanced FM sample timing depends on host block size");
    s.mix=0;const auto carrier=render(s,64);s.mix=1;const auto modulator=render(s,64);double difference=0;for(size_t i=0;i<a.size();++i)difference+=std::abs(carrier[i]-modulator[i]);require(difference>1,"Operator mix is not audible");
    EngineHarness dry(s);s.mix=0;EngineHarness wet(s);dry.on();wet.on();dry.tick(5000);wet.tick(5000);
    const auto dn=dry.engine.getNetworkSnapshot(),wn=wet.engine.getNetworkSnapshot();require(dn.offsets==wn.offsets&&dn.links==wn.links,"Audible operator mix changed internal network feedback");
    const auto coupled=render(s,64,.9,.8,.2);difference=0;for(size_t i=0;i<a.size();++i)difference+=std::abs(a[i]-coupled[i]);require(difference>1,"Advanced FM bypassed coupling/stress/Response");
    for(const auto rate:{44100,48000,96000}){s.ratio=8;s.envelopeAmount=4;s.shapeAmount=1;s.mix=.5;render(s,64,1,1,.001,rate);}
}
void derivedDataParity()
{
    DSP::FmSettings settings;settings.ratio=1.414;settings.shape=DSP::FmShape::triangle;settings.shapeAmount=.8;
    EngineHarness cached(settings),reference(settings);reference.engine.setDerivedDataCaching(false);cached.on(72);reference.on(72);
    cached.tick(24000);const auto updates=cached.engine.getDerivedUpdates();
    require(updates.coefficients==1&&updates.harmonics==1&&updates.bands==1,"F1: Static advanced FM recomputed derived data every sample");
    // Fresh matching voices: automate every cache dependency and enter/leave
    // the historical spectral treatment, including after its fade has settled.
    cached.engine.prepare(48000);reference.engine.prepare(48000);cached.on(72);reference.on(72);cached.on(60);reference.on(60);
    constexpr int block=32;std::array<float,block> left{},right{},expectedLeft{},expectedRight{};std::array<DSP::ModulationFrame,block> frames{};
    for(int b=0;b<2000;++b)
    {
        if(b==200||b==1500){cached.engine.setTreatment(SoundStudies::Treatment::spectral);reference.engine.setTreatment(SoundStudies::Treatment::spectral);cached.engine.setAmount(.9f);reference.engine.setAmount(.9f);}
        if(b==400){cached.engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);reference.engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);}
        if(b==1600){cached.engine.setTreatment(SoundStudies::Treatment::phaseDelay);reference.engine.setTreatment(SoundStudies::Treatment::phaseDelay);}
        if(b==1700){cached.engine.setTreatment(SoundStudies::Treatment::noteCoupledDelay);reference.engine.setTreatment(SoundStudies::Treatment::noteCoupledDelay);}
        if(b==1800)for(int note=48;note<62;++note){cached.on(note);reference.on(note);}
        if(b==250||b==450||b==650)
        {
            settings.ratio=b==250?3.17:1.2;settings.shape=static_cast<DSP::FmShape>((b/200)%3);settings.shapeAmount=b==450?1:.3;settings.envelopeAmount=1.5;
            settings.routes[0]={DSP::FmSource::lfo,DSP::FmTarget::ratio,.2};cached.engine.setFmSettings(settings);reference.engine.setFmSettings(settings);
        }
        if(b==300){cached.on(84);reference.on(84);}
        if(b==900)cached.engine.setDerivedDataCaching(false);if(b==1100)cached.engine.setDerivedDataCaching(true);
        for(int i=0;i<block;++i){const auto phase=(b*block+i)*.001;frames[static_cast<size_t>(i)]={std::sin(phase)*3,std::sin(phase)*.4,.1*std::sin(phase),0,std::sin(phase),.3};}
        cached.engine.render(left.data(),right.data(),block,nullptr,frames.data());reference.engine.render(expectedLeft.data(),expectedRight.data(),block,nullptr,frames.data());
        require(left==expectedLeft&&right==expectedRight,"F1: Caching/lazy spectral initialization changed audio under dependency changes or treatment transitions");
    }
}
template<class T>T* child(juce::Component& root,const juce::String& name)
{for(auto* c:root.getChildren()){if(auto* v=dynamic_cast<T*>(c);v&&c->getName()==name)return v;if(auto* v=child<T>(*c,name))return v;}return nullptr;}
void rendererAndUi()
{
    const auto directory=library();NitrideAudioProcessor p(directory);auto& session=p.getInstrumentSession();
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    auto* panel=child<FmPanel>(*editor,"Advanced FM");auto* disclosure=child<juce::TextButton>(*editor,"Open advanced FM");require(panel&&disclosure&&!panel->isShowing(),"FM controls did not start progressively hidden");
    disclosure->setToggleState(true,juce::dontSendNotification);disclosure->onClick();require(panel->isShowing(),"FM disclosure did not open native panel");
    child<juce::TextButton>(*panel,"FM envelope page")->onClick();require(child<UI::ValueControl>(*panel,"FM envelope attack")->isShowing(),"FM envelope page did not reveal independent ADSR");
    child<juce::TextButton>(*panel,"FM modulation page")->onClick();require(child<UI::ValueControl>(*panel,"FM route 3 amount")->isShowing(),"FM modulation page did not reveal routing");
    child<juce::TextButton>(*panel,"FM source page")->onClick();
    auto* ratio=child<UI::ValueControl>(*panel,"FM ratio");ratio->getAccessibilityHandler()->getValueInterface()->setValue(1.414);
    require(session.read().values[fmRatio]>1.4,"Accessible FM ratio edit failed");auto* integer=child<juce::ComboBox>(*panel,"Harmonic FM ratio");integer->setSelectedId(3,juce::sendNotificationSync);near(session.read().values[fmRatio],3,0,"Integer FM ratio shortcut failed");
    for(int i=fmRatio;i<parameterCount;++i)
    {
        const auto param=static_cast<Parameter>(i);const auto& d=hostParameters[static_cast<size_t>(i)];auto* host=p.getState().getParameter(d.id);
        require(host&&host->getVersionHint()==6&&p.getParameters()[hostIndex(param)]==host,"Advanced FM host metadata incomplete");session.set(param,d.kind==ParameterKind::choice?1:(d.minimum+d.maximum)*.5);
    }
    juce::MemoryBlock state;p.getStateInformation(state);NitrideAudioProcessor restored(directory);restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for(int i=fmRatio;i<parameterCount;++i)near(restored.getInstrumentSession().read().values[static_cast<size_t>(i)],session.read().values[static_cast<size_t>(i)],.0001,"Advanced FM state roundtrip lost a control");
    session.beginEdit({hostIndex(fmRoute3Amount)});session.set(fmRoute3Amount,2);session.endEdit({hostIndex(fmRoute3Amount)});p.getState().getParameter("cutoff")->setValueNotifyingHost(.5f);session.undo();near(session.read().values[fmRoute3Amount],0,1e-9,"High-index FM route undo failed");session.redo();near(session.read().values[fmRoute3Amount],2,1e-9,"High-index FM route redo failed");
    session.setCompareReference(factoryPatches()[0]);session.setComparing(true);near(session.audioValue(fmRatio),2,0,"Compare did not use neutral FM settings");session.setComparing(false);
    require(session.keep("Advanced FM").wasOk(),"Keep failed with advanced FM");const auto file=directory.getChildFile("advanced.nitridepreset");require(session.exportPreset(file).wasOk(),"Advanced FM export failed");const auto imported=PresetStore::importFile(file);require(imported.has_value(),"Advanced FM import failed");
    for(int i=fmRatio;i<parameterCount;++i)near(imported->patch.values[static_cast<size_t>(i)],session.read().values[static_cast<size_t>(i)],1e-9,"Preset lost advanced FM settings");
    ratio->onBegin();p.getStateInformation(state);p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));ratio->onChange(8);ratio->onEnd();require(session.read().values[fmRatio]<8,"Stale advanced FM gesture overwrote restored state");
    auto* hostRatio=p.getState().getParameter("fm_ratio");hostRatio->setValueNotifyingHost(hostRatio->convertTo0to1(1.7f));for(int i=0;i<10;++i)juce::MessageManager::getInstance()->runDispatchLoopUntil(20);near(ratio->getValue(),1.7,1e-5,"Advanced FM automation did not update native UI");
    panel->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));require(!panel->isShowing(),"Escape did not close advanced FM");
    editor.reset();directory.deleteRecursively();
    // Shared filter envelope is consumed once by both cutoff and FM routing.
    InstrumentSession s(library());auto patch=factoryPatches()[1];patch.values[fmRoute1Source]=3;patch.values[fmRoute1Target]=0;patch.values[fmRoute1Amount]=2;
    patch.values[filterAttack]=.001;patch.values[filterDecay]=.01;patch.values[filterSustain]=1;s.apply(patch);InstrumentRenderer r(s);r.prepare(48000);
    juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);r.process(audio,midi);midi.clear();for(int i=0;i<300;++i)r.process(audio,midi);
    near(r.fmSnapshot().index,std::min(4.0,patch.values[fm]+2*r.filterEnvelopeLevel()),1e-5,"Shared filter envelope did not reach FM depth route");
    near(s.read().values[fm],patch.values[fm],0,"FM routing overwrote base FM depth");
}
}
void runFmTests(){composition();ratiosAndEnvelope();referenceSpectra();soundAndNetwork();derivedDataParity();rendererAndUi();std::cout<<"FM ratios, independent envelope, shaped spectra, output-only mix, network integration, cached/eager parity, progressive native UI and state/history passed\n";}
