#include <AudioToolbox/AudioToolbox.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../Instrument/Parameters.h"
#include "FactoryPrograms.h"

namespace
{
void check(OSStatus result,const char* operation)
{
    if(result!=noErr)throw std::runtime_error(std::string(operation)+" failed: "+std::to_string(result));
}
struct Instance
{
    AudioUnit unit=nullptr;
    ~Instance(){if(unit){AudioUnitUninitialize(unit);AudioComponentInstanceDispose(unit);}}
};
}

int main(int argc,char** argv)
{
    try
    {
        AudioComponentDescription description{};
        description.componentType=kAudioUnitType_MusicDevice;
        description.componentSubType='NT01';description.componentManufacturer='NTRD';
        const auto component=AudioComponentFindNext(nullptr,&description);
        if(component==nullptr)throw std::runtime_error("Installed Nitride AU not found; run make install-au first");
        Instance instance;check(AudioComponentInstanceNew(component,&instance.unit),"AudioComponentInstanceNew");
        UInt32 listSize=0;Boolean writable=false;
        check(AudioUnitGetPropertyInfo(instance.unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,&listSize,&writable),"Parameter list info");
        std::vector<AudioUnitParameterID> parameterIds(listSize/sizeof(AudioUnitParameterID));
        check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,parameterIds.data(),&listSize),"Parameter list");
        const bool dump=argc==2&&std::string(argv[1])=="--dump-parameters";
        if(!dump&&parameterIds.size()!=Nitride::hostParameterCount)throw std::runtime_error("Installed AU does not publish the current automation contract");
        AudioUnitParameterID couplingId=0;bool foundCoupling=false;
        AudioUnitParameterID depthId=0,autobendId=0;bool foundDepth=false,foundAutobend=false;
        const auto hash=[](const char* text){std::uint32_t value=0;while(*text)value=31*value+static_cast<unsigned char>(*text++);return value&0x7fffffffu;};
        std::array<std::pair<const char*,AudioUnitParameterID>,Nitride::hostParameterCount> legacyIds{};
        for(int h=0;h<Nitride::hostParameterCount;++h)
        {
            if(h==Nitride::monoHostIndex)legacyIds[static_cast<size_t>(h)]={"Mono",hash("mono")};
            else{const auto& p=Nitride::hostParameters[static_cast<size_t>(Nitride::conditionIndex(h))];legacyIds[static_cast<size_t>(h)]={p.name,hash(p.id)};}
        }
        std::array<bool,legacyIds.size()> foundIds{};
        for(const auto id:parameterIds)
        {
            AudioUnitParameterInfo info{};UInt32 infoSize=sizeof(info);
            check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ParameterInfo,kAudioUnitScope_Global,id,&info,&infoSize),"Parameter info");
            std::array<char,256> name{};
            if(info.cfNameString!=nullptr)CFStringGetCString(info.cfNameString,name.data(),static_cast<CFIndex>(name.size()),kCFStringEncodingUTF8);
            else std::copy(std::begin(info.name),std::end(info.name),name.begin());
            if(std::string(name.data())=="Coupling"){couplingId=id;foundCoupling=true;}
            if(std::string(name.data())=="Autobend depth"){depthId=id;foundDepth=true;}
            if(std::string(name.data())=="Autobend"){autobendId=id;foundAutobend=true;}
            if(dump)std::cout<<name.data()<<"\t"<<id<<'\n';
            if(!dump)for(size_t i=0;i<legacyIds.size();++i)if(std::string(name.data())==legacyIds[i].first)
            {if(id!=legacyIds[i].second)throw std::runtime_error("Current AU parameter ID differs from its declared metadata");foundIds[i]=true;}
            if((info.flags&kAudioUnitParameterFlag_CFNameRelease)!=0&&info.cfNameString!=nullptr)CFRelease(info.cfNameString);
        }
        if(dump)return 0;
        UInt32 factoryInfoSize=0;Boolean factoryWritable=false;
        if(AudioUnitGetPropertyInfo(instance.unit,kAudioUnitProperty_FactoryPresets,kAudioUnitScope_Global,0,&factoryInfoSize,&factoryWritable)!=kAudioUnitErr_InvalidProperty)
            throw std::runtime_error("AU advertises the duplicate host factory-presets submenu");
        CFArrayRef factory=nullptr;UInt32 factorySize=sizeof(factory);
        const auto factoryStatus=AudioUnitGetProperty(instance.unit,kAudioUnitProperty_FactoryPresets,kAudioUnitScope_Global,0,&factory,&factorySize);
        if(factoryStatus!=kAudioUnitErr_InvalidProperty||factory!=nullptr)
        {
            if(factory)CFRelease(factory);
            throw std::runtime_error("AU still returns an advertised factory-presets list");
        }
        constexpr std::array reviewNames{
            "Review / Pads / Soft Alloy r1","Review / Basses / Tension Core r1",
            "Review / Pads / Soft Alloy r2","Review / Basses / Tension Core r2",
            "Review / Leads / Glass Current r1","Review / Sequence elements / Pulse Lattice r1",
            "Review / Leads / Glass Current r2","Review / Sequence elements / Pulse Lattice r2",
            "Review / Physical materials / Hollow Bronze r1","Review / Experimental / Fault Bloom r1",
            "Review / Physical materials / Hollow Bronze r2","Review / Pads / Prism Pulse r1",
            "Review / Basses / Felt Weight r1","Review / Pads / Prism Pulse r2",
            "Review / Pads / Velvet Pulse r3","Review / Leads / Reed Snap r1",
            "Review / Sequence elements / Undertow Grid r1","Review / Leads / Reed Snap r2",
            "Review / Physical materials / Skin Knock r1","Review / Experimental / Orbit Shards r1",
            "Review / Physical materials / Taffy Coil r1","Review / Pads / Stillwater r1",
            "Review / Basses / Crooked Velvet r1","Review / Basses / Crooked Velvet r2",
            "Review / Leads / Replica Voice r1","Review / Experimental / Tendon Drift r1",
            "Review / Sequence elements / Socket Dance r1",
            "Review / Leads / Replica Voice r2","Review / Sequence elements / Socket Dance r2",
            "Review / Basses / Sub Canvas r1","Review / Leads / Afterimage Keys r1",
            "Review / Experimental / Overtone Lantern r1","Review / Sequence elements / Buzz Loom r1",
            "Review / Physical materials / Cutoff Alloy r1","Review / Experimental / Brake Bloom r1",
            "Review / Physical materials / Tine Keys r2","Review / Physical materials / Rosewood Tap r1",
            "Review / Leads / Clear Stops r1","Review / Leads / Copper Lift r1","Review / Basses / Pocket Slap r1",
            "Review / Physical materials / Floor Pulse r1","Review / Physical materials / Rivet Kick r1","Review / Physical materials / Bowl Toms r1",
            "Review / Physical materials / Bowl Toms r2",
            "Review / Experimental / Pressure Lift r1","Review / Experimental / Sinkhole r1","Review / Sequence elements / Shred Relay r1",
            "Review / Experimental / Reverse Veil r1","Review / Experimental / Spectral Cut r1","Review / Experimental / Flash Room r1",
            "Review / Physical materials / Classic Tines r1","Review / Basses / Classic Bass r1","Review / Physical materials / Classic Tubes r1",
            "Review / Pads / Classic Voice r1","Review / Physical materials / Orch Chime r1","Review / Physical materials / Classic Koto r1",
            "Review / Leads / Whistle Stem r1","Review / Physical materials / Jaw Twang r1","Review / Physical materials / Vowel Pick r1"};
        if(Nitride::legacyProgramNames.size()!=4+reviewNames.size())throw std::runtime_error("Legacy fixture coverage changed");
        for(CFIndex i=0;i<static_cast<CFIndex>(Nitride::legacyProgramNames.size());++i)
        {
            if(i>=4&&std::string(Nitride::legacyProgramNames[static_cast<size_t>(i)])!=reviewNames[static_cast<size_t>(i-4)])throw std::runtime_error("Legacy review names were rewritten");
            const auto name=CFStringCreateWithCString(nullptr,Nitride::legacyProgramNames[static_cast<size_t>(i)],kCFStringEncodingUTF8);
            const AUPreset requested{static_cast<SInt32>(i),name};const auto* preset=&requested;
            check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_PresentPreset,kAudioUnitScope_Global,0,preset,sizeof(*preset)),"Select installed review program");
            CFRelease(name);
            CFPropertyListRef saved=nullptr;UInt32 savedSize=sizeof(saved);check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&saved,&savedSize),"Review class state");
            const AUPreset other{2,CFSTR("03 / Contact lead")};
            check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_PresentPreset,kAudioUnitScope_Global,0,&other,sizeof(other)),"Change program before review recall");
            check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&saved,sizeof(saved)),"Review class-state recall");
            CFPropertyListRef recalled=nullptr;UInt32 recalledSize=sizeof(recalled);
            check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&recalled,&recalledSize),"Recalled review document");
            const auto before=CFDictionaryGetValue(static_cast<CFDictionaryRef>(saved),CFSTR("jucePluginState"));
            const auto after=CFDictionaryGetValue(static_cast<CFDictionaryRef>(recalled),CFSTR("jucePluginState"));
            const auto same=before&&after&&CFEqual(before,after);CFRelease(recalled);CFRelease(saved);
            if(!same)throw std::runtime_error("Installed AU failed exact review document/conditions recall");
            if(i==5||i==7)
            {
                AudioUnitParameterValue enabled=0;check(AudioUnitGetParameter(instance.unit,autobendId,kAudioUnitScope_Global,0,&enabled),"Bass revision Autobend state");
                if(std::abs(enabled-(i==5?1.0f:0.0f))>.0001f)throw std::runtime_error("Bass r1/r2 attack distinction lost on installed AU recall");
            }
            if(i==8||i==10)
            {
                AudioUnitParameterValue coupling=0,stress=0;
                check(AudioUnitGetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,&coupling),"Lead revision coupling state");
                check(AudioUnitGetParameter(instance.unit,hash("stress"),kAudioUnitScope_Global,0,&stress),"Lead revision stress state");
                if(std::abs(coupling-(i==8?.52f:.46f))>.0001f||std::abs(stress-(i==8?.09f:.065f))>.0001f)throw std::runtime_error("Lead r1/r2 network reduction lost on installed AU recall");
            }
            if(i==9||i==11)
            {
                AudioUnitParameterValue wave=0,target=0;
                check(AudioUnitGetParameter(instance.unit,hash("lfo_fm_wave"),kAudioUnitScope_Global,0,&wave),"Sequence revision FM waveform");
                check(AudioUnitGetParameter(instance.unit,hash("fm_route_3_target"),kAudioUnitScope_Global,0,&target),"Sequence revision ratio route");
                if(std::abs(wave-(i==9?1.0f:4.0f))>.0001f||std::abs(target-(i==9?0.0f:1.0f))>.0001f)throw std::runtime_error("Sequence r1/r2 stepped harmonic route lost on installed AU recall");
            }
            if(i==12||i==13||i==14)
            {
                AudioUnitParameterValue mono=0,glide=0,sustain=0;
                check(AudioUnitGetParameter(instance.unit,hash("mono"),kAudioUnitScope_Global,0,&mono),"Material/experimental mono state");
                check(AudioUnitGetParameter(instance.unit,hash("glide_target"),kAudioUnitScope_Global,0,&glide),"Material/experimental glide routing");
                check(AudioUnitGetParameter(instance.unit,hash("amp_sustain"),kAudioUnitScope_Global,0,&sustain),"Material/experimental sustain state");
                if(std::abs(mono-(i==13?1.0f:0.0f))>.0001f||std::abs(glide-(i==13?3.0f:2.0f))>.0001f||std::abs(sustain-(i==13?.62f:0.0f))>.0001f)throw std::runtime_error("Material strike/experimental opposed-glide distinction lost on installed AU recall");
            }
            if(i==12||i==14)
            {
                AudioUnitParameterValue base=0,amount=0;
                check(AudioUnitGetParameter(instance.unit,hash("fm_depth"),kAudioUnitScope_Global,0,&base),"Bronze revision settled FM");
                check(AudioUnitGetParameter(instance.unit,hash("fm_envelope_amount"),kAudioUnitScope_Global,0,&amount),"Bronze revision strike compensation");
                if(std::abs(base-(i==12?.42f:.22f)/4)>.0001f||std::abs(amount-((i==12?2.2f:2.4f)+4)/8)>.0001f)throw std::runtime_error("Bronze r1/r2 tail-FM/strike compensation lost on installed AU recall");
            }
            if(i==15||i==16||i==17||i==18)
            {
                AudioUnitParameterValue mono=0,motion=0,legato=0;
                check(AudioUnitGetParameter(instance.unit,hash("mono"),kAudioUnitScope_Global,0,&mono),"Contrast mono state");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_amplitude_enabled"),kAudioUnitScope_Global,0,&motion),"Contrast rhythmic amplitude state");
                check(AudioUnitGetParameter(instance.unit,hash("legato"),kAudioUnitScope_Global,0,&legato),"Contrast retrigger articulation");
                if(std::abs(mono-(i==16?1.0f:0.0f))>.0001f||std::abs(motion-(i==16?0.0f:1.0f))>.0001f||std::abs(legato)>.0001f)throw std::runtime_error("New pad/bass contrast behavior lost on installed AU recall");
            }
            if(i==15||i==17||i==18)
            {
                AudioUnitParameterValue ratioMotion=0,shape=0,mix=0;
                check(AudioUnitGetParameter(instance.unit,hash("fm_route_1_amount"),kAudioUnitScope_Global,0,&ratioMotion),"Prism ratio-motion state");
                check(AudioUnitGetParameter(instance.unit,hash("fm_shape_amount"),kAudioUnitScope_Global,0,&shape),"Prism shaping state");
                check(AudioUnitGetParameter(instance.unit,hash("fm_operator_mix"),kAudioUnitScope_Global,0,&mix),"Prism operator mix state");
                if(std::abs(ratioMotion-((i==15?.035f:0.0f)+4)/8)>.0001f||std::abs(shape-(i==15?.45f:.18f))>.0001f||std::abs(mix-(i==15?.09f:.04f))>.0001f)throw std::runtime_error("Prism r1/r2 chord-FM distinction lost on installed AU recall");
            }
            if(i==19||i==20||i==21)
            {
                AudioUnitParameterValue mono=0,motion=0,legato=0;
                check(AudioUnitGetParameter(instance.unit,hash("mono"),kAudioUnitScope_Global,0,&mono),"Lead/sequence contrast mono");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_amplitude_enabled"),kAudioUnitScope_Global,0,&motion),"Lead/sequence contrast pulse");
                check(AudioUnitGetParameter(instance.unit,hash("legato"),kAudioUnitScope_Global,0,&legato),"Lead/sequence contrast articulation");
                if(std::abs(mono-(i==20?1.0f:0.0f))>.0001f||std::abs(motion-(i==20?1.0f:0.0f))>.0001f||std::abs(legato-(i==20?1.0f:0.0f))>.0001f)throw std::runtime_error("Poly retriggered lead/held mono sequence distinction lost on AU recall");
            }
            if(i==19||i==21)
            {
                AudioUnitParameterValue shape=0,source=0,target=0;
                check(AudioUnitGetParameter(instance.unit,hash("fm_shape"),kAudioUnitScope_Global,0,&shape),"Reed waveform profile");
                check(AudioUnitGetParameter(instance.unit,hash("fm_route_2_source"),kAudioUnitScope_Global,0,&source),"Reed transient shape source");
                check(AudioUnitGetParameter(instance.unit,hash("fm_route_2_target"),kAudioUnitScope_Global,0,&target),"Reed transient shape target");
                if(std::abs(shape-(i==19?2.0f:1.0f))>.0001f||std::abs(source-(i==19?0.0f:3.0f))>.0001f||std::abs(target-(i==19?0.0f:2.0f))>.0001f)throw std::runtime_error("Reed r1/r2 harmonic/transient distinction lost on AU recall");
            }
            if(i==22||i==23)
            {
                AudioUnitParameterValue mono=0,bend=0,pitchMotion=0,pitchWave=0,sync=0;
                check(AudioUnitGetParameter(instance.unit,hash("mono"),kAudioUnitScope_Global,0,&mono),"Material/shards poly state");
                check(AudioUnitGetParameter(instance.unit,autobendId,kAudioUnitScope_Global,0,&bend),"Material pitch relaxation");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_pitch_enabled"),kAudioUnitScope_Global,0,&pitchMotion),"Shards pitch motion");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_pitch_wave"),kAudioUnitScope_Global,0,&pitchWave),"Shards stepped pitch");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_pitch_sync"),kAudioUnitScope_Global,0,&sync),"Material/shards free motion");
                if(std::abs(mono)>.0001f||std::abs(bend-(i==22?1.0f:0.0f))>.0001f||std::abs(pitchMotion-(i==22?0.0f:1.0f))>.0001f||std::abs(pitchWave-(i==22?0.0f:4.0f))>.0001f||std::abs(sync)>.0001f)throw std::runtime_error("Skin impact/Orbit poly stepped-pitch distinction lost on AU recall");
            }
            if(i>=24&&i<=27)
            {
                AudioUnitParameterValue mono=0,bend=0,target=0,pulse=0,sync=0;
                check(AudioUnitGetParameter(instance.unit,hash("mono"),kAudioUnitScope_Global,0,&mono),"Artist-lens poly/mono");
                check(AudioUnitGetParameter(instance.unit,autobendId,kAudioUnitScope_Global,0,&bend),"Artist-lens onset motion");
                check(AudioUnitGetParameter(instance.unit,hash("autobend_target"),kAudioUnitScope_Global,0,&target),"Artist-lens bend routing");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_amplitude_enabled"),kAudioUnitScope_Global,0,&pulse),"Artist-lens free amplitude motion");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_amplitude_sync"),kAudioUnitScope_Global,0,&sync),"Artist-lens free rate");
                if(std::abs(mono-(i>=26?1.0f:0.0f))>.0001f||std::abs(bend-(i==25?0.0f:1.0f))>.0001f||std::abs(target-(i==24?3.0f:i>=26?1.0f:0.0f))>.0001f||std::abs(pulse-(i>=26?1.0f:0.0f))>.0001f||std::abs(sync)>.0001f)throw std::runtime_error("Artist-lens strike/patient-pad/loping-bass distinction lost on AU recall");
            }
            if(i==26||i==27)
            {
                AudioUnitParameterValue mix=0,index=0,route=0;
                check(AudioUnitGetParameter(instance.unit,hash("fm_operator_mix"),kAudioUnitScope_Global,0,&mix),"Crooked carrier blend");
                check(AudioUnitGetParameter(instance.unit,hash("fm_depth"),kAudioUnitScope_Global,0,&index),"Crooked settled index");
                check(AudioUnitGetParameter(instance.unit,hash("fm_route_3_target"),kAudioUnitScope_Global,0,&route),"Crooked random color destination");
                if(std::abs(mix-(i==26?.11f:.035f))>.0001f||std::abs(index-(i==26?.8f:.4f)/4)>.0001f||std::abs(route-(i==26?1.0f:0.0f))>.0001f)throw std::runtime_error("Crooked r1/r2 root-anchor distinction lost on AU recall");
            }
            if(i>=28&&i<=30)
            {
                AudioUnitParameterValue mono=0,glide=0,target=0,pulse=0,sync=0;
                check(AudioUnitGetParameter(instance.unit,hash("mono"),kAudioUnitScope_Global,0,&mono),"New lens mono/poly");
                check(AudioUnitGetParameter(instance.unit,hash("glide_enabled"),kAudioUnitScope_Global,0,&glide),"New lens pitch drag");
                check(AudioUnitGetParameter(instance.unit,hash("glide_target"),kAudioUnitScope_Global,0,&target),"New lens Tone drag destination");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_amplitude_enabled"),kAudioUnitScope_Global,0,&pulse),"New lens chopped cell");
                check(AudioUnitGetParameter(instance.unit,hash("lfo_amplitude_sync"),kAudioUnitScope_Global,0,&sync),"New lens host clock");
                if(std::abs(mono-(i==29?1.0f:0.0f))>.0001f||std::abs(glide-(i==29?1.0f:0.0f))>.0001f||std::abs(target-(i==29?4.0f:2.0f))>.0001f||std::abs(pulse-(i==30?1.0f:0.0f))>.0001f||std::abs(sync-(i==30?1.0f:0.0f))>.0001f)throw std::runtime_error("Replica/Tendon/Socket voice-drag-cell distinction lost on AU recall");
            }
            if(i==28||i==30||i==31||i==32)
            {
                AudioUnitParameterValue tail=0;
                check(AudioUnitGetParameter(instance.unit,hash("amp_release"),kAudioUnitScope_Global,0,&tail),"Tail-only revision amplitude release");
                const auto seconds=i==28?.9:i==30?.1:i==31?.4:.045;
                const auto expected=static_cast<float>(std::pow((seconds-.02)/(8-.02),.35));
                if(std::abs(tail-expected)>.0001f)throw std::runtime_error("Replica/Socket r1/r2 amplitude release distinction lost on AU recall");
            }
        }
        // Reset to the original lead for the established automation/MIDI probe.
        const AUPreset lead{2,CFSTR("03 / Contact lead")};check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_PresentPreset,kAudioUnitScope_Global,0,&lead,sizeof(lead)),"Restore hidden original lead program");
        for(const auto invalid:{static_cast<int>(Nitride::legacyProgramNames.size()),10000})
        {
            const AUPreset requested{invalid,CFSTR("Invalid")};
            if(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_PresentPreset,kAudioUnitScope_Global,0,&requested,sizeof(requested))==noErr)throw std::runtime_error("AU accepted an invalid legacy program number");
        }
        // AU reserves negative preset numbers for named user settings. They are
        // metadata, not invalid factory selections, and must not change the sound.
        CFPropertyListRef beforeCustom=nullptr;UInt32 beforeCustomSize=sizeof(beforeCustom);
        check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&beforeCustom,&beforeCustomSize),"Before user preset label");
        const AUPreset custom{-1,CFSTR("User saved setting")};
        check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_PresentPreset,kAudioUnitScope_Global,0,&custom,sizeof(custom)),"Set AU user preset metadata");
        CFPropertyListRef afterCustom=nullptr;UInt32 afterCustomSize=sizeof(afterCustom);
        check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&afterCustom,&afterCustomSize),"After user preset label");
        const auto sameCustom=CFEqual(CFDictionaryGetValue(static_cast<CFDictionaryRef>(beforeCustom),CFSTR("jucePluginState")),CFDictionaryGetValue(static_cast<CFDictionaryRef>(afterCustom),CFSTR("jucePluginState")));
        CFRelease(beforeCustom);CFRelease(afterCustom);
        if(!sameCustom)throw std::runtime_error("AU user preset metadata changed the full sound document");
        std::cout<<"Installed AU advertises no FactoryPresets list and exactly recalls all "<<Nitride::legacyProgramNames.size()<<" programs, including the appended FS1R partial studies\n";
        for(const auto found:foundIds)if(!found)throw std::runtime_error("A current AU parameter name disappeared");
        if(!foundCoupling)throw std::runtime_error("AU Coupling automation parameter missing");
        if(!foundDepth||!foundAutobend)throw std::runtime_error("AU Autobend controls missing");
        // JUCE's AU continuous-parameter API uses 0..1; value strings display
        // physical units. Indexed destinations use their discrete index instead.
        constexpr float normalizedDepth=(12.0f+36.0f)/72.0f;
        check(AudioUnitSetParameter(instance.unit,depthId,kAudioUnitScope_Global,0,normalizedDepth,0),"Autobend depth write");
        check(AudioUnitSetParameter(instance.unit,autobendId,kAudioUnitScope_Global,0,1,0),"Autobend enable write");
        AudioUnitParameterValue writtenDepth=0;check(AudioUnitGetParameter(instance.unit,depthId,kAudioUnitScope_Global,0,&writtenDepth),"Autobend depth read");
        if(std::abs(writtenDepth-normalizedDepth)>.0001f)throw std::runtime_error("AU did not retain Autobend write: "+std::to_string(writtenDepth));
        check(AudioUnitSetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,.82f,0),"Host parameter write");
        constexpr AudioUnitParameterID filterAmountId=1078209279;
        check(AudioUnitSetParameter(instance.unit,filterAmountId,kAudioUnitScope_Global,0,.75f,0),"Filter envelope amount write");
        const auto lfoId=hash("lfo_stress_depth");check(AudioUnitSetParameter(instance.unit,lfoId,kAudioUnitScope_Global,0,.65f,0),"LFO depth write");
        const std::array filterModulationIds{hash("filter_key_tracking"),hash("filter_velocity"),hash("filter_fm_depth")};
        constexpr std::array filterModulationValues{.8f,.7f,.5f};
        for(size_t i=0;i<filterModulationIds.size();++i)check(AudioUnitSetParameter(instance.unit,filterModulationIds[i],kAudioUnitScope_Global,0,filterModulationValues[i],0),"Filter modulation write");
        std::array<AudioUnitParameterID,Nitride::parameterCount-Nitride::fmRatio> fmIds{};
        for(size_t i=0;i<fmIds.size();++i){const auto& d=Nitride::hostParameters[Nitride::fmRatio+i];fmIds[i]=hash(d.id);check(AudioUnitSetParameter(instance.unit,fmIds[i],kAudioUnitScope_Global,0,d.kind==Nitride::ParameterKind::choice?1:.75f,0),"Advanced FM write");}
        AudioUnitParameterValue retained=0;check(AudioUnitGetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,&retained),"Host parameter read");
        if(std::abs(retained-.82f)>.0001f)throw std::runtime_error("AU did not retain host automation value");
        CFPropertyListRef savedState=nullptr;UInt32 stateSize=sizeof(savedState);
        check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&savedState,&stateSize),"AU session state");
        check(AudioUnitSetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,.2f,0),"Changed host parameter");
        check(AudioUnitSetParameter(instance.unit,filterAmountId,kAudioUnitScope_Global,0,.1f,0),"Changed filter amount");
        for(const auto id:filterModulationIds)check(AudioUnitSetParameter(instance.unit,id,kAudioUnitScope_Global,0,0,0),"Changed filter modulation");
        for(const auto id:fmIds)check(AudioUnitSetParameter(instance.unit,id,kAudioUnitScope_Global,0,0,0),"Changed advanced FM");
        check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&savedState,sizeof(savedState)),"AU session restore");
        CFRelease(savedState);
        check(AudioUnitGetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,&retained),"Restored host parameter");
        if(std::abs(retained-.82f)>.0001f)throw std::runtime_error("Installed AU lost automation conditions during state restore");
        check(AudioUnitGetParameter(instance.unit,depthId,kAudioUnitScope_Global,0,&retained),"Restored Autobend depth");
        if(std::abs(retained-normalizedDepth)>.0001f)throw std::runtime_error("Installed AU lost Autobend depth during state restore: "+std::to_string(retained));
        check(AudioUnitGetParameter(instance.unit,filterAmountId,kAudioUnitScope_Global,0,&retained),"Restored filter amount");
        if(std::abs(retained-.75f)>.0001f)throw std::runtime_error("Installed AU lost filter envelope during state restore");
        check(AudioUnitGetParameter(instance.unit,lfoId,kAudioUnitScope_Global,0,&retained),"Restored LFO depth");
        if(std::abs(retained-.65f)>.0001f)throw std::runtime_error("Installed AU lost LFO depth during state restore");
        for(size_t i=0;i<filterModulationIds.size();++i)
        {
            check(AudioUnitGetParameter(instance.unit,filterModulationIds[i],kAudioUnitScope_Global,0,&retained),"Restored filter modulation");
            if(std::abs(retained-filterModulationValues[i])>.0001f)throw std::runtime_error("Installed AU lost filter modulation during state restore");
        }
        for(size_t i=0;i<fmIds.size();++i)
        {
            check(AudioUnitGetParameter(instance.unit,fmIds[i],kAudioUnitScope_Global,0,&retained),"Restored advanced FM");
            const auto& d=Nitride::hostParameters[Nitride::fmRatio+i];
            const auto expected=d.kind==Nitride::ParameterKind::choice?1:.75f;
            if(std::abs(retained-expected)>.0001f)throw std::runtime_error(std::string("Installed AU lost ")+d.name+" during state restore: "+std::to_string(retained));
        }
        std::cout<<"Installed AU automation/state verified\n"<<std::flush;
        AudioStreamBasicDescription format{};
        format.mSampleRate=48000;format.mFormatID=kAudioFormatLinearPCM;
        format.mFormatFlags=static_cast<AudioFormatFlags>(kAudioFormatFlagsNativeFloatPacked)
            |static_cast<AudioFormatFlags>(kAudioFormatFlagIsNonInterleaved);
        format.mBytesPerPacket=format.mBytesPerFrame=sizeof(float);format.mFramesPerPacket=1;
        format.mChannelsPerFrame=2;format.mBitsPerChannel=32;
        check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&format,sizeof(format)),"Output format");
        UInt32 maximum=256;
        check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maximum,sizeof(maximum)),"Maximum frames");
        check(AudioUnitInitialize(instance.unit),"AudioUnitInitialize");
        std::cout<<"Installed AU initialized\n"<<std::flush;
        std::array<float,256> left{},right{};
        struct TwoBuffers {UInt32 count;AudioBuffer buffers[2];} storage{2,{{1,sizeof(left),left.data()},{1,sizeof(right),right.data()}}};
        auto* buffers=reinterpret_cast<AudioBufferList*>(&storage);
        AudioTimeStamp timestamp{};timestamp.mFlags=kAudioTimeStampSampleTimeValid;
        const auto render=[&] {
            AudioUnitRenderActionFlags flags=0;
            check(AudioUnitRender(instance.unit,&flags,&timestamp,0,256,buffers),"AudioUnitRender");
            timestamp.mSampleTime+=256;
            double peak=0;
            for(const auto value:left){if(!std::isfinite(value))throw std::runtime_error("AU returned non-finite audio");peak=std::max(peak,std::abs(static_cast<double>(value)));}
            return peak;
        };
        check(MusicDeviceMIDIEvent(instance.unit,0x90,60,96,32),"Host note-on");
        render();
        for(int i=0;i<32;++i)if(std::abs(left[static_cast<size_t>(i)])>0.0000001f)throw std::runtime_error("AU ignored the MIDI sample offset");
        double peak=0;for(int block=0;block<180;++block)peak=std::max(peak,render());
        if(peak<0.001)throw std::runtime_error("Installed AU produced no audible host-MIDI output");
        check(MusicDeviceMIDIEvent(instance.unit,0x80,60,0,0),"Host note-off");
        double tail=0;for(int block=0;block<1500;++block){const auto p=render();if(block>1480)tail=std::max(tail,p);}
        if(tail>0.0001)throw std::runtime_error("Installed AU did not release its note/tail");
        std::cout<<"Installed AU component passed: "<<Nitride::hostParameterCount<<" host parameters, pitch/filter parameter/class-state recall, real MIDI timing, stereo peak "<<peak<<", released tail "<<tail<<'\n';
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
