// Offline native AU preset export/recall probe. Does not link or modify Nitride.
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>
#include "FactoryPrograms.h"

namespace
{
void require(bool condition,const char* message)
{
    if(!condition)throw std::runtime_error(message);
}
void check(OSStatus status,const char* operation)
{
    if(status!=noErr)throw std::runtime_error(std::string(operation)+": "+std::to_string(status));
}
template<class T> struct Owned
{
    explicit Owned(T value):value(value){}
    ~Owned(){if(value)CFRelease(value);}
    Owned(const Owned&)=delete;
    Owned& operator=(const Owned&)=delete;
    T value;
};
struct Instance
{
    Instance()
    {
        AudioComponentDescription description{};
        description.componentType=kAudioUnitType_MusicDevice;
        description.componentSubType='NT01';description.componentManufacturer='NTRD';
        const auto component=AudioComponentFindNext(nullptr,&description);
        require(component!=nullptr,"Installed Nitride AU not found");
        check(AudioComponentInstanceNew(component,&unit),"Create isolated AU instance");
    }
    ~Instance(){if(unit){if(initialized)AudioUnitUninitialize(unit);AudioComponentInstanceDispose(unit);}}
    Instance(const Instance&)=delete;
    Instance& operator=(const Instance&)=delete;
    void select(int program,const std::string& expected)
    {
        require(program>=0&&program<static_cast<int>(Nitride::legacyProgramNames.size()),"Invalid historical program");
        require(expected==Nitride::legacyProgramNames[static_cast<size_t>(program)],"Factory name differs from the immutable source registry");
        const Owned<CFStringRef> name(CFStringCreateWithCString(nullptr,expected.c_str(),kCFStringEncodingUTF8));
        const AUPreset requested{program,name.value};
        // Hidden legacy numbers remain valid. Public factory-list enumeration is unsupported.
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_PresentPreset,kAudioUnitScope_Global,0,&requested,sizeof(requested)),"Select source historical program");
    }
    void prepare()
    {
        AudioStreamBasicDescription format{};
        format.mSampleRate=48000;format.mFormatID=kAudioFormatLinearPCM;
        format.mFormatFlags=static_cast<AudioFormatFlags>(kAudioFormatFlagsNativeFloatPacked)|static_cast<AudioFormatFlags>(kAudioFormatFlagIsNonInterleaved);
        format.mBytesPerPacket=format.mBytesPerFrame=sizeof(float);format.mFramesPerPacket=1;
        format.mChannelsPerFrame=2;format.mBitsPerChannel=32;
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&format,sizeof(format)),"Set output format");
        UInt32 maximum=64;
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maximum,sizeof(maximum)),"Set block size");
        check(AudioUnitInitialize(unit),"Initialize AU");initialized=true;
    }
    AudioUnit unit=nullptr;
    bool initialized=false;
};
CFPropertyListRef state(AudioUnit unit)
{
    CFPropertyListRef value=nullptr;UInt32 size=sizeof(value);
    check(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&value,&size),"Read native AU ClassInfo");
    require(value&&CFGetTypeID(value)==CFDictionaryGetTypeID(),"ClassInfo is not a dictionary");
    return value;
}
CFPropertyListRef read(const std::string& path)
{
    std::ifstream stream(path,std::ios::binary);
    require(stream.good(),"Cannot open .aupreset");
    std::vector<UInt8> bytes{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
    require(!bytes.empty()&&bytes.size()<=4*1024*1024,"Invalid .aupreset size");
    const Owned<CFDataRef> data(CFDataCreate(nullptr,bytes.data(),static_cast<CFIndex>(bytes.size())));
    auto result=CFPropertyListCreateWithData(nullptr,data.value,kCFPropertyListImmutable,nullptr,nullptr);
    require(result&&CFGetTypeID(result)==CFDictionaryGetTypeID(),"Invalid .aupreset property list");
    return result;
}
void writeNew(const std::string& path,CFPropertyListRef value)
{
    const Owned<CFDataRef> data(CFPropertyListCreateData(nullptr,value,kCFPropertyListXMLFormat_v1_0,0,nullptr));
    require(data.value!=nullptr,"Cannot serialize native AU ClassInfo");
    const auto file=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0644);
    require(file>=0,"Cannot create NEW preset; existing files are never replaced");
    const auto* bytes=CFDataGetBytePtr(data.value);const auto length=CFDataGetLength(data.value);
    CFIndex written=0;
    while(written<length)
    {
        const auto count=::write(file,bytes+written,static_cast<size_t>(length-written));
        if(count<=0){::close(file);::unlink(path.c_str());throw std::runtime_error("Preset write failed");}
        written+=count;
    }
    const auto synced=::fsync(file);const auto closed=::close(file);
    require(synced==0&&closed==0,"Cannot flush new preset");
}
std::vector<AudioUnitParameterID> parameterIds(AudioUnit unit)
{
    UInt32 size=0;Boolean writable=false;
    check(AudioUnitGetPropertyInfo(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,&size,&writable),"Parameter list size");
    std::vector<AudioUnitParameterID> ids(size/sizeof(AudioUnitParameterID));
    check(AudioUnitGetProperty(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,ids.data(),&size),"Parameter list");
    return ids;
}
void verify(const std::string& path,int program,const std::string& expected)
{
    Instance baseline,recalled;baseline.select(program,expected);
    const Owned<CFPropertyListRef> source(state(baseline.unit)),preset(read(path));
    for(const auto* key:{"type","subtype","manufacturer","version"})
    {
        const Owned<CFStringRef> name(CFStringCreateWithCString(nullptr,key,kCFStringEncodingUTF8));
        const auto a=CFDictionaryGetValue(static_cast<CFDictionaryRef>(source.value),name.value);
        const auto b=CFDictionaryGetValue(static_cast<CFDictionaryRef>(preset.value),name.value);
        require(a&&b&&CFEqual(a,b),"Preset component identity differs from installed AU");
    }
    // Restore over a different factory program, rather than coincidentally matching defaults.
    if(program==1)recalled.select(0,"01 / Soft pad");
    else recalled.select(1,"02 / Glass keys");
    check(AudioUnitSetProperty(recalled.unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&preset.value,sizeof(preset.value)),"Restore .aupreset through native AU API");
    const Owned<CFPropertyListRef> after(state(recalled.unit));
    const auto originalBytes=CFDictionaryGetValue(static_cast<CFDictionaryRef>(source.value),CFSTR("jucePluginState"));
    const auto fileBytes=CFDictionaryGetValue(static_cast<CFDictionaryRef>(preset.value),CFSTR("jucePluginState"));
    const auto recalledBytes=CFDictionaryGetValue(static_cast<CFDictionaryRef>(after.value),CFSTR("jucePluginState"));
    require(originalBytes&&fileBytes&&recalledBytes&&CFEqual(originalBytes,fileBytes)&&CFEqual(originalBytes,recalledBytes),"Preset did not preserve the exact full Nitride document");
    const auto ids=parameterIds(baseline.unit);
    require(ids==parameterIds(recalled.unit)&&ids.size()==89,"AU automation contract changed");
    for(const auto id:ids)
    {
        AudioUnitParameterValue a=0,b=0;
        check(AudioUnitGetParameter(baseline.unit,id,kAudioUnitScope_Global,0,&a),"Read factory parameter");
        check(AudioUnitGetParameter(recalled.unit,id,kAudioUnitScope_Global,0,&b),"Read recalled parameter");
        require(a==b,"Recalled parameter differs from factory baseline");
    }
    baseline.prepare();recalled.prepare();
    double peak=0;
    for(int frame=0;frame<48000*4;frame+=64)
    {
        for(const auto unit:{baseline.unit,recalled.unit})
        {
            if(frame==0)for(const auto note:{60u,64u,67u})check(MusicDeviceMIDIEvent(unit,0x90,note,96,0),"Send test note-on");
            if(frame==48000*2)for(const auto note:{60u,64u,67u})check(MusicDeviceMIDIEvent(unit,0x80,note,0,0),"Send test note-off");
        }
        std::array<std::array<float,64>,2> original{},restored{};
        const auto render=[&](AudioUnit unit,auto& audio)
        {
            struct Stereo {UInt32 count;AudioBuffer buffers[2];} storage{2,{{1,64*sizeof(float),audio[0].data()},{1,64*sizeof(float),audio[1].data()}}};
            AudioTimeStamp time{};time.mFlags=kAudioTimeStampSampleTimeValid;time.mSampleTime=frame;
            AudioUnitRenderActionFlags flags=0;
            check(AudioUnitRender(unit,&flags,&time,0,64,reinterpret_cast<AudioBufferList*>(&storage)),"Render serialization probe");
        };
        render(baseline.unit,original);render(recalled.unit,restored);
        for(size_t channel=0;channel<2;++channel)for(size_t i=0;i<64;++i)
        {
            const auto a=original[channel][i],b=restored[channel][i];
            require(std::isfinite(a)&&std::isfinite(b)&&a==b,"Native preset audio differs from original factory conditions");
            peak=std::max(peak,std::abs(static_cast<double>(a)));
        }
    }
    require(peak>.000001,"Serialization probe produced no audible signal");
    std::cout<<"{\"full_document_exact\":true,\"parameters\":"<<ids.size()<<",\"audio_exact\":true,\"sample_rate\":48000,\"block_frames\":64,\"frames\":192000,\"peak\":"<<peak<<"}\n";
}
}

int main(int argc,char** argv)
{
    try
    {
        if(argc==6&&std::string(argv[1])=="export")
        {
            Instance instance;instance.select(std::stoi(argv[2]),argv[3]);
            const Owned<CFPropertyListRef> original(state(instance.unit));
            const Owned<CFMutableDictionaryRef> preset(CFDictionaryCreateMutableCopy(nullptr,0,static_cast<CFDictionaryRef>(original.value)));
            const Owned<CFStringRef> name(CFStringCreateWithCString(nullptr,argv[4],kCFStringEncodingUTF8));
            require(name.value&&CFStringGetLength(name.value)>0,"Empty preset label");
            CFDictionarySetValue(preset.value,CFSTR("name"),name.value);
            writeNew(argv[5],preset.value);
            std::cout<<"Exported clean isolated AU ClassInfo; only outer host label changed\n";
        }
        else if(argc==5&&std::string(argv[1])=="verify")verify(argv[2],std::stoi(argv[3]),argv[4]);
        else throw std::runtime_error("Usage: nitride_au_presets export PROGRAM EXPECTED-FACTORY-NAME HOST-LABEL NEW.aupreset | verify FILE PROGRAM EXPECTED-FACTORY-NAME");
        return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
