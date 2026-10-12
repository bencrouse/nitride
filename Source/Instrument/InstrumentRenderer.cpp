#include "InstrumentRenderer.h"

namespace Nitride
{
void InstrumentRenderer::prepare(double sampleRate)
{
    rate = sampleRate;
    held = {}; deferred = {}; sustainPedal = {};
    groupHeld={};keyCounts={};presses={};velocities={};nextPress=0;physicalKeys=groupKeys=soundingKeys=0;monoNote=-1;monoMode=session.audioMono();
    wheel = 0;lfos.prepare(sampleRate);
    filterNote=DSP::filterTrackingReference;filterOwner=-1;filterVelocityValue=0;
    keyTrackingAmount=session.audioValue(filterKeyTracking);velocityAmount=session.audioValue(filterVelocity);
    editorEpoch=session.editorMidi.epoch();
    engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);
    filterEnvelope.prepare(sampleRate);
    updateEngine();
    engine.prepare(sampleRate);
    reverb.setSampleRate(sampleRate); reverb.reset(); tone = {};
    cutoffSmooth.reset(sampleRate, .025); resonanceSmooth.reset(sampleRate, .025); gainSmooth.reset(sampleRate, .025);
    cutoffSmooth.setCurrentAndTargetValue(static_cast<float>(session.audioValue(cutoff)));
    resonanceSmooth.setCurrentAndTargetValue(static_cast<float>(session.audioValue(resonance)));
    gainSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(session.audioValue(output))));
    filterAmountSmooth.reset(sampleRate,.025);filterAmountSmooth.setCurrentAndTargetValue(session.audioValue(filterAmount));
    filterNoteSmooth.reset(sampleRate,.025);filterNoteSmooth.setCurrentAndTargetValue(0);
    filterFmSmooth.reset(sampleRate,.025);filterFmSmooth.setCurrentAndTargetValue(session.audioValue(filterFmDepth));
    lastFilterCutoff=DSP::modulatedCutoff(cutoffSmooth.getCurrentValue(),0,0,0,rate);
    publish();
}

void InstrumentRenderer::updateEngine()
{
    DSP::FmSettings fmSettings;fmSettings.ratio=session.audioValue(fmRatio);fmSettings.shape=static_cast<DSP::FmShape>(static_cast<int>(session.audioValue(fmShape)));
    fmSettings.shapeAmount=session.audioValue(fmShapeAmount);fmSettings.mix=session.audioValue(fmMix);fmSettings.envelopeAmount=session.audioValue(fmEnvelopeAmount);
    fmSettings.envelope={session.audioValue(fmAttack),session.audioValue(fmDecay),session.audioValue(fmSustain),session.audioValue(fmRelease)};
    fmRouteFrames=false;
    for(size_t i=0;i<fmRoutes.size();++i)
    {
        const auto& route=fmRoutes[i];auto& setting=fmSettings.routes[i];setting={static_cast<DSP::FmSource>(static_cast<int>(session.audioValue(route.source))),static_cast<DSP::FmTarget>(static_cast<int>(session.audioValue(route.target))),session.audioValue(route.amount)};
        fmRouteFrames|=(setting.source==DSP::FmSource::lfo||setting.source==DSP::FmSource::filter)&&std::abs(setting.amount)>0;
    }
    engine.setFmSettings(fmSettings);
    filterEnvelope.configure(session.audioValue(filterAttack),session.audioValue(filterDecay),session.audioValue(filterSustain),session.audioValue(filterRelease));
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
int InstrumentRenderer::latestNoteOwner() const noexcept
{
    int selected=-1;
    // Physical keys take priority over pedal-held notes. Within each class the most
    // recent owner wins; capture owners retain the captured press's original order.
    for(int pass=0;pass<2&&selected<0;++pass)
        for(size_t i=0;i<held.size();++i)if((pass==0?held[i]:deferred[i])&&(selected<0||presses[i]>presses[static_cast<size_t>(selected)]))selected=static_cast<int>(i);
    return selected;
}
void InstrumentRenderer::refreshFilterNote(int newest)
{
    // New physical presses outrank all older owners. Re-scan only when the
    // selected key is released; unrelated polyphonic note events stay O(1).
    if(newest>=0&&(filterOwner<0||!held[static_cast<size_t>(filterOwner)]||presses[static_cast<size_t>(newest)]>presses[static_cast<size_t>(filterOwner)]))filterOwner=newest;
    if(soundingKeys==0)filterOwner=-1;
    else if(filterOwner<0||!held[static_cast<size_t>(filterOwner)])filterOwner=latestNoteOwner();
    if(filterOwner>=0){filterNote=filterOwner%128;filterVelocityValue=velocities[static_cast<size_t>(filterOwner)];}
    // No owners: retain the last note's offsets for the shared release tail.
    filterNoteSmooth.setTargetValue(DSP::filterNoteOctaves(filterNote,keyTrackingAmount,filterVelocityValue,velocityAmount));
}
void InstrumentRenderer::refreshMonoNote()
{
    const auto selected=latestNoteOwner();
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
        if(held[index]||deferred[index])--soundingKeys;
        keyCounts[index]=0;
        held[index] = deferred[index] = false;
        if (!monoMode&&!heldAnywhere(note)) engine.noteOff(note);
    }
    sustainPedal[static_cast<size_t>(channel)] = false;
    if(monoMode)refreshMonoNote();
    refreshFilterNote();
    releaseFilterGate();
}

void InstrumentRenderer::releaseFilterGate() noexcept
{if(soundingKeys==0)filterEnvelope.noteOff();}

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
            if(!held[index]&&!deferred[index])++soundingKeys;
            if(!held[index])++physicalKeys;
            held[index]=true;if(keyCounts[index]<65535)++keyCounts[index];
            deferred[index] = false;
            int captureOwner=-1;
            if (editor && midiChannel == 15)
                for(size_t c=0;c<31;++c)
                {
                    const auto owner=c*128+static_cast<size_t>(note);
                    if((held[owner]||deferred[owner])&&(captureOwner<0||presses[owner]>presses[static_cast<size_t>(captureOwner)]))captureOwner=static_cast<int>(owner);
                }
            const auto capture=captureOwner>=0;
            if(capture)
            {
                presses[index]=presses[static_cast<size_t>(captureOwner)];
            }
            else presses[index]=++nextPress;
            velocities[index]=capture?velocities[static_cast<size_t>(captureOwner)]:message.getFloatVelocity();
            if (!capture)
            {
                if(!(monoMode&&overlap&&session.audioValue(legato)>=.5))filterEnvelope.noteOn();
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
            if(!deferred[index])--soundingKeys;
            if(monoMode)refreshMonoNote();else if (!heldAnywhere(note)) engine.noteOff(note);
            releaseFilterGate();
        }
        refreshFilterNote(message.isNoteOn()?static_cast<int>(index):-1);
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
                const auto index=static_cast<size_t>(channel*128+note);
                if(deferred[index]&&!held[index])--soundingKeys;
                deferred[index] = false;
                if (!monoMode&&!heldAnywhere(note)) engine.noteOff(note);
            }
        if(monoMode)refreshMonoNote();
        refreshFilterNote();
        releaseFilterGate();
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff()) releaseChannel(channel);
}

void InstrumentRenderer::renderRange(float* left, float* right, int samples)
{
    int offset = 0;
    while (offset < samples)
    {
        const auto size = std::min(32, samples - offset);
        std::array<DSP::ModulationFrame,32> engineModulation{};
        std::array<double,32> cutoffModulation{},amplitudeModulation{},filterLevels{};
        const auto engineActive=lfos.engineActive()||fmRouteFrames;
        for(int i=0;i<size;++i)
        {
            const auto values=lfos.next();const auto n=static_cast<size_t>(i);
            filterLevels[n]=filterEnvelope.next();
            engineModulation[n]={values[0],values[1],values[4],values[5],lfos.fmWave(),filterLevels[n]};cutoffModulation[n]=values[2];amplitudeModulation[n]=values[3];
        }
        std::array<double,32> toneMotion{};
        engine.render(left+offset,right+offset,size,toneMotion.data(),engineActive?engineModulation.data():nullptr);
        for(int i=0;i<size;++i)
        {
            const auto base=static_cast<double>(cutoffSmooth.getNextValue());
            auto shift=toneMotion[static_cast<size_t>(i)]+12*cutoffModulation[static_cast<size_t>(i)];
            const auto envelopeLevel=filterLevels[static_cast<size_t>(i)],amount=filterAmountSmooth.getNextValue();
            if(std::abs(amount)>0)shift+=12*amount*envelopeLevel;
            const auto fmDepth=filterFmSmooth.getNextValue();
            // Engine::render produces a bipolar +/-0.8 soft-limited oscillator mix.
            // Tap before Tone, tremolo, Space and master gain; no feedback or LPF.
            const auto oscillator=fmDepth>0?(static_cast<double>(left[offset+i])+right[offset+i])*.625:0;
            const auto frequency=DSP::modulatedCutoff(base,shift/12+filterNoteSmooth.getNextValue(),fmDepth,oscillator,rate);
            lastFilterCutoff=frequency;
            const auto g=std::tan(juce::MathConstants<double>::pi*frequency/rate);
            const auto k=2.0-1.85*resonanceSmooth.getNextValue();const auto a=1/(1+g*(g+k));
            const auto filterSample=[&](double input,int channel) {
                auto& t=tone[static_cast<size_t>(channel)];const auto v1=a*(t[0]+g*(input-t[1]));const auto v2=t[1]+g*v1;
                t[0]=2*v1-t[0];t[1]=2*v2-t[1];return static_cast<float>(v2);
            };
            const auto amplitude=static_cast<float>(amplitudeModulation[static_cast<size_t>(i)]);
            left[offset+i]=filterSample(left[offset+i],0)*amplitude;if(right!=left)right[offset+i]=filterSample(right[offset+i],1)*amplitude;
        }
        offset += size;
    }
}

void InstrumentRenderer::process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi, bool injectEditorNotes,const DSP::Transport& transport)
{
    const juce::ScopedNoDenormals noDenormals;
    audio.clear();
    if (audio.getNumChannels() == 0 || audio.getNumSamples() == 0) return;
    updateEngine();
    keyTrackingAmount=session.audioValue(filterKeyTracking);velocityAmount=session.audioValue(filterVelocity);
    filterNoteSmooth.setTargetValue(DSP::filterNoteOctaves(filterNote,keyTrackingAmount,filterVelocityValue,velocityAmount));
    filterFmSmooth.setTargetValue(session.audioValue(filterFmDepth));
    std::array<DSP::LfoSettings,DSP::lfoCount> settings{};
    for(size_t i=0;i<settings.size();++i)
    {
        const auto& p=lfoParameters[i];settings[i]={session.audioValue(p.enabled)>=.5,session.audioValue(p.sync)>=.5,
            static_cast<DSP::LfoWave>(static_cast<int>(session.audioValue(p.wave))),session.audioValue(p.rate),session.audioValue(p.depth),static_cast<int>(session.audioValue(p.division))};
    }
    lfos.configure(settings,transport,session.audioValue(lfoTempo));
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
    filterAmountSmooth.setTargetValue(session.audioValue(filterAmount));
    resonanceSmooth.setTargetValue(static_cast<float>(session.audioValue(resonance)));
    const auto epoch=session.editorMidi.epoch();
    if(editorEpoch!=epoch){for(int channel=16;channel<32;++channel)releaseChannel(channel);editorEpoch=epoch;}
    const auto editorCount=injectEditorNotes?session.editorMidi.drain(editorCommands):0;
    auto* left = audio.getWritePointer(0);
    auto* right = audio.getWritePointer(audio.getNumChannels() > 1 ? 1 : 0);
    int offset = 0;
    auto host = midi.begin();
    std::uint32_t editor=0;
    int groupPosition=-1;
    while (host != midi.end() || editor < editorCount)
    {
        const auto fromEditor=editor<editorCount&&(host==midi.end()||(*host).samplePosition>0);
        if(fromEditor&&editorCommands[editor].epoch!=editorEpoch){++editor;continue;}
        const auto position=fromEditor?offset:juce::jlimit(offset,audio.getNumSamples(),(*host).samplePosition);
        renderRange(left + offset, right + offset, position - offset);
        if(position!=groupPosition){beginMidiGroup();groupPosition=position;}
        // Large SysEx is not part of this instrument's input protocol. Avoid materializing
        // an allocating MidiMessage on the audio thread for those messages.
        if(fromEditor)
        {
            const auto& command=editorCommands[editor++];handleMidi(juce::MidiMessage(command.bytes.data(),3),true);
        }
        else{const auto metadata=*host++;if(metadata.numBytes<=3)handleMidi(metadata.getMessage(),false);}
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
    for(size_t note=0;note<128;++note)
    {
        std::uint16_t down=0,sounding=0;
        for(size_t channel=0;channel<16;++channel){const auto i=channel*128+note;if(held[i])down|=static_cast<std::uint16_t>(1u<<channel);if(held[i]||deferred[i])sounding|=static_cast<std::uint16_t>(1u<<channel);}
        session.editorMidi.hostHeld[note].store(down,std::memory_order_relaxed);session.editorMidi.hostSounding[note].store(sounding,std::memory_order_relaxed);
    }
    session.live.faults.store(engine.getNumericalFaults());
    for(size_t i=0;i<DSP::lfoCount;++i)session.live.lfoPhases[i].store(static_cast<float>(lfos.phase(i)),std::memory_order_relaxed);
    session.live.lfoBpm.store(static_cast<float>(lfos.bpm()));session.live.lfoHostTempo.store(lfos.hostTempo());
    session.live.publish(engine.getNetworkSnapshot());
}

void InstrumentRenderer::reset() noexcept
{
    held = {}; deferred = {}; sustainPedal = {};
    groupHeld={};keyCounts={};physicalKeys=groupKeys=soundingKeys=0;monoNote=-1;
    filterEnvelope.reset();
    filterNote=DSP::filterTrackingReference;filterOwner=-1;filterVelocityValue=0;filterNoteSmooth.setCurrentAndTargetValue(0);
    lfos.prepare(rate);
    engine.allNotesOff();
    session.editorMidi.reset();editorEpoch=session.editorMidi.epoch();
    publish();
}
}
