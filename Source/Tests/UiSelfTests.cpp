#include "PluginEditor.h"

// Freeze the pre-integration review algorithm as an independent reference.
// It is compiled without an application entry point or an audio device.
#define NITRIDE_INSTRUMENT_REVIEW 1
#define NITRIDE_REFERENCE_CAPTURE 1
#include "../Studies/GestureApplication.cpp"

#include <stdexcept>

namespace
{
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}

template<typename Type>
Type* namedChild(juce::Component& parent,const juce::String& name)
{
    for(auto* child:parent.getChildren())
    {
        if(auto* result=dynamic_cast<Type*>(child);result!=nullptr&&child->getName()==name)return result;
        if(auto* result=namedChild<Type>(*child,name))return result;
    }
    return nullptr;
}

juce::TextButton* button(juce::Component& parent,const juce::String& label)
{
    for(auto* child:parent.getChildren())
    {
        if(auto* result=dynamic_cast<juce::TextButton*>(child);result!=nullptr&&result->getButtonText()==label)return result;
        if(auto* result=button(*child,label))return result;
    }
    return nullptr;
}

double renderFor(NitrideAudioProcessor& processor,int samples,int blockSize=257)
{
    double peak=0;
    while(samples>0)
    {
        const auto size=std::min(samples,blockSize);
        juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;
        processor.processBlock(audio,midi);peak=std::max(peak,static_cast<double>(audio.getMagnitude(0,size)));samples-=size;
    }
    return peak;
}

void referenceMatches()
{
    constexpr int block=257,samples=48000;
    for(int program=0;program<4;++program)
    {
        InstrumentReview legacy;
        const auto expected=legacy.renderReference(program,48000,samples,block);
        NitrideAudioProcessor processor;processor.applyPreset(program);processor.prepareToPlay(48000,block);
        double difference=0;
        for(int offset=0;offset<samples;offset+=block)
        {
            const auto size=std::min(block,samples-offset);juce::AudioBuffer<float> audio(2,size);juce::MidiBuffer midi;
            if(offset==0)
            {
                const auto& patch=Nitride::factoryPatches()[static_cast<size_t>(program)];
                if(patch.chord&&!patch.mono)for(const auto note:{48,55,59,62})midi.addEvent(juce::MidiMessage::noteOn(1,note,.74f),0);
                else midi.addEvent(juce::MidiMessage::noteOn(1,60,.74f),0);
            }
            processor.processBlock(audio,midi);
            for(int c=0;c<2;++c)for(int i=0;i<size;++i)
            {
                const auto value=audio.getSample(c,i);require(std::isfinite(value),"Non-finite hosted audio");
                difference=std::max(difference,std::abs(static_cast<double>(value-expected[static_cast<size_t>(c)][static_cast<size_t>(offset+i)])));
            }
        }
        if(difference>=0.00001)std::cerr<<"Reference difference, program "<<program<<": "<<difference<<'\n';
        require(difference<0.00001,"Hosted rendering changed the approved review sound");
    }
}
}

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        referenceMatches();
        NitrideAudioProcessor processor;processor.applyPreset(2);processor.prepareToPlay(48000,256);
        juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),64);processor.processBlock(audio,midi);
        require(audio.getMagnitude(0,64)<0.0000001f,"Host MIDI note began before its sample offset");
        require(audio.getMagnitude(64,192)>0.00001f,"Host MIDI did not produce audio");
        require(midi.isEmpty(),"Instrument unexpectedly passed MIDI through");

        midi.addEvent(juce::MidiMessage::controllerEvent(1,64,127),0);
        midi.addEvent(juce::MidiMessage::noteOff(1,60),1);processor.processBlock(audio,midi);renderFor(processor,48000);
        require(processor.getInstrumentSession().live.voices.load()==1,"Sustain pedal failed to hold a host note");
        midi.addEvent(juce::MidiMessage::controllerEvent(1,64,0),0);processor.processBlock(audio,midi);renderFor(processor,48000);
        require(processor.getInstrumentSession().live.voices.load()==0,"Sustain pedal left a stuck note");

        processor.applyPreset(1);processor.prepareToPlay(48000,256);
        midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);processor.processBlock(audio,midi);renderFor(processor,24000);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        editor->addToDesktop(0);editor->setVisible(true);
        require(editor->getWidth()==1200&&editor->getHeight()==840,"Approved editor size changed");
        auto* depth=namedChild<juce::Component>(*editor,"FM depth");require(depth!=nullptr,"Source control missing");
        auto* accessibility=depth->getAccessibilityHandler();require(accessibility!=nullptr&&accessibility->getValueInterface()!=nullptr,"Source accessibility control unavailable");
        accessibility->getValueInterface()->setValue(3.2);
        require(std::abs(processor.getInstrumentSession().values[Nitride::fm].load()-3.2)<.001,"Editor source control is disconnected from processor");
        auto* hold=button(*editor,"HOLD");require(hold!=nullptr,"Hold control missing");
        hold->setToggleState(true,juce::dontSendNotification);hold->onClick();renderFor(processor,1024);
        require(processor.getInstrumentSession().live.voices.load()==1,"Hold capture doubled a host note");
        editor.reset();renderFor(processor,48000);
        require(processor.getInstrumentSession().live.voices.load()==1,"Closing the editor released a held host note");
        midi.addEvent(juce::MidiMessage::noteOff(1,60),0);processor.processBlock(audio,midi);renderFor(processor,48000);
        require(processor.getInstrumentSession().live.voices.load()==0,"Editor close left a stuck audition note");

        std::unique_ptr<juce::AudioProcessorEditor> touchEditor(processor.createEditor());
        auto* field=namedChild<juce::Component>(*touchEditor,"Coupling and Stress playing field");require(field!=nullptr,"Approved field missing");
        field->keyPressed(juce::KeyPress(juce::KeyPress::spaceKey));renderFor(processor,4096);
        require(processor.getInstrumentSession().live.voices.load()>0,"Field audition is disconnected");
        touchEditor.reset();renderFor(processor,48000);
        require(processor.getInstrumentSession().live.voices.load()==0,"Field audition survived closing its editor");

        juce::MemoryBlock state;processor.getStateInformation(state);
        NitrideAudioProcessor restored;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        require(std::abs(restored.getInstrumentSession().values[Nitride::fm].load()-3.2)<.001,"Basic condition recall lost the edited source");
        require(restored.getCurrentProgram()==1,"Basic condition recall lost the current program");

        if(argc==3&&juce::String(argv[1])=="--snapshot")
        {
            std::unique_ptr<juce::AudioProcessorEditor> preview(processor.createEditor());
            const auto image=preview->createComponentSnapshot(preview->getLocalBounds(),true,2);
            auto stream=juce::File(juce::String::fromUTF8(argv[2])).createOutputStream();
            require(stream!=nullptr&&stream->setPosition(0)&&stream->truncate().wasOk(),"Could not write AU editor snapshot");
            require(juce::PNGImageFormat().writeImageToStream(image,*stream),"AU editor snapshot failed");
        }
        std::cout<<"Hosted instrument checks passed: review sound equivalence, MIDI timing, sustain, connected controls, editor ownership/release, basic conditions.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
