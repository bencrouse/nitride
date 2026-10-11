#include "PluginEditor.h"
#include "AllocationAudit.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
[[maybe_unused]] void* volatile escaped=nullptr;
void process(NitrideAudioProcessor& p,juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi)
{
    const auto before=Nitride::Audit::allocations,freed=Nitride::Audit::deallocations;
    {Nitride::Audit::Scope scope;p.processBlock(audio,midi);}
    require(Nitride::Audit::allocations==before,"Render callback allocated memory");
    require(Nitride::Audit::deallocations==freed,"Render callback deallocated memory");
    require(std::isfinite(audio.getMagnitude(0,audio.getNumSamples())),"Non-finite concurrent audio");
}
template<class T>T* named(juce::Component& root,const juce::String& name)
{
    for(auto* c:root.getChildren())
    {
        if(auto* v=dynamic_cast<T*>(c);v&&(c->getName()==name||(dynamic_cast<juce::TextButton*>(c)&&dynamic_cast<juce::TextButton*>(c)->getButtonText()==name)))return v;
        if(auto* v=named<T>(*c,name))return v;
    }
    return nullptr;
}
void lifetime(const juce::File& directory)
{
    for(int channel:{1,15,16})
    {
        NitrideAudioProcessor p(directory);p.applyPreset(2);p.prepareToPlay(48000,64);auto& s=p.getInstrumentSession();
        juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(4096);
        midi.addEvent(juce::MidiMessage::noteOn(channel,60,.8f),0);process(p,audio,midi);
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());auto* hold=named<juce::TextButton>(*editor,"HOLD");require(hold,"Missing Hold");
        hold->setToggleState(true,juce::dontSendNotification);hold->onClick();process(p,audio,midi);
        midi.addEvent(juce::MidiMessage::noteOff(channel,60),0);process(p,audio,midi);
        require(s.live.voices.load()==1,"Host release lost an editor capture");
        editor.reset();for(int i=0;i<1200;++i)process(p,audio,midi);
        require(s.live.voices.load()==0,"Host/editor channel collision left a captured note stuck");
        editor.reset(p.createEditor());s.keyboard.noteOn(15,64,.8f);process(p,audio,midi);editor.reset();
        for(int i=0;i<1200;++i)process(p,audio,midi);require(s.live.voices.load()==0,"Keyboard note survived editor teardown");
        midi.addEvent(juce::MidiMessage::noteOn(channel,67,.8f),0);process(p,audio,midi);
        for(int i=0;i<3000;++i)s.editorMidi.push(0x9f,60,96);
        require(s.editorMidi.overflows.load()>0,"Overflow case did not overflow");
        for(int i=0;i<16;++i)process(p,audio,midi);require(s.live.voices.load()==1,"Editor overflow released a host-owned note");
        midi.addEvent(juce::MidiMessage::noteOff(channel,67),0);process(p,audio,midi);
        for(int i=0;i<1200;++i)process(p,audio,midi);require(s.live.voices.load()==0,"Queue overflow left an editor note stuck");
        s.keyboard.noteOn(16,72,.8f);process(p,audio,midi);p.releaseResources();p.prepareToPlay(48000,64);
        s.keyboard.noteOn(16,76,.8f);process(p,audio,midi);s.keyboard.noteOff(16,76,0);
        for(int i=0;i<1200;++i)process(p,audio,midi);require(s.live.voices.load()==0,"Fresh input after audio reset lost its release");
    }
}
void concurrency(const juce::File& directory)
{
    NitrideAudioProcessor p(directory);p.applyPreset(2);p.prepareToPlay(48000,64);auto& s=p.getInstrumentSession();
    juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.ensureSize(4096);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);
    process(p,audio,midi);juce::MemoryBlock state;p.getStateInformation(state);
    std::atomic<bool> stop=false,ready=false;
    std::exception_ptr failure;
    std::jthread render([&]{ready.store(true);try{while(!stop.load())process(p,audio,midi);}catch(...){failure=std::current_exception();stop.store(true);}});
    while(!ready.load())std::this_thread::yield();
    auto* depth=p.getState().getParameter("autobend_depth");auto* coupling=p.getState().getParameter("coupling");
    for(int i=0;i<100;++i)
    {
        const auto generation=s.restoreEpoch.load();
        s.keyboard.noteOn(15,64,.8f);s.keyboard.noteOff(15,64,0);
        coupling->setValueNotifyingHost(static_cast<float>(i%10)/10);depth->setValueNotifyingHost(depth->convertTo0to1(static_cast<float>(i%25)-12));
        p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        require(!s.setIfCurrent(Nitride::fm,4,generation),"Old gesture overwrote completed recall");
        require(!s.setGroupIfCurrent({{Nitride::coupling,1},{Nitride::stress,1}},generation),"Old grouped gesture overwrote recall");
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor.reset();
    }
    stop.store(true);render.join();if(failure)std::rethrow_exception(failure);require(s.live.faults.load()==0,"Active concurrency caused DSP faults");
    coupling->setValueNotifyingHost(.73f);require(std::abs(s.read().values[Nitride::coupling]-.73)<.00001,"Automation after recall did not win");
    // Framework notification callbacks are measured separately from our renderer.
    const auto before=Nitride::Audit::allocations;
    {Nitride::Audit::Scope scope;depth->setValueNotifyingHost(.3f);}
    std::cout<<"Framework parameter callback allocations after active notifications: "<<Nitride::Audit::allocations-before<<'\n';
    for(int program=0;program<4;++program){p.setCurrentProgram(program);require(p.getCurrentProgram()==program,"Program readback changed");}
    coupling->setValueNotifyingHost(.67f);require(std::abs(s.read().values[Nitride::coupling]-.67)<.00001,"Automation after program selection did not win");
    NitrideAudioProcessor cold(directory);auto* first=cold.getState().getParameter("fm_depth");const auto initial=Nitride::Audit::allocations;
    {Nitride::Audit::Scope scope;first->setValueNotifyingHost(.7f);}
    std::cout<<"Framework FIRST parameter callback allocations: "<<Nitride::Audit::allocations-initial<<" (separate from processBlock)\n";
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
#if NITRIDE_ALLOCATION_AUDIT
        {Nitride::Audit::Scope scope;escaped=std::malloc(37);std::free(escaped);escaped=nullptr;}
        require(Nitride::Audit::allocations>0&&Nitride::Audit::deallocations>0,"Allocation audit did not detect its deliberate violation");
#endif
        const auto directory=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nitride-safety-"+juce::Uuid().toString());
        lifetime(directory);concurrency(directory);
        std::cout<<"Realtime safety passed: cold render allocation guard, host/editor ownership, teardown, overflow recovery, active APVTS/recall concurrency and generation rejection\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
