#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include <iostream>
#include <thread>
#include <stdexcept>

namespace
{
using namespace Nitride;
using SoundStudies::noteFrequency;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double value,double expected,double tolerance,const char* message){require(std::abs(value-expected)<=tolerance,message);}
struct TestDirectory
{
    juce::File folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("nitride-pitch-tests","",false);
    TestDirectory(){require(folder.createDirectory().wasOk(),"Cannot create test directory");}
    ~TestDirectory(){folder.deleteRecursively();}
};
double render(NitrideAudioProcessor& processor,int samples,int block=64)
{
    double peak=0;juce::AudioBuffer<float> audio(2,block);juce::MidiBuffer midi;
    while(samples>0)
    {
        const auto size=std::min(samples,block);audio.setSize(2,size,false,false,true);processor.processBlock(audio,midi);
        for(int c=0;c<2;++c)for(int i=0;i<size;++i){const auto v=audio.getSample(c,i);require(std::isfinite(v),"Non-finite pitch-motion audio");peak=std::max(peak,std::abs(static_cast<double>(v)));}
        samples-=size;
    }
    return peak;
}
void event(NitrideAudioProcessor& processor,const juce::MidiMessage& message)
{
    juce::AudioBuffer<float> audio(2,1);juce::MidiBuffer midi;midi.addEvent(message,0);processor.processBlock(audio,midi);
}
Patch motionPatch()
{
    auto p=factoryPatches()[2];p.name="Pitch test";p.values[glideOn]=0;p.values[glideCurve]=0;p.values[glideTime]=.1;
    p.values[autobendOn]=0;p.values[autobendTime]=.1;p.values[motionDepth]=0;p.values[spaceMix]=0;
    return p;
}
void pitchBehavior(const juce::File& directory)
{
    for(const auto rate:{44100,48000,96000})
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[glideOn]=1;patch.values[glideCurve]=1;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(rate,256);
        event(p,juce::MidiMessage::noteOn(1,60,.8f));render(p,rate/5);
        event(p,juce::MidiMessage::noteOn(1,72,.8f));render(p,rate/20-1);
        near(p.getPitchSnapshot().carrierHz,noteFrequency(66),.02,"Linear Glide midpoint is not six semitones");
        event(p,juce::MidiMessage::noteOn(1,67,.8f));render(p,rate/10);
        near(p.getPitchSnapshot().carrierHz,noteFrequency(67),.0001,"Glide did not arrive after retargeting");
        event(p,juce::MidiMessage::noteOff(1,67));
        require(p.getPitchSnapshot().note==72,"Mono did not return to the previous held key");
        event(p,juce::MidiMessage::noteOff(1,72));require(p.getPitchSnapshot().note==60,"Mono held-note stack lost its oldest key");
        event(p,juce::MidiMessage::noteOff(1,60));render(p,rate);
        require(p.getInstrumentSession().live.voices.load()==0,"Mono note stack left a stuck voice");
    }
    for(int target=0;target<5;++target)for(const auto depth:{-12.0,12.0})
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[autobendOn]=1;patch.values[autobendDepth]=depth;patch.values[autobendTarget]=target;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,256);event(p,juce::MidiMessage::noteOn(1,60,.8f));
        const auto initial=p.getPitchSnapshot();const auto offset=depth*std::pow(1-1.0/4800,3);
        const auto carrier=target==1?0:offset;const auto modulator=target==0?0:target==3?-offset:offset;
        near(initial.carrierHz,noteFrequency(60+carrier),.0001,"Autobend carrier routing or sign is wrong");
        near(initial.modulatorHz,noteFrequency(60+modulator),.0001,"Autobend modulator routing or Opposed sign is wrong");
        if(target==3)near(initial.carrierHz*initial.modulatorHz,noteFrequency(60)*noteFrequency(60),.0001,"Opposed pitch movements are not equal in semitones");
        render(p,4800);near(p.getPitchSnapshot().carrierHz,noteFrequency(60),.0001,"Autobend did not settle into tune");
        near(p.getPitchSnapshot().modulatorHz,noteFrequency(60),.0001,"Autobend left a modulator detuned");
    }
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[glideOn]=1;patch.values[glideCurve]=1;
        patch.values[autobendOn]=1;patch.values[autobendDepth]=12;patch.values[autobendTarget]=3;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,64);
        event(p,juce::MidiMessage::noteOn(1,60,.8f));render(p,9600);event(p,juce::MidiMessage::noteOn(1,72,.8f));render(p,2399);
        const auto snapshot=p.getPitchSnapshot();near(snapshot.carrierHz,noteFrequency(67.5),.02,"Combined Glide/Autobend did not add carrier motion");
        near(snapshot.modulatorHz,noteFrequency(64.5),.02,"Combined Glide/Autobend did not oppose modulator motion");
        event(p,juce::MidiMessage::pitchWheel(1,16383));render(p,48000);
        near(p.getPitchSnapshot().carrierHz,noteFrequency(74-2.0/8192),.001,"Pitch wheel did not compose with settled pitch motion");
        p.getInstrumentSession().set(pitch,12);render(p,48000);
        near(p.getPitchSnapshot().carrierHz,noteFrequency(86-2.0/8192),.001,"Master tuning swallowed the pitch wheel at its upper limit");
    }
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[glideOn]=1;patch.values[glideLegato]=1;
        patch.values[autobendOn]=1;patch.values[autobendDepth]=-12;patch.values[autobendPhrase]=1;patch.values[legato]=1;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,64);
        event(p,juce::MidiMessage::noteOn(1,60,.8f));render(p,9600);
        const auto level=p.getInstrumentSession().live.level.load();event(p,juce::MidiMessage::noteOn(1,67,.8f));
        near(p.getPitchSnapshot().autobendSemitones,0,.0001,"Phrase Autobend retriggered on an overlapping key");
        near(p.getInstrumentSession().live.level.load(),level,.01,"Legato restarted the amplitude envelope");
        event(p,juce::MidiMessage::noteOff(1,67));near(p.getPitchSnapshot().autobendSemitones,0,.0001,"Return to a held key retriggered Autobend");
        event(p,juce::MidiMessage::controllerEvent(1,64,127));event(p,juce::MidiMessage::noteOff(1,60));
        event(p,juce::MidiMessage::noteOn(1,72,.8f));
        near(p.getPitchSnapshot().glideHz,noteFrequency(72),.0001,"Pedal-held notes incorrectly counted as legato keys");
        require(p.getPitchSnapshot().autobendSemitones<-11,"New phrase did not trigger Autobend while pedal held");
        event(p,juce::MidiMessage::noteOff(1,72));event(p,juce::MidiMessage::controllerEvent(1,64,0));render(p,48000);
        require(p.getInstrumentSession().live.voices.load()==0,"Pedal release left a bent mono note sounding");
    }
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.mono=false;patch.values[glideOn]=1;patch.values[glideCurve]=1;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,64);
        event(p,juce::MidiMessage::noteOn(1,48,.8f));render(p,9600);
        juce::AudioBuffer<float> audio(2,1);juce::MidiBuffer chord;
        for(const auto note:{60,64,67})chord.addEvent(juce::MidiMessage::noteOn(1,note,.8f),0);p.processBlock(audio,chord);
        for(const auto note:{60,64,67})near(p.getPitchSnapshot(note).glideHz,noteFrequency(48+(note-48)/4800.0),.001,"Simultaneous chord Glide cascaded through MIDI note order");
        event(p,juce::MidiMessage::allNotesOff(1));render(p,48000);
        require(p.getInstrumentSession().live.voices.load()==0,"Polyphonic glides did not release");
    }
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[glideOn]=1;patch.values[glideCurve]=1;patch.values[glideTime]=1;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,64);event(p,juce::MidiMessage::noteOn(1,60,.8f));
        event(p,juce::MidiMessage::noteOn(1,72,.8f));render(p,12000);
        const auto before=p.getPitchSnapshot().carrierHz;p.getInstrumentSession().set(glideTime,.2);render(p,1);
        require(p.getPitchSnapshot().carrierHz>before&&p.getPitchSnapshot().carrierHz-before<1,"Time automation jumped or reversed an active slide");
        p.getInstrumentSession().set(glideTarget,3);render(p,14400);
        near(p.getPitchSnapshot().carrierHz,noteFrequency(72),.0001,"Live routing/retiming failed to settle");
        near(p.getPitchSnapshot().modulatorHz,noteFrequency(72),.0001,"Live routing left modulator detuned");
    }
    {
        // Tone routing must have an audible filter effect, independent of pitch.
        NitrideAudioProcessor a(directory),b(directory);auto patch=motionPatch();patch.values[cutoff]=700;
        patch.values[autobendOn]=1;patch.values[autobendDepth]=12;patch.values[autobendTarget]=2;
        a.getInstrumentSession().apply(patch);patch.values[autobendTarget]=4;b.getInstrumentSession().apply(patch);
        a.prepareToPlay(48000,256);b.prepareToPlay(48000,256);
        event(a,juce::MidiMessage::noteOn(1,60,.8f));event(b,juce::MidiMessage::noteOn(1,60,.8f));
        double difference=0;juce::AudioBuffer<float> left(2,256),right(2,256);juce::MidiBuffer midi;
        for(int block=0;block<18;++block)
        {
            a.processBlock(left,midi);b.processBlock(right,midi);
            for(int i=0;i<256;++i){const auto delta=left.getSample(0,i)-right.getSample(0,i);difference+=delta*delta;}
        }
        require(difference>.001,"Pitch + Tone did not move the actual filter");
    }
    for(int target=0;target<5;++target)
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.mono=false;patch.values[coupling]=1;patch.values[stress]=1;patch.values[response]=.01;
        patch.values[glideOn]=1;patch.values[glideTime]=.001;patch.values[glideTarget]=target;
        patch.values[autobendOn]=1;patch.values[autobendTime]=.001;patch.values[autobendDepth]=target%2==0?36:-36;patch.values[autobendTarget]=target;
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,64);
        for(const auto note:{24,36,48,60,72,84,96,108,112,116,120,124})event(p,juce::MidiMessage::noteOn(1,note,1.0f));
        render(p,24000);event(p,juce::MidiMessage::allNotesOff(1));render(p,48000);
        require(p.getInstrumentSession().live.voices.load()==0&&p.getInstrumentSession().live.faults.load()==0,"Extreme pitch routing was unstable or failed to release");
    }
}

void independentFmReference()
{
    SoundStudies::Engine engine;engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);engine.setNetworkDimensions(0,0,.025);
    engine.setFmIndex(3);engine.setAmplitudeEnvelope(.001,.01,1,.1);
    SoundStudies::PitchSettings settings;settings.autobend={true,.2,12,SoundStudies::PitchTarget::modulators,false};engine.setPitchMotion(settings);engine.prepare(48000);engine.beginNoteGroup();engine.noteOn(24,1);
    constexpr int samples=12000;std::vector<float> left(samples),right(samples);engine.render(left.data(),right.data(),samples);
    double carrierPhase=0,modulatorPhase=0,age=0,maximumError=0;std::array<double,4> poles{};
    const auto coefficient=1-std::exp(-juce::MathConstants<double>::twoPi*.38/4);
    for(int sample=0;sample<samples;++sample)
    {
        const auto remaining=std::max(0.0,1-static_cast<double>(sample+1)/9600);
        const auto modulatorHz=noteFrequency(24+12*remaining*remaining*remaining);
        double output=0;
        for(int step=0;step<4;++step)
        {
            age+=1/192000.0;const auto env=std::min(1.0,age/.001);
            auto value=std::sin(carrierPhase+3*std::sin(2*modulatorPhase))*env;
            carrierPhase+=juce::MathConstants<double>::twoPi*noteFrequency(24)/192000;
            modulatorPhase+=juce::MathConstants<double>::twoPi*modulatorHz/192000;
            carrierPhase-=juce::MathConstants<double>::twoPi*std::floor(carrierPhase/juce::MathConstants<double>::twoPi);
            modulatorPhase-=juce::MathConstants<double>::twoPi*std::floor(modulatorPhase/juce::MathConstants<double>::twoPi);
            for(auto& pole:poles){pole+=coefficient*(value-pole);value=pole;}output=value;
        }
        const auto expected=.8*std::tanh(output*.16/.8);maximumError=std::max(maximumError,std::abs(expected-left[static_cast<size_t>(sample)]));
    }
    require(maximumError<.000001,"Independent carrier/modulator FM does not match a direct low-frequency FM reference");
    std::cout<<"Independent FM reference maximum error: "<<maximumError<<'\n';
    // At a high modulator frequency all non-carrier sidebands are beyond the
    // audible band. They must disappear, rather than folding into audible aliases.
    SoundStudies::Engine high;high.setTreatment(SoundStudies::Treatment::expressiveNetwork);high.setNetworkDimensions(0,0,.025);
    high.setFmIndex(3);high.setAmplitudeEnvelope(.001,.01,1,.1);settings.autobend={true,5,36,SoundStudies::PitchTarget::modulators,false};
    high.setPitchMotion(settings);high.prepare(48000);high.beginNoteGroup();high.noteOn(96,1);
    left.resize(4800);right.resize(4800);high.render(left.data(),right.data(),4800);
    carrierPhase=0;age=0;poles={};maximumError=0;
    constexpr double j0=-.260051954901933;
    for(size_t sample=0;sample<left.size();++sample)
    {
        double output=0;
        for(int step=0;step<4;++step)
        {
            age+=1/192000.0;auto value=j0*std::sin(carrierPhase)*std::min(1.0,age/.001);
            carrierPhase+=juce::MathConstants<double>::twoPi*noteFrequency(96)/192000;
            carrierPhase-=juce::MathConstants<double>::twoPi*std::floor(carrierPhase/juce::MathConstants<double>::twoPi);
            for(auto& pole:poles){pole+=coefficient*(value-pole);value=pole;}output=value;
        }
        maximumError=std::max(maximumError,std::abs(.8*std::tanh(output*.16/.8)-left[sample]));
    }
    require(maximumError<.000001,"Out-of-band moving FM sidebands aliased into the audible reference source");
    SoundStudies::Engine ultrasonic;ultrasonic.setTreatment(SoundStudies::Treatment::expressiveNetwork);ultrasonic.setNetworkDimensions(1,1,.025);
    ultrasonic.setFmIndex(0);settings.autobend={true,5,36,SoundStudies::PitchTarget::carrier,false};
    ultrasonic.setPitchMotion(settings);ultrasonic.prepare(48000);ultrasonic.beginNoteGroup();ultrasonic.noteOn(108,1);
    ultrasonic.render(left.data(),right.data(),4800);
    double peak=0;for(const auto value:left)peak=std::max(peak,std::abs(static_cast<double>(value)));
    require(peak<.00000001,"An ultrasonic bent carrier folded into an audible network fundamental");
    // Envelope-preserving velocity changes must not introduce a hard amplitude step.
    SoundStudies::Engine legatoVoice;legatoVoice.setTreatment(SoundStudies::Treatment::expressiveNetwork);legatoVoice.setNetworkDimensions(0,0,.025);
    legatoVoice.setFmIndex(0);legatoVoice.setAmplitudeEnvelope(.001,.01,1,.1);SoundStudies::PitchSettings legatoSettings;legatoSettings.legato=true;
    legatoVoice.setPitchMotion(legatoSettings);legatoVoice.prepare(48000);legatoVoice.beginNoteGroup();legatoVoice.noteOn(60,.1f,SoundStudies::Articulation::lead);
    legatoVoice.render(left.data(),right.data(),4800);auto previous=left.back();legatoVoice.beginNoteGroup();legatoVoice.noteOn(67,1,SoundStudies::Articulation::lead,true);
    legatoVoice.render(left.data(),right.data(),4800);double largestStep=0;
    for(size_t i=0;i<144;++i){largestStep=std::max(largestStep,std::abs(static_cast<double>(left[i]-previous)));previous=left[i];}
    require(largestStep<.01,"Legato velocity change introduced a hard amplitude step");
}

std::vector<float> phrase(const juce::File& directory,int block,double rate)
{
    NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[glideOn]=1;patch.values[glideTarget]=3;
    patch.values[autobendOn]=1;patch.values[autobendTarget]=0;patch.values[autobendDepth]=-7;
    p.getInstrumentSession().apply(patch);p.prepareToPlay(rate,block);
    const auto samples=static_cast<int>(rate*.5);std::vector<float> result;result.reserve(static_cast<size_t>(samples));
    const std::array positions{13,static_cast<int>(rate*.08),static_cast<int>(rate*.16),static_cast<int>(rate*.28)};
    for(int offset=0;offset<samples;offset+=block)
    {
        const auto size=std::min(block,samples-offset);juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;
        for(size_t i=0;i<positions.size();++i)if(positions[i]>=offset&&positions[i]<offset+size)
            midi.addEvent(i==3?juce::MidiMessage::allNotesOff(1):juce::MidiMessage::noteOn(1,60+static_cast<int>(i)*5,.8f),positions[i]-offset);
        p.processBlock(audio,midi);result.insert(result.end(),audio.getReadPointer(0),audio.getReadPointer(0)+size);
    }
    return result;
}
template<class T>T* child(juce::Component& root,const juce::String& name)
{
    for(auto* c:root.getChildren()){if(auto* found=dynamic_cast<T*>(c);found&&c->getName()==name)return found;if(auto* found=child<T>(*c,name))return found;}return nullptr;
}
void integration(const juce::File& directory)
{
    NitrideAudioProcessor p(directory);auto& s=p.getInstrumentSession();
    require(p.getParameters()[16]->getName(64)=="Mono","Appending motion controls moved the legacy mono host index");
    for(int i=0;i<legacyParameterCount;++i)require(p.getParameters()[i]->getName(64)==hostParameters[static_cast<size_t>(i)].name,"Legacy host parameter order changed");
    auto patch=motionPatch();patch.values[glideOn]=1;patch.values[glideTarget]=3;patch.values[autobendOn]=1;patch.values[autobendDepth]=-12;s.apply(patch);
    s.beginEdit({hostIndex(autobendDepth)});s.set(autobendDepth,7);s.set(autobendDepth,9);s.endEdit({hostIndex(autobendDepth)});s.undo();near(s.read().values[autobendDepth],-12,0,"Pitch edit undo used the wrong host/condition mapping");s.redo();
    s.setComparing(true);juce::MemoryBlock state;p.getStateInformation(state);
    NitrideAudioProcessor restored(directory);restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    near(restored.getInstrumentSession().read().values[autobendDepth],9,0,"State recall lost edited Autobend while comparing");
    near(restored.getInstrumentSession().read().values[glideTarget],3,0,"State lost pitch routing");
    require(s.keep("Pitch motion saved").wasOk(),"Cannot keep pitch-motion preset");
    NitrideAudioProcessor reloaded(directory);require(reloaded.getInstrumentSession().selectPreset(s.selectedPresetId()),"Kept pitch preset not reloadable");
    near(reloaded.getInstrumentSession().read().values[autobendDepth],9,0,"Disk preset lost pitch motion");
    auto legacy=PresetStore::encode({"legacy",factoryPatches()[2]});legacy.getDynamicObject()->setProperty("version",1);
    for(size_t i=legacyParameterCount;i<hostParameters.size();++i)legacy["values"].getDynamicObject()->removeProperty(hostParameters[i].id);
    const auto migrated=PresetStore::decode(legacy);require(migrated.has_value(),"Legacy preset could not migrate");
    near(migrated->patch.values[glideOn],1,0,"Legacy mono glide was not enabled");near(migrated->patch.values[glideCurve],2,0,"Legacy mono curve changed");near(migrated->patch.values[autobendOn],0,0,"Migration enabled Autobend");
    near(migrated->patch.values[glideTime],.025,0,"Legacy mono glide time changed");
    auto oldDocument=s.saveDocument();oldDocument.getDynamicObject()->setProperty("version",2);
    oldDocument.getDynamicObject()->setProperty("undo",juce::Array<juce::var>{});oldDocument.getDynamicObject()->setProperty("redo",juce::Array<juce::var>{});oldDocument.getDynamicObject()->setProperty("kept",juce::Array<juce::var>{});
    oldDocument["current"].getDynamicObject()->setProperty("patch",legacy);oldDocument["current"].getDynamicObject()->setProperty("reference",legacy);
    require(restored.getInstrumentSession().restoreDocument(oldDocument).wasOk(),"Milestone-2 session could not migrate embedded legacy conditions");
    near(restored.getInstrumentSession().read().values[glideTime],.025,0,"Migrated session lost Classic Glide");
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->addToDesktop(0);editor->setVisible(true);
    auto* depth=child<juce::Slider>(*editor,"Autobend depth");auto* destination=child<juce::ComboBox>(*editor,"Glide destination");
    require(depth&&destination,"Native pitch controls missing");depth->setValue(-5,juce::sendNotificationSync);
    near(s.read().values[autobendDepth],-5,0,"Native depth control not connected");
    destination->setSelectedId(2,juce::sendNotificationSync);near(s.read().values[glideTarget],1,0,"Native destination control not connected");
    auto* hostDepth=p.getState().getParameter("autobend_depth");hostDepth->setValueNotifyingHost(hostDepth->convertTo0to1(12));
    for(int i=0;i<100&&std::abs(depth->getValue()-12)>.001;++i)juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    near(depth->getValue(),12,.001,"Host Autobend automation not displayed");
    p.getStateInformation(state);depth->onDragStart();depth->setValue(20,juce::sendNotificationSync);
    p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));depth->setValue(25,juce::sendNotificationSync);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(80);depth->setValue(30,juce::sendNotificationSync);depth->onDragEnd();
    near(s.read().values[autobendDepth],12,.001,"A stale pitch-control drag overwrote restored conditions");
    editor.reset();
    p.prepareToPlay(48000,256);
    std::thread host([&]{for(int i=0;i<2000;++i)s.storeFromHost(hostIndex(autobendDepth),static_cast<double>(i%73)-36);});
    render(p,48000);host.join();require(s.live.faults.load()==0,"Concurrent host controls caused pitch faults");
}

void renderDemos(const juce::File& directory,const juce::File& output)
{
    require(output.createDirectory().wasOk(),"Cannot create demo directory");
    const std::array names{"01-plain","02-glide-all","03-glide-opposed","04-autobend-all","05-autobend-opposed","06-glide-plus-autobend","07-pitch-and-tone"};
    for(size_t demo=0;demo<names.size();++demo)
    {
        NitrideAudioProcessor p(directory);auto patch=motionPatch();patch.values[spaceMix]=.12;patch.values[glideTime]=.3;patch.values[autobendTime]=.25;patch.values[autobendDepth]=12;
        if(demo==1||demo==2||demo==5||demo==6)patch.values[glideOn]=1;
        if(demo==2)patch.values[glideTarget]=3;
        if(demo>=3&&demo<=6)patch.values[autobendOn]=1;
        if(demo==4||demo==5)patch.values[autobendTarget]=3;
        if(demo==6){patch.values[glideTarget]=4;patch.values[autobendTarget]=4;patch.values[cutoff]=2500;}
        p.getInstrumentSession().apply(patch);p.prepareToPlay(48000,256);
        juce::AudioBuffer<float> outputAudio(2,48000*6);juce::MidiBuffer all;
        const std::array notes{48,55,60,63,48};
        for(size_t i=0;i<notes.size();++i)all.addEvent(juce::MidiMessage::noteOn(1,notes[i],.8f),static_cast<int>(i)*48000);
        all.addEvent(juce::MidiMessage::allNotesOff(1),48000*5);
        for(int offset=0;offset<outputAudio.getNumSamples();offset+=256)
        {
            const auto size=std::min(256,outputAudio.getNumSamples()-offset);float* pointers[]{outputAudio.getWritePointer(0,offset),outputAudio.getWritePointer(1,offset)};
            juce::AudioBuffer<float> block(pointers,2,size);juce::MidiBuffer midi;midi.addEvents(all,offset,size,-offset);p.processBlock(block,midi);
        }
        const auto file=output.getChildFile(juce::String(names[demo])+".wav");file.deleteFile();
        juce::WavAudioFormat wav;std::unique_ptr<juce::OutputStream> stream=file.createOutputStream();
        auto writer=wav.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(48000).withNumChannels(2).withBitsPerSample(24));
        require(writer&&writer->writeFromAudioSampleBuffer(outputAudio,0,outputAudio.getNumSamples()),"Cannot write musical demo");
        std::cout<<file.getFullPathName()<<'\n';
    }
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        TestDirectory directory;
        if(argc==3&&juce::String(argv[1])=="--render-demo"){renderDemos(directory.folder,juce::File(argv[2]));return 0;}
        pitchBehavior(directory.folder);independentFmReference();integration(directory.folder);
        for(const auto rate:{44100.0,48000.0,96000.0})
        {
            const auto a=phrase(directory.folder,64,rate),b=phrase(directory.folder,257,rate);double error=0;
            for(size_t i=0;i<a.size();++i)error=std::max(error,std::abs(static_cast<double>(a[i]-b[i])));
            require(error<.00001,"Pitch trajectories depend on render block boundaries");
        }
        std::cout<<"Pitch motion passed: timing, signed routing, Opposed, composition, legato/mono/pedal, poly chords, independent FM, host controls, persistence, migration and block invariance.\n";return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
