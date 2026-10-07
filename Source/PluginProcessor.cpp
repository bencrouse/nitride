#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr std::array<const char*, Nitride::parameterCount> stateNames {
    "coupling","stress","response","fm","pitch","attack","decay","sustain","release",
    "cutoff","resonance","motionRate","motionDepth","spaceMix","spaceSize","output"
};
}

NitrideAudioProcessor::NitrideAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)), renderer(session)
{
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
    // Retain basic condition recall through the existing processor hooks. Full preset,
    // gesture automation and session UX validation remain the next milestone.
    const auto patch=session.read();
    juce::XmlElement state("NITRIDE_INSTRUMENT");
    state.setAttribute("version",1);state.setAttribute("program",getCurrentProgram());
    state.setAttribute("mono",patch.mono);state.setAttribute("chord",patch.chord);
    for(size_t i=0;i<stateNames.size();++i)state.setAttribute(stateNames[i],patch.values[i]);
    copyXmlToBinary(state,destination);
}

void NitrideAudioProcessor::setStateInformation(const void* data,int size)
{
    if(auto state=getXmlFromBinary(data,size);state&&state->hasTagName("NITRIDE_INSTRUMENT")&&state->getIntAttribute("version")==1)
    {
        const auto index=juce::jlimit(0,getNumPrograms()-1,state->getIntAttribute("program"));
        auto patch=Nitride::factoryPatches()[static_cast<size_t>(index)];
        for(size_t i=0;i<stateNames.size();++i)patch.values[i]=state->getDoubleAttribute(stateNames[i],patch.values[i]);
        patch.mono=state->getBoolAttribute("mono",patch.mono);patch.chord=state->getBoolAttribute("chord",patch.chord);
        session.program.store(index);session.apply(patch);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NitrideAudioProcessor(); }
