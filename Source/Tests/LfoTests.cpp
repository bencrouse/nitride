#include "LfoTests.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace Nitride;
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double a,double b,double tolerance,const char* message){require(std::abs(a-b)<=tolerance,message);}
juce::File library(){return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-lfo-"+juce::Uuid().toString());}
void wavesAndClock()
{
    using DSP::LfoWave;
    near(DSP::lfoWave(LfoWave::sine,.25),1,1e-12,"Sine quarter-cycle is wrong");
    near(DSP::lfoWave(LfoWave::triangle,0),0,1e-12,"Triangle origin is wrong");near(DSP::lfoWave(LfoWave::triangle,.25),1,1e-12,"Triangle peak is wrong");near(DSP::lfoWave(LfoWave::triangle,.75),-1,1e-12,"Triangle trough is wrong");
    near(DSP::lfoWave(LfoWave::saw,.25),-.5,1e-12,"Sawtooth slope is wrong");near(DSP::lfoWave(LfoWave::square,.2),1,0,"Square positive half is wrong");near(DSP::lfoWave(LfoWave::square,.7),-1,0,"Square negative half is wrong");
    near(DSP::lfoWave(LfoWave::sampleHold,.1,7),DSP::lfoWave(LfoWave::sampleHold,.9,7),0,"Sample & Hold changed within a cycle");
    require(std::abs(DSP::lfoWave(LfoWave::sampleHold,0,7)-DSP::lfoWave(LfoWave::sampleHold,0,8))>.001,"Sample & Hold did not change between cycles");
    for(int wave=0;wave<5;++wave)for(int i=0;i<1000;++i)require(std::abs(DSP::lfoWave(static_cast<LfoWave>(wave),i/1000.0,i/20))<=1,"LFO waveform exceeded its range");
    for(const auto sampleRate:{44100,48000,96000})
    {
        DSP::LfoBank bank;bank.prepare(sampleRate);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};
        settings[0]={true,false,LfoWave::sine,2,1,9};settings[1]={true,false,LfoWave::triangle,3,2,9};bank.configure(settings,{},120);
        for(int i=0;i<sampleRate/4;++i)bank.next();bank.next();near(bank.phase(0),.5,1e-10,"Independent pitch LFO rate drifted");near(bank.phase(1),.75,1e-10,"Independent FM LFO rate drifted");
        settings[0].rate=4;bank.configure(settings,{},120);bank.next();near(bank.phase(0),.5+2.0/sampleRate,1e-10,"Free rate edit reset phase");
        settings[0].wave=LfoWave::square;auto previous=bank.next()[0];bank.configure(settings,{},120);
        for(int i=0;i<100;++i){const auto value=bank.next()[0];require(std::abs(value-previous)<.03,"Waveform change was not dezippered");previous=value;}
        settings[0].enabled=false;bank.configure(settings,{},120);for(int i=0;i<sampleRate/40;++i)bank.next();near(bank.next()[0],0,0,"Disabling LFO failed to return to neutral");
    }
    for(size_t division=0;division<DSP::subdivisions.size();++division)
    {
        DSP::LfoBank bank;bank.prepare(48000);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};settings[0].enabled=true;settings[0].sync=true;settings[0].division=static_cast<int>(division);
        const auto beats=DSP::beatsPerCycle(static_cast<int>(division));DSP::Transport transport{120,beats*.25,4,true,true,false};bank.configure(settings,transport,90);
        for(int i=0;i<100;++i)bank.next();near(bank.phase(0),.25,1e-10,"Musical subdivision or stopped phase is wrong");
        transport.bpm=180;transport.ppq=beats*.75;bank.configure(settings,transport,90);bank.next();near(bank.phase(0),.75,1e-10,"Synced seek/tempo change did not follow PPQ");
    }
    near(DSP::beatsPerCycle(2,3),3,0,"Bar sync ignored host meter");near(DSP::beatsPerCycle(0,3),12,0,"Four-bar sync ignored host meter");
    DSP::LfoBank fallback;fallback.prepare(48000);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};settings[0].sync=true;settings[0].enabled=true;fallback.configure(settings,{},90);
    for(int i=0;i<12000;++i)fallback.next();fallback.next();near(fallback.phase(0),.375,1e-10,"Fallback-tempo sync does not advance without PPQ");near(fallback.bpm(),90,0,"Fallback BPM was not used");
    DSP::Transport bad{NAN,NAN,0,true,true,true};fallback.configure(settings,bad,100);require(!fallback.hostTempo(),"Invalid host tempo did not fall back");near(fallback.bpm(),100,0,"Invalid BPM did not use fallback");require(std::isfinite(fallback.next()[0]),"Invalid host timing contaminated LFO output");
    DSP::LfoBank together; together.prepare(48000);std::array<DSP::LfoBank,DSP::lfoCount> independent;
    for(size_t i=0;i<DSP::lfoCount;++i)
    {
        settings[i]={true,i%2==0,static_cast<LfoWave>(i%5),.01+static_cast<double>(i)*3.5,i==0?-24.0:i==1?-4.0:i==2?-6.0:i==3?1.0:-1.0,static_cast<int>(i)*3};
        independent[i].prepare(48000);std::array<DSP::LfoSettings,DSP::lfoCount> isolated{};isolated[i]=settings[i];independent[i].configure(isolated,{},300);
    }
    together.configure(settings,{},300);
    for(int n=0;n<24000;++n)
    {
        const auto all=together.next();require(all[3]>=0&&all[3]<=1,"Tremolo amplified or inverted nominal gain");
        for(size_t i=0;i<DSP::lfoCount;++i)near(all[i],independent[i].next()[i],0,"Running another lane changed a destination's independent clock/depth");
    }
}
// Long triplet S&H runs exposed a one-sample hash change at sample 128000,
// beyond the shorter modulation-render tests' duration.
void sampleHoldBoundaryClock()
{
    const auto renderClock=[](double rate,double bpm,int division,double start,double meter,int block)
    {
        DSP::LfoBank bank;bank.prepare(rate);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};
        settings[1]={true,true,DSP::LfoWave::sampleHold,.25,.12,division};
        std::vector<double> result(static_cast<size_t>(rate*8));
        for(size_t offset=0;offset<result.size();offset+=static_cast<size_t>(block))
        {
            DSP::Transport transport{bpm,start+static_cast<double>(offset)*bpm/(60*rate),meter,true,true,true};
            transport.barCount=static_cast<std::int64_t>(std::floor(transport.ppq/meter));transport.barStart=static_cast<double>(transport.barCount)*meter;transport.hasBarCount=transport.hasBarStart=true;
            bank.configure(settings,transport,120);
            for(size_t i=offset;i<std::min(offset+static_cast<size_t>(block),result.size());++i){bank.next();result[i]=bank.fmWave();}
        }
        return result;
    };
    for(const auto rate:{48000.0,96000.0})
    {
        const auto reference=renderClock(rate,120,8,0,4,64);
        require(reference==renderClock(rate,120,8,0,4,257),"Triplet S&H cycle hash depends on host block size");
        require(reference==renderClock(rate,120,8,0,4,1024),"Triplet S&H cycle hash depends on large host blocks");
    }
    for(const auto division:{0,14})
        require(renderClock(48000,157.5,division,-1.25,3,64)==renderClock(48000,157.5,division,-1.25,3,257),"Negative-PPQ/meter S&H clock depends on host block size");
    DSP::LfoBank stopped;stopped.prepare(48000);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};
    settings[1]={true,true,DSP::LfoWave::sampleHold,.25,.12,8};
    DSP::Transport transport{120,DSP::beatsPerCycle(8)*(4-1e-7),4,true,true,false};stopped.configure(settings,transport,120);
    for(int i=0;i<10000;++i)stopped.next();near(stopped.fmWave(),DSP::lfoWave(DSP::LfoWave::sampleHold,0,3,2),1e-10,"S&H boundary repair moved a genuine pre-boundary position");
    transport.ppq=DSP::beatsPerCycle(8)*4;stopped.configure(settings,transport,120);
    for(int i=0;i<10000;++i)stopped.next();near(stopped.fmWave(),DSP::lfoWave(DSP::LfoWave::sampleHold,0,4,2),1e-10,"Stopped S&H seek did not follow the new cycle");
}

void squareBoundaryClock()
{
    const auto renderClock=[](double rate,int block)
    {
        DSP::LfoBank bank;bank.prepare(rate);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};
        settings[3]={true,true,DSP::LfoWave::square,.25,.92,17};
        std::vector<double> result(static_cast<size_t>(rate*8));
        for(size_t offset=0;offset<result.size();offset+=static_cast<size_t>(block))
        {
            bank.configure(settings,{120,static_cast<double>(offset)*120/(60*rate),4,true,true,true},120);
            for(size_t i=offset;i<std::min(offset+static_cast<size_t>(block),result.size());++i)result[i]=bank.next()[3];
        }
        return result;
    };
    for(double rate:{44100.0,48000.0,96000.0})
    {
        const auto expected=renderClock(rate,64);
        require(expected==renderClock(rate,257),"Square half-cycle depends on host block size");
        require(expected==renderClock(rate,1024),"Square half-cycle depends on large blocks");
    }
    DSP::LfoBank stopped;stopped.prepare(48000);std::array<DSP::LfoSettings,DSP::lfoCount> settings{};
    settings[3]={true,true,DSP::LfoWave::square,.25,1,17};
    DSP::Transport transport{120,DSP::beatsPerCycle(17)*(.5-1e-7),4,true,true,false};stopped.configure(settings,transport,120);
    for(int i=0;i<10000;++i)stopped.next();near(stopped.next()[3],0,1e-10,"Square correction moved a genuine pre-half-cycle position");
    transport.ppq=DSP::beatsPerCycle(17)*.5;stopped.configure(settings,transport,120);
    for(int i=0;i<10000;++i)stopped.next();near(stopped.next()[3],1,1e-10,"Square seek did not select the exact half-cycle");
}

struct Playhead final:juce::AudioPlayHead
{
    PositionInfo position;
    juce::Optional<PositionInfo> getPosition()const override{return position;}
};
void hostTiming()
{
    NitrideAudioProcessor p(library());auto& s=p.getInstrumentSession();s.set(pitchLfoOn,1);s.set(pitchLfoSync,1);s.set(pitchLfoDepth,2);p.prepareToPlay(48000,64);
    Playhead playhead;playhead.position.setBpm(150);playhead.position.setPpqPosition(7.25);playhead.position.setIsPlaying(true);playhead.position.setTimeSignature(juce::AudioPlayHead::TimeSignature{3,4});p.setPlayHead(&playhead);
    juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);p.processBlock(audio,midi);
    near(s.live.lfoBpm.load(),150,1e-5,"Processor did not read host BPM");require(s.live.lfoHostTempo.load(),"Host tempo was not published to UI");near(s.live.lfoPhases[0].load(),.25+63*150.0/(60*48000),1e-6,"Processor did not align LFO to host PPQ");
    s.set(pitchLfoDivision,2);playhead.position.setIsPlaying(false);playhead.position.setPpqPosition(1.5);p.processBlock(audio,midi);near(s.live.lfoPhases[0].load(),.5,1e-6,"Stopped bar-sync phase ignored host meter");
    playhead.position.setPpqPosition(17.5);playhead.position.setPpqPositionOfLastBarStart(16);playhead.position.setBarCount(4);s.set(pitchLfoDivision,0);
    p.processBlock(audio,midi);near(s.live.lfoPhases[0].load(),.125,1e-6,"Multi-bar sync ignored host bar start/count across meter changes");
    p.setPlayHead(nullptr);s.set(lfoTempo,95);p.processBlock(audio,midi);require(!s.live.lfoHostTempo.load(),"Missing host timing left host BPM status stuck");near(s.live.lfoBpm.load(),95,1e-5,"Processor fallback BPM was not used");
}
std::vector<float> render(int destination,int waveform,int block,bool sync=false)
{
    InstrumentSession session(library());auto patch=factoryPatches()[1];patch.values[cutoff]=1500;patch.values[resonance]=.3;patch.values[spaceMix]=0;
    if(destination==6)
    {
        patch.values[glideOn]=patch.values[autobendOn]=1;patch.values[glideTarget]=patch.values[autobendTarget]=4;
        patch.values[filterAmount]=3;
    }
    for(int i=0;i<6;++i)if(destination==i||destination==6)
    {
        const auto& p=lfoParameters[static_cast<size_t>(i)];patch.values[p.enabled]=1;patch.values[p.wave]=waveform;patch.values[p.rate]=5;patch.values[p.sync]=sync?1:0;patch.values[p.division]=14;
        patch.values[p.depth]=i==0?5:i==1?2:i==2?2:.7;
    }
    session.apply(patch);InstrumentRenderer renderer(session);renderer.prepare(48000);juce::MidiBuffer events;events.addEvent(juce::MidiMessage::noteOn(1,60,.8f),13);events.addEvent(juce::MidiMessage::noteOff(1,60),14013);
    std::vector<float> result(18000);
    for(int offset=0;offset<18000;offset+=block)
    {
        const auto size=std::min(block,18000-offset);juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;midi.addEvents(events,offset,size,-offset);
        DSP::Transport transport{120,1.25+offset*2.0/48000,4,true,true,true};renderer.process(audio,midi,false,transport);
        for(int i=0;i<size;++i){const auto value=audio.getSample(0,i);require(std::isfinite(value),"Modulation render became non-finite");result[static_cast<size_t>(offset+i)]=value;}
    }
    for(size_t i=0;i<parameterCount;++i)near(session.read().values[i],patch.values[i],1e-12,"LFO wrote modulation into stored base conditions");return result;
}
void destinations()
{
    const auto baseline=render(-1,0,64);
    for(int destination=0;destination<6;++destination)for(int wave=0;wave<5;++wave)
    {
        const auto audio=render(destination,wave,64);double difference=0;for(size_t i=0;i<audio.size();++i)difference+=std::abs(static_cast<double>(audio[i])-baseline[i]);
        require(difference>.01,"An LFO waveform/destination was not audible");
    }
    for(int destination=0;destination<=6;++destination)
    {
        const auto a=render(destination,4,64,true),b=render(destination,4,257,true);double maximum=0;
        for(size_t i=0;i<a.size();++i)maximum=std::max(maximum,std::abs(static_cast<double>(a[i])-b[i]));require(maximum<1e-5,"Synced LFO depends on host block size");
        require(a==render(destination,4,64,true),"Sample & Hold render is not deterministic");
    }
}
void pitchComposition()
{
    using SoundStudies::Engine;
    Engine modulated,neutral;
    SoundStudies::PitchSettings settings;settings.glide.enabled=settings.autobend.enabled=true;
    settings.glide.target=SoundStudies::PitchTarget::opposed;settings.autobend.target=SoundStudies::PitchTarget::tone;
    settings.glide.seconds=.2;settings.autobend.seconds=.1;settings.autobend.depth=7;
    for(auto* engine:{&modulated,&neutral}){engine->setPitchMotion(settings);engine->prepare(48000);engine->noteOn(60,.8f);}
    std::array<float,256> a{},b{};std::array<double,256> toneA{},toneB{};std::array<DSP::ModulationFrame,256> frames{};
    for(auto& frame:frames)frame.pitch=12;
    modulated.render(a.data(),b.data(),256,toneA.data(),frames.data());neutral.render(a.data(),b.data(),256,toneB.data());
    for(auto* engine:{&modulated,&neutral})engine->noteOn(72,.8f);
    modulated.render(a.data(),b.data(),256,toneA.data(),frames.data());neutral.render(a.data(),b.data(),256,toneB.data());
    const auto m=modulated.getPitchSnapshot(72),n=neutral.getPitchSnapshot(72);
    near(m.carrierHz/n.carrierHz,2,1e-12,"Pitch LFO did not compose equally with carrier Glide/Autobend");
    near(m.modulatorHz/n.modulatorHz,2,1e-12,"Pitch LFO did not compose equally with modulator Glide/Autobend");
    require(toneA==toneB,"Pitch LFO rewrote the independent Pitch + Tone trajectory");
    Engine high;high.setPitchMotion({});high.setTreatment(SoundStudies::Treatment::expressiveNetwork);high.setNetworkDimensions(.8f,.8f,.035);high.prepare(48000);high.noteOn(127,.8f);
    for(auto& frame:frames)frame.pitch=24;
    high.render(a.data(),b.data(),256,nullptr,frames.data());
    for(const auto sample:a)near(sample,0,1e-7,"LFO pitch bypassed high-carrier band suppression without Glide/Autobend");
}
template<class T>T* child(juce::Component& root,const juce::String& name)
{for(auto* c:root.getChildren()){if(auto* v=dynamic_cast<T*>(c);v&&c->getName()==name)return v;if(auto* v=child<T>(*c,name))return v;}return nullptr;}
void uiAndState()
{
    NitrideAudioProcessor p(library());auto& s=p.getInstrumentSession();std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    auto* destination=child<juce::ComboBox>(*editor,"LFO destination");auto* wave=child<juce::ComboBox>(*editor,"LFO waveform");auto* sync=child<juce::TextButton>(*editor,"Sync selected LFO");auto* enable=child<juce::TextButton>(*editor,"Enable selected LFO");
    require(destination&&wave&&sync&&enable,"Native Motion bank controls missing");destination->setSelectedId(1,juce::sendNotificationSync);wave->setSelectedId(3,juce::sendNotificationSync);
    enable->setToggleState(true,juce::dontSendNotification);enable->onClick();auto* rate=child<juce::Slider>(*editor,"Pitch LFO rate");auto* depth=child<juce::Slider>(*editor,"Pitch LFO depth");require(rate&&depth,"Selected lane rate/depth controls missing");
    rate->setValue(3,juce::sendNotificationSync);depth->setValue(4,juce::sendNotificationSync);near(s.read().values[pitchLfoRate],3,1e-6,"Native lane rate is disconnected");near(s.read().values[pitchLfoDepth],4,1e-6,"Native lane depth is disconnected");
    near(depth->getValueFromText("DEPTH  -3.5 st"),-3.5,0,"Formatted bipolar depth text could not be edited");near(depth->getValueFromText("4.25 st"),4.25,0,"Plain bipolar depth text could not be edited");
    destination->setSelectedId(6,juce::sendNotificationSync);auto* stressDepth=child<juce::Slider>(*editor,"Stress LFO depth");require(stressDepth,"Stress lane missing");stressDepth->setValue(45,juce::sendNotificationSync);
    near(stressDepth->getValueFromText("DEPTH  45 %"),45,0,"Formatted percent depth text could not be edited");
    auto* hostCoupling=p.getState().getParameter("coupling");hostCoupling->setValueNotifyingHost(hostCoupling->convertTo0to1(.3f));
    juce::MemoryBlock state;p.getStateInformation(state);NitrideAudioProcessor restored(library());restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));auto& r=restored.getInstrumentSession();
    near(r.read().values[stressLfoDepth],.45,1e-6,"State lost LFO parameters above host index 63");r.undo();near(r.read().values[stressLfoDepth],0,1e-6,"Serialized scalable undo did not restore stress depth");near(r.read().values[coupling],.3,1e-6,"LFO undo reverted unrelated base value");r.redo();near(r.read().values[stressLfoDepth],.45,1e-6,"LFO redo failed");
    destination->setSelectedId(1,juce::sendNotificationSync);near(rate->getValue(),3,1e-6,"Destination selection lost independent lane settings");near(depth->getValue(),4,1e-6,"Destination selection lost independent depth");
    sync->setToggleState(true,juce::dontSendNotification);sync->onClick();require(!rate->isVisible(),"Sync did not replace Hz with a musical division");auto* division=child<juce::ComboBox>(*editor,"LFO subdivision");require(division&&division->isVisible(),"Synced subdivision control missing");division->setSelectedId(15,juce::sendNotificationSync);near(s.read().values[pitchLfoDivision],14,0,"Subdivision control is disconnected");
    s.setComparing(true);near(s.audioValue(pitchLfoOn),0,0,"Compare did not audition neutral LFO reference");s.setComparing(false);
    auto preset=PresetStore::decode(PresetStore::encode({"lfo-current",s.read()}));require(preset&&preset->patch.values[pitchLfoSync]>.5,"Current preset round trip lost LFO settings");
    auto invalid=s.saveDocument();invalid["current"].getDynamicObject()->setProperty("parameters",juce::Array<juce::var>{hostParameterCount});require(r.restoreDocument(invalid).failed(),"Invalid history parameter index was accepted");
    p.getStateInformation(state);rate->onDragStart();p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));rate->setValue(10,juce::sendNotificationSync);rate->onDragEnd();near(s.read().values[pitchLfoRate],3,1e-6,"Stale LFO drag overwrote recalled rate");
}
}
void runLfoTests(){wavesAndClock();sampleHoldBoundaryClock();squareBoundaryClock();hostTiming();destinations();pitchComposition();uiAndState();std::cout<<"LFO waves, independent clocks, musical sync/meter/seek/fallback, all destinations, deterministic S&H/square boundaries, pitch composition/band suppression, UI and scalable recall/history passed\n";}
