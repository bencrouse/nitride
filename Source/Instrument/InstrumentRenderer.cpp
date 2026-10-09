#include "InstrumentRenderer.h"

namespace Nitride
{
void InstrumentRenderer::prepare(double sampleRate)
{
    rate = sampleRate;
    held = {}; deferred = {}; sustainPedal = {};
    groupHeld={};keyCounts={};presses={};velocities={};nextPress=0;physicalKeys=groupKeys=0;monoNote=-1;monoMode=session.audioMono();
    wheel = lfoPhase = 0;
    editorMidi.ensureSize(8192);
    engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);
    updateEngine();
    engine.prepare(sampleRate);
    reverb.setSampleRate(sampleRate); reverb.reset(); tone = {};
    cutoffSmooth.reset(sampleRate, .025); resonanceSmooth.reset(sampleRate, .025); gainSmooth.reset(sampleRate, .025);
    cutoffSmooth.setCurrentAndTargetValue(static_cast<float>(session.audioValue(cutoff)));
    resonanceSmooth.setCurrentAndTargetValue(static_cast<float>(session.audioValue(resonance)));
    gainSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(session.audioValue(output))));
    session.keyboard.allNotesOff(1); session.keyboard.allNotesOff(16);
    publish();
}

void InstrumentRenderer::updateEngine()
{
    engine.setFmIndex(session.audioValue(fm));
    engine.setAmplitudeEnvelope(session.audioValue(attack), session.audioValue(decay),
        static_cast<float>(session.audioValue(sustain)), session.audioValue(release));
    engine.setNetworkDimensions(static_cast<float>(session.audioValue(coupling)),
        static_cast<float>(session.audioValue(stress)), session.audioValue(response));
    engine.setPitchBend(static_cast<float>(session.audioValue(pitch) + wheel));
    SoundStudies::PitchSettings settings;
    settings.glide={session.audioValue(glideOn)>=.5,session.audioValue(glideTime),static_cast<SoundStudies::PitchTarget>(static_cast<int>(session.audioValue(glideTarget))),
        session.audioValue(glideLegato)>=.5,static_cast<SoundStudies::GlideCurve>(static_cast<int>(session.audioValue(glideCurve)))};
    settings.autobend={session.audioValue(autobendOn)>=.5,session.audioValue(autobendTime),session.audioValue(autobendDepth),
        static_cast<SoundStudies::PitchTarget>(static_cast<int>(session.audioValue(autobendTarget))),session.audioValue(autobendPhrase)>=.5};
    settings.legato=session.audioValue(legato)>=.5;engine.setPitchMotion(settings);
}

void InstrumentRenderer::beginMidiGroup()
{
    groupHeld=held;groupKeys=physicalKeys;engine.beginNoteGroup();
}
void InstrumentRenderer::refreshMonoNote()
{
    int selected=-1;
    // Physical keys take priority over pedal-held notes. Within each class the most
    // recent owner wins; capture owners retain the captured press's original order.
    for(int pass=0;pass<2&&selected<0;++pass)
        for(size_t i=0;i<held.size();++i)if((pass==0?held[i]:deferred[i])&&(selected<0||presses[i]>presses[static_cast<size_t>(selected)]))selected=static_cast<int>(i);
    if(selected<0){if(monoNote>=0)engine.noteOff(monoNote);monoNote=-1;return;}
    const auto note=selected%128;if(note==monoNote)return;
    engine.noteOn(note,velocities[static_cast<size_t>(selected)],SoundStudies::Articulation::lead,true,monoNote>=0);monoNote=note;
}

bool InstrumentRenderer::heldAnywhere(int note) const noexcept
{
    for (size_t channel = 0; channel < 32; ++channel)
        if (held[channel * 128 + static_cast<size_t>(note)] || deferred[channel * 128 + static_cast<size_t>(note)]) return true;
    return false;
}

void InstrumentRenderer::releaseChannel(int channel)
{
    for (int note = 0; note < 128; ++note)
    {
        const auto index = static_cast<size_t>(channel * 128 + note);
        if(held[index]){--physicalKeys;if(groupHeld[index]){groupHeld[index]=false;--groupKeys;}}
        keyCounts[index]=0;
        held[index] = deferred[index] = false;
        if (!monoMode&&!heldAnywhere(note)) engine.noteOff(note);
    }
    sustainPedal[static_cast<size_t>(channel)] = false;
    if(monoMode)refreshMonoNote();
}

void InstrumentRenderer::handleMidi(const juce::MidiMessage& message, bool editor)
{
    const auto midiChannel = message.getChannel() - 1;
    const auto channel = midiChannel + (editor ? 16 : 0);
    if (message.isNoteOn() || message.isNoteOff())
    {
        const auto note = message.getNoteNumber();
        const auto index = static_cast<size_t>(channel * 128 + note);
        if (message.isNoteOn())
        {
            const auto overlap=groupKeys-(groupHeld[index]?1:0)>0;
            if(!held[index])++physicalKeys;
            held[index]=true;if(keyCounts[index]<65535)++keyCounts[index];
            deferred[index] = false;
            bool capture = false;
            if (editor && midiChannel == 15)
                for (size_t c = 0; c < 31; ++c) capture |= held[c * 128 + static_cast<size_t>(note)] || deferred[c * 128 + static_cast<size_t>(note)];
            if(capture)
            {
                for(size_t c=0;c<31;++c)presses[index]=std::max(presses[index],presses[c*128+static_cast<size_t>(note)]);
            }
            else presses[index]=++nextPress;
            velocities[index]=message.getFloatVelocity();
            if (!capture)
            {
                engine.noteOn(note,message.getFloatVelocity(),monoMode?SoundStudies::Articulation::lead:SoundStudies::Articulation::pad,overlap);
                if(monoMode)monoNote=note;
            }
        }
        else
        {
            if(!held[index]&&keyCounts[index]==0)return;
            if(keyCounts[index]>0)--keyCounts[index];
            if(keyCounts[index]>0)return;
            if(held[index]){--physicalKeys;if(groupHeld[index]){groupHeld[index]=false;--groupKeys;}}
            held[index]=false;
            deferred[index] = sustainPedal[static_cast<size_t>(channel)];
            if(monoMode)refreshMonoNote();else if (!heldAnywhere(note)) engine.noteOff(note);
        }
    }
    else if (message.isPitchWheel())
    {
        wheel = static_cast<double>(message.getPitchWheelValue() - 8192) * 2 / 8192;
        engine.setPitchBend(static_cast<float>(session.audioValue(pitch) + wheel));
    }
    else if (message.isController() && message.getControllerNumber() == 64)
    {
        sustainPedal[static_cast<size_t>(channel)] = message.getControllerValue() >= 64;
        if (!sustainPedal[static_cast<size_t>(channel)])
            for (int note = 0; note < 128; ++note)
            {
                deferred[static_cast<size_t>(channel * 128 + note)] = false;
                if (!monoMode&&!heldAnywhere(note)) engine.noteOff(note);
            }
        if(monoMode)refreshMonoNote();
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff()) releaseChannel(channel);
}

void InstrumentRenderer::renderRange(float* left, float* right, int samples)
{
    int offset = 0;
    while (offset < samples)
    {
        const auto size = std::min(32, samples - offset);
        const auto modulation = std::sin(lfoPhase) * session.audioValue(motionDepth) * .18;
        engine.setNetworkDimensions(static_cast<float>(juce::jlimit(0.0, 1.0, session.audioValue(coupling) + modulation)),
            static_cast<float>(session.audioValue(stress)), session.audioValue(response));
        std::array<double,32> toneMotion{};
        engine.render(left+offset,right+offset,size,toneMotion.data());
        for(int i=0;i<size;++i)
        {
            const auto base=static_cast<double>(cutoffSmooth.getNextValue());
            const auto shift=toneMotion[static_cast<size_t>(i)];
            const auto frequency=std::min(SoundStudies::pitchIsZero(shift)?base:std::max(10.0,base*std::exp2(shift/12)),rate*.42);
            const auto g=std::tan(juce::MathConstants<double>::pi*frequency/rate);
            const auto k=2.0-1.85*resonanceSmooth.getNextValue();const auto a=1/(1+g*(g+k));
            const auto filterSample=[&](double input,int channel) {
                auto& t=tone[static_cast<size_t>(channel)];const auto v1=a*(t[0]+g*(input-t[1]));const auto v2=t[1]+g*v1;
                t[0]=2*v1-t[0];t[1]=2*v2-t[1];return static_cast<float>(v2);
            };
            left[offset+i]=filterSample(left[offset+i],0);if(right!=left)right[offset+i]=filterSample(right[offset+i],1);
        }
        lfoPhase += juce::MathConstants<double>::twoPi * session.audioValue(motionRate) * size / rate;
        lfoPhase = std::fmod(lfoPhase, juce::MathConstants<double>::twoPi);
        offset += size;
    }
}

void InstrumentRenderer::process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi, bool injectEditorNotes)
{
    const juce::ScopedNoDenormals noDenormals;
    audio.clear();
    if (audio.getNumChannels() == 0 || audio.getNumSamples() == 0) return;
    updateEngine();
    if(monoMode!=session.audioMono())
    {
        monoMode=session.audioMono();engine.allNotesOff();monoNote=-1;
        if(monoMode)refreshMonoNote();
        else
        {
            std::array<bool,128> started{};
            for(size_t i=0;i<held.size();++i)if((held[i]||deferred[i])&&!started[i%128]){engine.noteOn(static_cast<int>(i%128),velocities[i]);started[i%128]=true;}
        }
    }
    cutoffSmooth.setTargetValue(static_cast<float>(session.audioValue(cutoff)));
    resonanceSmooth.setTargetValue(static_cast<float>(session.audioValue(resonance)));
    editorMidi.clear();
    if (injectEditorNotes) session.keyboard.processNextMidiBuffer(editorMidi, 0, audio.getNumSamples(), true);
    auto* left = audio.getWritePointer(0);
    auto* right = audio.getWritePointer(audio.getNumChannels() > 1 ? 1 : 0);
    int offset = 0;
    auto host = midi.begin();
    auto editor = editorMidi.begin();
    int groupPosition=-1;
    while (host != midi.end() || editor != editorMidi.end())
    {
        const auto fromEditor = editor != editorMidi.end()
            && (host == midi.end() || (*editor).samplePosition < (*host).samplePosition);
        const auto metadata = fromEditor ? *editor++ : *host++;
        const auto position = juce::jlimit(offset, audio.getNumSamples(), metadata.samplePosition);
        renderRange(left + offset, right + offset, position - offset);
        if(position!=groupPosition){beginMidiGroup();groupPosition=position;}
        // Large SysEx is not part of this instrument's input protocol. Avoid materializing
        // an allocating MidiMessage on the audio thread for those messages.
        if (metadata.numBytes <= 3)
        {
            const auto message = metadata.getMessage();
            if (!fromEditor) session.keyboard.processNextMidiEvent(message);
            handleMidi(message, fromEditor);
        }
        offset = position;
    }
    renderRange(left + offset, right + offset, audio.getNumSamples() - offset);

    juce::Reverb::Parameters space;
    space.roomSize = static_cast<float>(session.audioValue(spaceSize));
    space.damping = .45f; space.width = 1;
    space.wetLevel = static_cast<float>(session.audioValue(spaceMix)) * .45f;
    space.dryLevel = 1 - static_cast<float>(session.audioValue(spaceMix)) * .35f;
    reverb.setParameters(space);
    if (right != left) reverb.processStereo(left, right, audio.getNumSamples());
    else reverb.processMono(left, audio.getNumSamples());

    gainSmooth.setTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(session.audioValue(output))));
    float peak = 0;
    for (int i = 0; i < audio.getNumSamples(); ++i)
    {
        const auto gain = gainSmooth.getNextValue();
        left[i] = juce::jlimit(-.95f, .95f, left[i] * gain);
        if (right != left) right[i] = juce::jlimit(-.95f, .95f, right[i] * gain);
        peak = std::max(peak, std::abs(left[i]));
    }
    session.live.peak.store(peak); publish();
}

void InstrumentRenderer::publish()
{
    int external = 0;
    for (size_t i = 0; i < 31 * 128; ++i) external += held[i] || deferred[i] ? 1 : 0;
    session.live.externalKeys.store(external);
    session.live.faults.store(engine.getNumericalFaults());
    session.live.publish(engine.getNetworkSnapshot());
}

void InstrumentRenderer::reset() noexcept
{
    held = {}; deferred = {}; sustainPedal = {};
    groupHeld={};keyCounts={};physicalKeys=groupKeys=0;monoNote=-1;
    engine.allNotesOff();
    publish();
}
}
