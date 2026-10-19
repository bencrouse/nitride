#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include <bit>

namespace
{
struct OwnEcho
{
    const NitrideAudioProcessor* processor=nullptr;int index=-1;float value=0;const Nitride::Patch* restored=nullptr;
};
thread_local OwnEcho ownEcho;
struct EchoScope
{
    explicit EchoScope(OwnEcho echo):previous(ownEcho){ownEcho=echo;}
    ~EchoScope(){ownEcho=previous;}
    OwnEcho previous;
};
bool sameFloat(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
}

NitrideAudioProcessor::NitrideAudioProcessor(juce::File presetDirectory)
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      session(std::move(presetDirectory)),apvts(*this,nullptr,"NITRIDE_PARAMETERS",createParameterLayout()),renderer(session)
{
    for(size_t i=0;i<Nitride::parameterCount;++i)
    {
        hostBindings[static_cast<size_t>(Nitride::hostIndex(static_cast<Nitride::Parameter>(i)))]=apvts.getParameter(Nitride::hostParameters[i].id);
        apvts.addParameterListener(Nitride::hostParameters[i].id,this);
    }
    hostBindings[Nitride::monoHostIndex]=apvts.getParameter(Nitride::monoParameterId);
    apvts.addParameterListener(Nitride::monoParameterId,this);
    session.writeHostParameter=[this](int index,double value)
    {
        auto* p=hostBindings[static_cast<size_t>(index)];const auto normalized=p->convertTo0to1(static_cast<float>(value));
        EchoScope echo({this,index,p->convertFrom0to1(normalized),nullptr});p->setValueNotifyingHost(normalized);
    };
    session.beginHostGesture=[this](int index){hostBindings[static_cast<size_t>(index)]->beginChangeGesture();};
    session.endHostGesture=[this](int index){hostBindings[static_cast<size_t>(index)]->endChangeGesture();};
}

NitrideAudioProcessor::~NitrideAudioProcessor()
{
    session.endAllEdits();session.writeHostParameter={};session.beginHostGesture={};session.endHostGesture={};
    for(const auto& p:Nitride::hostParameters)apvts.removeParameterListener(p.id,this);
    apvts.removeParameterListener(Nitride::monoParameterId,this);
}

juce::AudioProcessorValueTreeState::ParameterLayout NitrideAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto add=[&](size_t i)
    {
        const auto& d=Nitride::hostParameters[i];
        const juce::ParameterID id(d.id,Nitride::versionHint(static_cast<Nitride::Parameter>(i)));
        if(d.kind==Nitride::ParameterKind::boolean)layout.add(std::make_unique<juce::AudioParameterBool>(id,d.name,d.initial>=.5));
        else if(d.kind==Nitride::ParameterKind::choice)layout.add(std::make_unique<juce::AudioParameterChoice>(id,d.name,juce::StringArray::fromTokens(d.choices,"|",""),static_cast<int>(d.initial)));
        else layout.add(std::make_unique<juce::AudioParameterFloat>(id,d.name,
            juce::NormalisableRange<float>(static_cast<float>(d.minimum),static_cast<float>(d.maximum),0,static_cast<float>(d.skew)),static_cast<float>(d.initial)));
    };
    for(size_t i=0;i<Nitride::legacyParameterCount;++i)add(i);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(Nitride::monoParameterId,1),"Mono",false));
    for(size_t i=Nitride::legacyParameterCount;i<Nitride::parameterCount;++i)add(i);
    return layout;
}

void NitrideAudioProcessor::parameterChanged(const juce::String& id,float value)
{
    const auto receive=[this,value](int index)
    {
        if(ownEcho.processor==this)
        {
            if(ownEcho.index==index&&sameFloat(value,ownEcho.value))return;
            if(ownEcho.restored)
            {
                const auto physical=index==Nitride::monoHostIndex?(ownEcho.restored->mono?1.0:0.0):ownEcho.restored->values[static_cast<size_t>(Nitride::conditionIndex(index))];
                const auto* p=hostBindings[static_cast<size_t>(index)];const auto expected=p->convertFrom0to1(p->convertTo0to1(static_cast<float>(physical)));
                if(sameFloat(value,expected))return;
            }
        }
        // Only the known synchronous echo is ignored. Other values, processors
        // and threads still publish automation normally, including reentrant writes.
        session.storeFromHost(index,value);
    };
    for(size_t i=0;i<Nitride::hostParameters.size();++i)
        if(id==Nitride::hostParameters[i].id){receive(Nitride::hostIndex(static_cast<Nitride::Parameter>(i)));return;}
    if(id==Nitride::monoParameterId)receive(Nitride::monoHostIndex);
}

void NitrideAudioProcessor::prepareToPlay(double sampleRate, int)
{
    renderer.prepare(sampleRate);
}

void NitrideAudioProcessor::releaseResources()
{
    renderer.reset();
}

bool NitrideAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

const juce::String NitrideAudioProcessor::getName() const { return JucePlugin_Name; }

void NitrideAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    Nitride::DSP::Transport transport;
    if(auto* playhead=getPlayHead())if(const auto position=playhead->getPosition())
    {
        if(const auto bpm=position->getBpm()){transport.bpm=*bpm;transport.hasTempo=true;}
        if(const auto ppq=position->getPpqPosition()){transport.ppq=*ppq;transport.hasPosition=true;}
        if(const auto bar=position->getPpqPositionOfLastBarStart()){transport.barStart=*bar;transport.hasBarStart=true;}
        if(const auto count=position->getBarCount()){transport.barCount=*count;transport.hasBarCount=true;}
        transport.playing=position->getIsPlaying();
        if(const auto signature=position->getTimeSignature();signature&&signature->numerator>0&&signature->denominator>0)
            transport.beatsPerBar=signature->numerator*4.0/signature->denominator;
    }
    renderer.process(buffer, midi,true,transport);
    // Instrument input is consumed, not passed through to another MIDI destination.
    midi.clear();
}

juce::AudioProcessorEditor* NitrideAudioProcessor::createEditor()
{
    return new NitrideAudioProcessorEditor(*this);
}

int NitrideAudioProcessor::getNumPrograms() { return static_cast<int>(Nitride::builtInPresets().size()); }
const juce::String NitrideAudioProcessor::getProgramName(int index)
{
    if(index<0||index>=getNumPrograms())return{};
    return Nitride::builtInPresets()[static_cast<size_t>(index)].patch.name;
}
void NitrideAudioProcessor::applyPreset(int index) { session.applyProgram(index); }

void NitrideAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    const juce::ScopedLock transaction(session.stateTransactionLock());
    juce::XmlElement state("NITRIDE_INSTRUMENT");
    state.setAttribute("version",7);
    const auto document=session.saveDocument();
    state.createNewChildElement("DOCUMENT")->addTextElement(juce::JSON::toString(document,true));
    // The document's edited conditions are the authority even while Compare is
    // auditioning a reference, or automation is changing the live parameter bank.
    const auto preset=Nitride::PresetStore::decode(document["current"]["patch"]);
    if(preset)if(auto xml=parameterStateFor(preset->patch).createXml())state.addChildElement(xml.release());
    copyXmlToBinary(state,destination);
}

juce::ValueTree NitrideAudioProcessor::parameterStateFor(const Nitride::Patch& patch)
{
    auto tree=apvts.copyState();
    for(auto child:tree)
    {
        const auto id=child.getProperty("id").toString();
        for(size_t i=0;i<Nitride::parameterCount;++i)if(id==Nitride::hostParameters[i].id)child.setProperty("value",static_cast<float>(patch.values[i]),nullptr);
        if(id==Nitride::monoParameterId)child.setProperty("value",patch.mono?1.0f:0.0f,nullptr);
    }
    return tree;
}

void NitrideAudioProcessor::setStateInformation(const void* data,int size)
{
    const juce::ScopedLock transaction(session.stateTransactionLock());
    if(size<=0||size>4*1024*1024)return;
    if(auto state=getXmlFromBinary(data,size);state&&state->hasTagName("NITRIDE_INSTRUMENT"))
    {
        const auto version=state->getIntAttribute("version");
        if(version==7)
        {
            const auto* document=state->getChildByName("DOCUMENT");if(document==nullptr)return;
            const auto json=juce::JSON::parse(document->getAllSubText());
            if(session.restoreDocument(json).failed())return;
        }
        else return;
        const auto restored=session.read();EchoScope echo({this,-1,0,&restored});apvts.replaceState(parameterStateFor(restored));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NitrideAudioProcessor(); }
