// Only built in detached historical worktrees. It uses those producers' own
// serializers, never today's encoder with a changed version number.
#include "PluginProcessor.h"
#if NITRIDE_LEGACY_PRESETS
#include "Instrument/PresetStore.h"
#endif
#include <iostream>

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    if(argc!=2)return 1;
    const juce::File output(argv[1]);if(output.createDirectory().failed())return 1;
    const auto temporary=output.getChildFile(".capture-temp");
#if NITRIDE_LEGACY_PRESETS
    NitrideAudioProcessor p(temporary);
#else
    NitrideAudioProcessor p;
#endif
    for(int program:{0,2})
    {
        p.setCurrentProgram(program);auto& s=p.getInstrumentSession();s.set(Nitride::fm,3.25);s.set(Nitride::pitch,5);
#if NITRIDE_LEGACY_PRESETS
        auto preset=s.read();preset.name=program==0?"Historical poly":"Historical mono";
        if(!output.getChildFile(program==0?"preset-v1-poly.nitridepreset":"preset-v1-mono.nitridepreset").replaceWithText(juce::JSON::toString(Nitride::PresetStore::encode({"historical",preset}))))return 1;
        if(s.keep("Historical embedded").failed())return 1;
        s.set(Nitride::coupling,.81);s.setComparing(true);
#endif
        juce::MemoryBlock state;p.getStateInformation(state);
        const auto name=juce::String("session-v")+juce::String(NITRIDE_LEGACY_VERSION)+(program==0?"-poly.state":"-mono.state");
        if(!output.getChildFile(name).replaceWithData(state.getData(),state.getSize()))return 1;
    }
    temporary.deleteRecursively();std::cout<<"Captured historical version "<<NITRIDE_LEGACY_VERSION<<'\n';return 0;
}
