#include <AudioToolbox/AudioToolbox.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../Instrument/Parameters.h"

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
        constexpr std::array<std::pair<const char*,AudioUnitParameterID>,17> legacyIds {{
            {"Mono",3357411},{"Pitch",106677056},{"Tone resonance",157452366},{"FM depth",294406475},
            {"Amplitude release",426656908},{"Space size",504585114},{"Tone cutoff",798419949},{"Amplitude decay",894767487},
            {"Output",1141971201},{"Motion depth",1149422842},{"Space mix",1193923491},{"Stress",1255494068},
            {"Motion rate",1284417481},{"Coupling",1777075325},{"Amplitude sustain",1779138216},{"Response",1807160385},{"Amplitude attack",1896459555}
        }};
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
            if(!dump)for(const auto& legacy:legacyIds)if(std::string(name.data())==legacy.first&&id!=legacy.second)throw std::runtime_error("A legacy AU parameter ID changed");
            if((info.flags&kAudioUnitParameterFlag_CFNameRelease)!=0&&info.cfNameString!=nullptr)CFRelease(info.cfNameString);
        }
        if(dump)return 0;
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
        AudioUnitParameterValue retained=0;check(AudioUnitGetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,&retained),"Host parameter read");
        if(std::abs(retained-.82f)>.0001f)throw std::runtime_error("AU did not retain host automation value");
        CFPropertyListRef savedState=nullptr;UInt32 stateSize=sizeof(savedState);
        check(AudioUnitGetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&savedState,&stateSize),"AU session state");
        check(AudioUnitSetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,.2f,0),"Changed host parameter");
        check(AudioUnitSetProperty(instance.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&savedState,sizeof(savedState)),"AU session restore");
        CFRelease(savedState);
        check(AudioUnitGetParameter(instance.unit,couplingId,kAudioUnitScope_Global,0,&retained),"Restored host parameter");
        if(std::abs(retained-.82f)>.0001f)throw std::runtime_error("Installed AU lost automation conditions during state restore");
        check(AudioUnitGetParameter(instance.unit,depthId,kAudioUnitScope_Global,0,&retained),"Restored Autobend depth");
        if(std::abs(retained-normalizedDepth)>.0001f)throw std::runtime_error("Installed AU lost Autobend depth during state restore: "+std::to_string(retained));
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
        std::cout<<"Installed AU component passed: "<<Nitride::hostParameterCount<<" host parameters, pitch parameter/class-state recall, real MIDI timing, stereo peak "<<peak<<", released tail "<<tail<<'\n';
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
