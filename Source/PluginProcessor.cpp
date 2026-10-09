#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Instrument/PresetStore.h"

namespace
{
constexpr std::array<const char*, Nitride::legacyParameterCount> stateNames {
    "coupling","stress","response","fm","pitch","attack","decay","sustain","release",
    "cutoff","resonance","motionRate","motionDepth","spaceMix","spaceSize","output"
};
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
    session.writeHostParameter=[this](int index,double value){auto* p=hostBindings[static_cast<size_t>(index)];p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(value)));};
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
        const juce::ParameterID id(d.id,i<Nitride::legacyParameterCount?1:2);
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
    for(size_t i=0;i<Nitride::hostParameters.size();++i)
        if(id==Nitride::hostParameters[i].id){session.storeFromHost(Nitride::hostIndex(static_cast<Nitride::Parameter>(i)),value);return;}
    if(id==Nitride::monoParameterId)session.storeFromHost(Nitride::monoHostIndex,value);
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
    renderer.process(buffer, midi);
    // Instrument input is consumed, not passed through to another MIDI destination.
    midi.clear();
}

juce::AudioProcessorEditor* NitrideAudioProcessor::createEditor()
{
    return new NitrideAudioProcessorEditor(*this);
}

int NitrideAudioProcessor::getNumPrograms() { return static_cast<int>(Nitride::factoryPatches().size()); }
const juce::String NitrideAudioProcessor::getProgramName(int index)
{
    if(index<0||index>=getNumPrograms())return{};
    return Nitride::factoryPatches()[static_cast<size_t>(index)].name;
}
void NitrideAudioProcessor::applyPreset(int index) { session.applyProgram(index); }

void NitrideAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    juce::XmlElement state("NITRIDE_INSTRUMENT");
    state.setAttribute("version",3);
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
    if(size<=0||size>4*1024*1024)return;
    if(auto state=getXmlFromBinary(data,size);state&&state->hasTagName("NITRIDE_INSTRUMENT"))
    {
        const auto version=state->getIntAttribute("version");
        if(version==2||version==3)
        {
            const auto* document=state->getChildByName("DOCUMENT");if(document==nullptr)return;
            const auto json=juce::JSON::parse(document->getAllSubText());
            if(session.restoreDocument(json).failed())return;
        }
        else if(version==1)
        {
            const auto index=juce::jlimit(0,getNumPrograms()-1,state->getIntAttribute("program"));
            auto patch=Nitride::factoryPatches()[static_cast<size_t>(index)];
            for(size_t i=0;i<stateNames.size();++i)patch.values[i]=Nitride::InstrumentSession::clamp(static_cast<Nitride::Parameter>(i),state->getDoubleAttribute(stateNames[i],patch.values[i]));
            patch.mono=state->getBoolAttribute("mono",patch.mono);patch.chord=state->getBoolAttribute("chord",patch.chord);
            patch.values[Nitride::glideOn]=patch.mono?1:0;
            if(patch.mono){patch.values[Nitride::glideTime]=.025;patch.values[Nitride::glideCurve]=2;}
            session.establishRestoredPatch(patch,index);
        }
        else return;
        apvts.replaceState(parameterStateFor(session.read()));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NitrideAudioProcessor(); }
