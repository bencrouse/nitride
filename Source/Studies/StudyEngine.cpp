#include "StudyEngine.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace SoundStudies
{
namespace
{
constexpr auto twoPi = std::numbers::pi * 2.0;
constexpr int oversampling = 4;
constexpr std::array<double, 4> bodyRatios { 1.0, 2.17, 3.91, 6.32 };
constexpr std::array<double, 4> bodyWeights { 0.58, 0.25, 0.13, 0.08 };
constexpr std::array<double, 4> interactionRatios { 1.0, 1.414, 2.73, 4.18 };
constexpr std::array<std::array<size_t, 2>, 9> networkEdges {{
    {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}, {0, 3}, {1, 4}, {2, 5}
}};
constexpr std::array<double, 9> networkSigns { 1, 1, -1, 1, -1, 1, -1, 1, -1 };
constexpr std::array<double, 6> networkHarmonics { 1, 2, 3, 5, 7, 9 };
constexpr std::array<double, 6> networkDetuning { 0, 0.06, -0.21, 0.28, -0.35, 0.46 };
constexpr std::array<double, 6> networkKick { 0, 0.7, -0.9, 0.6, -0.5, 1.0 };
constexpr std::array<double, 9> networkFrustration { 0.6, -0.8, 0.5, -0.7, 0.9, -0.4, 1.0, -0.85, 0.7 };

double wrap(double phase) noexcept
{
    return phase - twoPi * std::floor(phase / twoPi);
}

double readDelay(const std::array<float, 4096>& samples, size_t cursor, double delay) noexcept
{
    auto position = static_cast<double>(cursor) - delay;
    if (position < 0) position += static_cast<double>(samples.size());
    const auto index = static_cast<int>(position);
    const auto fraction = position - static_cast<double>(index);
    const auto sampleAt = [&samples, index](int offset) {
        const auto size = static_cast<int>(samples.size());
        return static_cast<double>(samples[static_cast<size_t>((index + offset + size) % size)]);
    };
    const auto y0 = sampleAt(-1), y1 = sampleAt(0), y2 = sampleAt(1), y3 = sampleAt(2);
    return y1 + 0.5 * fraction * (y2 - y0 + fraction
        * (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 + fraction * (3.0 * (y1 - y2) + y3 - y0)));
}
}

double Engine::sourceIndex(Source selected) noexcept
{
    return selected == Source::rich ? 2.7 : 1.65;
}

double Engine::bessel(int order, double index) noexcept
{
    // Small fixed FM index: convergent power series avoids platform-specific special functions.
    double term = 1.0;
    for (int i = 1; i <= order; ++i) term *= index * 0.5 / static_cast<double>(i);
    double sum = term;
    for (int k = 1; k < 24; ++k)
    {
        term *= -index * index * 0.25 / static_cast<double>(k * (k + order));
        sum += term;
    }
    return sum;
}

double Engine::partialRatio(int partial, double amount) noexcept
{
    const auto harmonic = static_cast<double>(2 * partial + 1);
    // Keep the fundamental anchored; spread higher sidebands progressively further apart.
    return harmonic + amount * (0.38 * (harmonic - 1.0) + 0.075 * std::pow(harmonic - 1.0, 1.45));
}

void Engine::prepare(double sampleRate)
{
    // Use the host's actual clock, including the low rates exercised by AU validation.
    // Clamping a valid rate silently changes every note's pitch and envelope timing.
    rate = std::isfinite(sampleRate) && sampleRate > 0 ? sampleRate : 48000.0;
    internalRate = rate * oversampling;
    smoothing = 1.0 - std::exp(-1.0 / (rate * 0.025));
    decimationCoefficient = 1.0 - std::exp(-twoPi * (rate * 0.38) / internalRate);
    networkLearning = 1.0 - std::exp(-1.0 / (internalRate * 0.025));
    networkImpulseDecay = std::exp(-1.0 / (internalRate * 0.035));
    couplingDimension = targetCoupling;
    stressDimension = targetStress;
    responseDimension = targetResponse;
    updateDimensionRates();
    voices = {};
    downsampleFilter = {};
    modeWeights = {};
    modeWeights[static_cast<size_t>(treatment)] = 1.0;
    smoothedAmount = targetAmount;
    sourceBlend = source == Source::rich ? 1.0 : 0.0;
    currentIndex = indexOverride ? targetIndex : sourceIndex(source);
    envelopeValues = envelopeTarget;
    smoothedBend = targetBend = 0.0;
    pitchMultiplier = 1.0;
    clock = 0;
    nextSerial = numericalFaults = 0;
    playing = false;
    groupOrigin=lastNoteHz=toneAnchorHz=0;
    for (size_t i = 0; i < partialAmplitudes.size(); ++i)
    {
        const auto n = static_cast<int>(i);
        // Fold negative-frequency sidebands of sin(t + beta*sin(2t)) onto positive harmonics.
        for (size_t s = 0; s < sourceAmplitudes.size(); ++s)
        {
            const auto index = sourceIndex(static_cast<Source>(s));
            sourceAmplitudes[s][i] = bessel(n, index) + (n % 2 == 0 ? 1.0 : -1.0) * bessel(n + 1, index);
        }
        partialAmplitudes[i] = sourceAmplitudes[static_cast<size_t>(source)][i];
        partialSpreads[i] = partialRatio(n, 1.0) - static_cast<double>(2 * n + 1);
    }
    for (size_t row = 0; row < indexAmplitudes.size(); ++row)
        for (size_t i = 0; i < partialAmplitudes.size(); ++i)
        {
            const auto index = static_cast<double>(row) / 32.0;
            const auto n = static_cast<int>(i);
            indexAmplitudes[row][i] = bessel(n, index) + (n % 2 == 0 ? 1.0 : -1.0) * bessel(n + 1, index);
        }
    for(size_t n=0;n<rawBessel.size();++n)
    {
        for(size_t row=0;row<indexBessel.size();++row)indexBessel[row][n]=bessel(static_cast<int>(n),static_cast<double>(row)/32);
        for(size_t s=0;s<sourceBessel.size();++s)sourceBessel[s][n]=bessel(static_cast<int>(n),sourceIndex(static_cast<Source>(s)));
    }
}

void Engine::setPitchMotion(const PitchSettings& requested) noexcept
{
    const auto settings=validPitchSettings(requested);
    if(pitchOverride&&samePitchSettings(settings,pitchSettings))return;
    pitchOverride=true;
    const auto startGlide=settings.glide.enabled&&!pitchSettings.glide.enabled;
    if(settings.glide.enabled&&settings.glide.target==PitchTarget::tone
        &&(!pitchSettings.glide.enabled||pitchSettings.glide.target!=PitchTarget::tone))
    {
        toneAnchorHz=lastNoteHz;
    }
    pitchSettings=settings;
    for(auto& voice:voices)if(voice.active)
    {
        if(startGlide)voice.glide.begin(voice.note,voice.frequency,settings.glide.seconds,settings.glide.curve,rate);
        voice.glide.retime(settings.glide.seconds,settings.glide.curve);voice.autobend.retime(settings.autobend.seconds);
        voice.glideRouting.set(settings.glide.enabled,settings.glide.target,rate);
        voice.autobendRouting.set(settings.autobend.enabled,settings.autobend.target,rate);
    }
}
void Engine::beginNoteGroup() noexcept
{
    groupOrigin=lastNoteHz;
    if(std::none_of(voices.begin(),voices.end(),[](const auto& v){return v.active;}))toneAnchorHz=0;
}
Engine::PitchSnapshot Engine::getPitchSnapshot(int note) const noexcept
{
    const Voice* selected=nullptr;
    for(const auto& v:voices)if(v.active&&(note<0||v.note==note)&&(!selected||v.serial>selected->serial))selected=&v;
    if(!selected)return{};
    return{selected->note,selected->carrierHz,selected->modulatorHz,selected->glide.frequency(),selected->autoOffset};
}

void Engine::updateVoicePitch(Voice& voice) noexcept
{
    if(voice.velocitySamples>0){voice.velocity+=voice.velocityStep;if(--voice.velocitySamples==0)voice.velocity=voice.targetVelocity;}
    if(!pitchSettings.glide.enabled&&!pitchSettings.autobend.enabled&&!voice.splitPhase&&voice.glideRouting.silent()&&voice.autobendRouting.silent())
    {
        voice.frequency=voice.glide.destination();voice.carrierHz=voice.modulatorHz=voice.frequency*pitchMultiplier;
        voice.tonePitch=0;voice.carrierBand=1;voice.autobend.advance();return;
    }
    voice.glide.advance();voice.autobend.advance();
    const auto& g=voice.glideRouting.advance();const auto& a=voice.autobendRouting.advance();
    const auto target=voice.glide.destination(),current=voice.glide.frequency();
    const auto glideRatio=current/target;
    const auto needsGlideOffset=(!pitchIsZero(g[0])&&!samePitchValue(g[0],1))||(!pitchIsZero(g[1])&&!samePitchValue(g[1],1));
    const auto glideOffset=needsGlideOffset?12*std::log2(glideRatio):0;
    // Depth changes are dezippered on held voices; new strikes initialize at full depth.
    voice.autoDepth+=std::min(1.0,1/(rate*.003))*(pitchSettings.autobend.depth-voice.autoDepth);
    voice.autoOffset=voice.autoDepth*voice.autobend.value();
    const auto glideHz=[&](double weight) {
        if(samePitchValue(weight,1))return current;if(pitchIsZero(weight))return target;
        if(samePitchValue(weight,-1))return target/glideRatio;
        return target*std::exp2(glideOffset*weight/12);
    };
    voice.carrierHz=glideHz(g[0])*pitchMultiplier;
    voice.modulatorHz=glideHz(g[1])*pitchMultiplier;
    if(!pitchIsZero(voice.autoOffset))
    {
        if(!pitchIsZero(a[0]))voice.carrierHz*=std::exp2(voice.autoOffset*a[0]/12);
        if(!pitchIsZero(a[1]))voice.modulatorHz*=std::exp2(voice.autoOffset*a[1]/12);
    }
    voice.frequency=voice.glideRouting.silent()?target:current;
    if(!voice.splitPhase&&!samePitchValue(voice.carrierHz,voice.modulatorHz)){voice.splitPhase=true;voice.modulatorPhase=voice.phase;}
    const auto bandLimitedMotion=voice.splitPhase||(pitchSettings.glide.enabled&&pitchSettings.glide.curve!=GlideCurve::classic)
        ||(pitchSettings.autobend.enabled&&!pitchIsZero(pitchSettings.autobend.depth));
    voice.carrierBand=bandLimitedMotion?std::clamp((rate*.47-voice.carrierHz)/(rate*.08),0.0,1.0):1;
    voice.tonePitch=(!pitchIsZero(g[2])&&toneAnchorHz>0?12*std::log2(current/toneAnchorHz)*g[2]:0)+voice.autoOffset*a[2];
    if(voice.splitPhase&&(!samePitchValue(voice.previousCarrierHz,voice.carrierHz)||!samePitchValue(voice.previousModulatorHz,voice.modulatorHz)))
    {
        for(size_t n=0;n<voice.positiveBands.size();++n)
        {
            const auto band=[&](double hz){return std::clamp((rate*.47-std::abs(hz))/(rate*.08),0.0,1.0);};
            voice.positiveBands[n]=band(voice.carrierHz+2*static_cast<double>(n)*voice.modulatorHz);
            voice.negativeBands[n]=band(voice.carrierHz-2*static_cast<double>(n)*voice.modulatorHz);
        }
        voice.previousCarrierHz=voice.carrierHz;voice.previousModulatorHz=voice.modulatorHz;
    }
}
double Engine::splitReference(const Voice& voice) const noexcept
{
    // Band-limited FM sidebands for independent carrier/modulator phases. A short
    // trig recurrence evaluates the same Bessel expansion without 25 sine calls.
    const auto carrierSin=std::sin(voice.phase),carrierCos=std::cos(voice.phase);
    const auto stepSin=std::sin(voice.modulatorPhase*2),stepCos=std::cos(voice.modulatorPhase*2);
    double sine=0,cosine=1,result=rawBessel[0]*carrierSin*voice.positiveBands[0];
    for(size_t n=1;n<rawBessel.size();++n)
    {
        const auto nextSin=sine*stepCos+cosine*stepSin;
        cosine=cosine*stepCos-sine*stepSin;sine=nextSin;
        if(n<12)result+=rawBessel[n]*(carrierSin*cosine+carrierCos*sine)*voice.positiveBands[n];
        result+=(n%2==0?1:-1)*rawBessel[n]*(carrierSin*cosine-carrierCos*sine)*voice.negativeBands[n];
    }
    return result;
}

void Engine::setFmIndex(double index) noexcept
{
    indexOverride = true;
    targetIndex = std::isfinite(index) ? std::clamp(index, 0.0, 4.0) : 1.65;
}

void Engine::setAmplitudeEnvelope(double attack, double decay, float sustain, double release) noexcept
{
    envelopeOverride = true;
    envelopeTarget = { std::isfinite(attack) ? std::clamp(attack, 0.001, 4.0) : 0.006,
        std::isfinite(decay) ? std::clamp(decay, 0.01, 4.0) : 0.16,
        std::isfinite(sustain) ? std::clamp(static_cast<double>(sustain), 0.0, 1.0) : 0.62,
        std::isfinite(release) ? std::clamp(release, 0.02, 8.0) : 0.16 };
}

void Engine::setAmount(float amount) noexcept
{
    targetAmount = std::isfinite(amount) ? std::clamp(static_cast<double>(amount), 0.0, 1.0) : 0.0;
}

void Engine::setPitchBend(float semitones) noexcept
{
    // Product tuning remains +/-12; allow its sum with the independent MIDI wheel.
    targetBend = std::isfinite(semitones) ? std::clamp(static_cast<double>(semitones), -24.0, 24.0) : 0.0;
}

void Engine::setNetworkDimensions(float coupling, float stress, double responseSeconds) noexcept
{
    targetCoupling = std::isfinite(coupling) ? std::clamp(static_cast<double>(coupling), 0.0, 1.0) : 0.0;
    targetStress = std::isfinite(stress) ? std::clamp(static_cast<double>(stress), 0.0, 1.0) : 0.0;
    targetResponse = std::isfinite(responseSeconds) ? std::clamp(responseSeconds, 0.01, 1.6) : 0.025;
}

void Engine::updateDimensionRates() noexcept
{
    if (std::abs(responseDimension - 0.025) < 1.0e-12)
    {
        expressiveLearning = networkLearning;
        expressiveImpulseDecay = networkImpulseDecay;
        expressivePhaseSpeed = 1.0;
        return;
    }
    expressiveLearning = 1.0 - std::exp(-1.0 / (internalRate * responseDimension));
    expressiveImpulseDecay = std::exp(-1.0 / (internalRate * responseDimension * 1.4));
    expressivePhaseSpeed = 0.025 / responseDimension;
}

Engine::NetworkSnapshot Engine::getNetworkSnapshot() const noexcept
{
    NetworkSnapshot snapshot;
    const Voice* selected = nullptr;
    for (const auto& voice : voices)
        if (voice.active)
        {
            ++snapshot.activeVoices;
            if (selected == nullptr || voice.serial > selected->serial) selected = &voice;
        }
    if (selected == nullptr) return snapshot;
    const auto& state = treatment == Treatment::expressiveNetwork ? selected->expressiveNetwork
        : treatment == Treatment::stressedNetwork ? selected->stressedNetwork : selected->network;
    double real = 0.0, imaginary = 0.0;
    for (size_t i = 0; i < state.offsets.size(); ++i)
    {
        snapshot.offsets[i] = static_cast<float>(wrap(state.offsets[i]+(selected->splitPhase&&i!=0?(selected->modulatorPhase-selected->phase)*networkHarmonics[i]:0)));
        real += std::cos(state.offsets[i]);
        imaginary += std::sin(state.offsets[i]);
    }
    for (size_t i = 0; i < state.links.size(); ++i) snapshot.links[i] = static_cast<float>(state.links[i]);
    snapshot.level = static_cast<float>(selected->envelope * selected->velocity);
    snapshot.coherence = static_cast<float>(std::hypot(real, imaginary) / 6.0);
    return snapshot;
}

double Engine::patternSeconds(Pattern selected) noexcept
{
    return selected == Pattern::sweep ? 10.0 : selected == Pattern::hits ? 4.0 : 8.0;
}

float Engine::sweepAt(double seconds) noexcept
{
    if (seconds < 1.0 || seconds >= 8.0) return 0.0f;
    if (seconds < 4.0) return static_cast<float>((seconds - 1.0) / 3.0);
    if (seconds < 5.0) return 1.0f;
    return static_cast<float>((8.0 - seconds) / 3.0);
}

void Engine::start(Pattern selected) noexcept
{
    allNotesOff();
    pattern = selected;
    playing = true;
    clock = 0;
    setPitchBend(0);
    loopSamples = static_cast<std::int64_t>(std::llround(patternSeconds(selected) * rate));
}

void Engine::stop() noexcept
{
    playing = false;
    setPitchBend(0);
    allNotesOff();
}

void Engine::noteOn(int note, float velocity, Articulation articulation,bool overlapping,bool returning) noexcept
{
    if (velocity <= 0.0f) { noteOff(note); return; }
    auto* voice = &voices.front();
    bool retrigger = false;
    for (auto& candidate : voices)
    {
        if (!candidate.active) { voice = &candidate; break; }
        if (candidate.serial < voice->serial) voice = &candidate;
    }
    if (articulation == Articulation::lead)
        for (auto& candidate : voices)
            if (candidate.active && candidate.articulation == Articulation::lead)
            {
                voice = &candidate;
                retrigger = true;
                break;
            }
    const auto origin=retrigger?voice->frequency:groupOrigin;
    const auto stolen=voice->active&&pitchOverride&&(pitchSettings.glide.enabled||pitchSettings.autobend.enabled)?voice->lastSample:0;
    if (!retrigger)
    {
        *voice={};voice->stolenSample=stolen;voice->stealLength=std::max(1,static_cast<int>(std::ceil(internalRate*.002)));
        if(!pitchIsZero(stolen))voice->stealSamples=voice->stealLength;
    }
    const auto preserveEnvelope=retrigger&&(returning||(overlapping&&pitchSettings.legato));
    if(!preserveEnvelope){voice->attackStart=voice->envelope;voice->age=voice->releaseAge=0;}
    voice->released = false;
    voice->active = true;
    voice->articulation = articulation;
    voice->note = std::clamp(note, 0, 127);
    if (!retrigger) voice->frequency = 440.0 * std::exp2(static_cast<double>(voice->note - 69) / 12.0);
    voice->targetVelocity=std::clamp(static_cast<double>(velocity),0.0,1.0);
    if(preserveEnvelope)
    {
        voice->velocitySamples=std::max(1,static_cast<int>(std::ceil(rate*.003)));
        voice->velocityStep=(voice->targetVelocity-voice->velocity)/voice->velocitySamples;
    }
    else{voice->velocity=voice->targetVelocity;voice->velocitySamples=0;}
    if(!preserveEnvelope)voice->network.impulse = voice->stressedNetwork.impulse = voice->expressiveNetwork.impulse = voice->velocity;
    if(pitchOverride)
    {
        const auto slide=pitchSettings.glide.enabled&&(!pitchSettings.glide.legatoOnly||overlapping||returning);
        voice->glide.begin(voice->note,slide?origin:0,pitchSettings.glide.seconds,pitchSettings.glide.curve,rate);
        if(!retrigger){voice->glideRouting.reset(pitchSettings.glide.enabled,pitchSettings.glide.target);voice->autobendRouting.reset(pitchSettings.autobend.enabled,pitchSettings.autobend.target);}
        voice->autoDepth=pitchSettings.autobend.depth;
        if(pitchSettings.autobend.enabled&&!returning&&(!pitchSettings.autobend.phraseOnly||!overlapping))
            voice->autobend.trigger(pitchSettings.autobend.seconds,rate);
        if(toneAnchorHz<=0)toneAnchorHz=voice->glide.destination();
        lastNoteHz=voice->glide.frequency();
    }
    if (!retrigger)
        for (size_t i = 0; i < voice->network.links.size(); ++i)
            voice->network.links[i] = voice->stressedNetwork.links[i] = voice->expressiveNetwork.links[i] = networkSigns[i] * 0.55;
    voice->serial = ++nextSerial;
}

void Engine::noteOff(int note) noexcept
{
    for (auto& voice : voices)
        if (voice.active && voice.note == note && !voice.released)
        {
            voice.released = true;
            voice.releaseLevel = voice.envelope;
            voice.releaseAge = 0.0;
        }
}

void Engine::allNotesOff() noexcept
{
    for (auto& voice : voices)
        if (voice.active) noteOff(voice.note);
}

void Engine::schedule() noexcept
{
    if (!playing) return;
    const auto position = clock % loopSamples;
    const auto samplesAt = [this](double seconds) { return static_cast<std::int64_t>(std::llround(seconds * rate)); };
    if (pattern == Pattern::pad || pattern == Pattern::sweep)
    {
        constexpr std::array<int, 4> chord { 48, 55, 59, 62 };
        if (position == 0) for (const auto note : chord) noteOn(note, 0.7f);
        const auto off = pattern == Pattern::sweep ? 8.0 : 6.0;
        if (position == samplesAt(off)) for (const auto note : chord) noteOff(note);
    }
    else if (pattern == Pattern::hits)
    {
        constexpr std::array<int, 8> notes { 48, 60, 67, 55, 72, 60, 63, 48 };
        const auto step = samplesAt(0.5);
        const auto index = static_cast<size_t>(position / step);
        if (position % step == 0) noteOn(notes[index], 0.72f, Articulation::hit);
        if (position % step == samplesAt(0.12)) noteOff(notes[index]);
    }
    else if (pattern == Pattern::velocity)
    {
        constexpr std::array<float, 8> velocities { 0.15f, 0.3f, 0.5f, 0.7f, 0.95f, 0.7f, 0.5f, 0.3f };
        const auto step = samplesAt(1.0);
        if (position % step == 0) noteOn(60, velocities[static_cast<size_t>(position / step)], Articulation::hit);
        if (position % step == samplesAt(0.3)) noteOff(60);
    }
    else
    {
        struct Note { double time, length; int pitch; float velocity; };
        constexpr std::array<Note, 12> phrase {{
            { 0.0, .16, 48, .45f }, { .25, .16, 48, .8f }, { .5, .25, 60, .95f },
            { .875, .4, 63, .65f }, { 1.5, .22, 55, .85f }, { 2.0, .9, 60, .75f },
            { 3.0, .3, 60, .4f }, { 3.5, .4, 67, .85f }, { 4.0, 1.0, 55, .9f },
            { 5.25, .3, 48, .65f }, { 6.0, .9, 60, .9f }, { 7.0, .3, 72, .65f }
        }};
        for (const auto& event : phrase)
        {
            if (position == samplesAt(event.time)) noteOn(event.pitch, event.velocity, Articulation::lead);
            if (position == samplesAt(event.time + event.length)) noteOff(event.pitch);
        }
        constexpr std::array<std::array<double, 2>, 10> bends {{
            {0, 0}, {2.25, -2}, {2.55, 2}, {2.8, 0}, {4.25, 4},
            {4.65, -3}, {4.9, 0}, {6.25, -5}, {6.55, 3}, {6.85, 0}
        }};
        for (const auto& bend : bends)
            if (position == samplesAt(bend[0])) setPitchBend(static_cast<float>(bend[1]));
    }
}

double Engine::envelope(Voice& voice) noexcept
{
    voice.network.impulse *= networkImpulseDecay;
    voice.stressedNetwork.impulse *= networkImpulseDecay;
    voice.expressiveNetwork.impulse *= expressiveImpulseDecay;
    voice.age += 1.0 / internalRate;
    if (voice.released)
    {
        voice.releaseAge += 1.0 / internalRate;
        const auto release = envelopeOverride ? envelopeValues[3]
            : voice.articulation == Articulation::pad ? 1.4 : voice.articulation == Articulation::lead ? 0.16 : 0.22;
        voice.envelope = voice.releaseLevel * std::max(0.0, 1.0 - voice.releaseAge / release);
        if (voice.envelope < 0.000001) voice.active = false;
    }
    else
    {
        const auto attack = envelopeOverride ? envelopeValues[0]
            : voice.articulation == Articulation::pad ? 0.65 : voice.articulation == Articulation::lead ? 0.006 : 0.003;
        const auto sustain = envelopeOverride ? envelopeValues[2]
            : voice.articulation == Articulation::pad ? 0.72 : voice.articulation == Articulation::lead ? 0.62 : 0.0;
        const auto decay = envelopeOverride ? envelopeValues[1] : voice.articulation == Articulation::pad ? 0.7 : 0.16;
        voice.envelope = voice.age < attack ? voice.attackStart + (1.0 - voice.attackStart) * voice.age / attack
            : sustain + (1.0 - sustain) * std::exp(-(voice.age - attack) / decay);
    }
    return voice.envelope;
}

double Engine::bodySample(Voice& voice, double env, double fundamental) noexcept
{
    const auto amount = smoothedAmount;
    const auto feedback = amount * 1.6 * std::tanh(voice.bodySignal * 0.8);
    const auto driven = std::sin(voice.phase + currentIndex * std::sin(voice.phase * 2.0 + feedback));
    double body = 0.0;
    for (size_t i = 0; i < voice.modes.size(); ++i)
    {
        auto& mode = voice.modes[i];
        const auto neighbour = voice.modes[(i + 1) % voice.modes.size()].displacement;
        const auto stiffness = std::sqrt(1.0 + amount * 7.0 * mode.displacement * mode.displacement);
        const auto frequency = std::min(fundamental * bodyRatios[i] * stiffness, internalRate * 0.35);
        const auto angle = twoPi * frequency / internalRate;
        const auto forcing = driven * env * voice.velocity + amount * 0.12 * std::tanh(neighbour);
        mode.velocity += forcing * angle * 0.075;
        // Exact oscillator rotation with state-dependent stiffness (a simplified nonlinear body).
        const auto sine = std::sin(angle), cosine = std::cos(angle);
        const auto damping = bodyDamping[i];
        const auto displacement = (mode.displacement * cosine + mode.velocity * sine) * damping;
        mode.velocity = (mode.velocity * cosine - mode.displacement * sine) * damping;
        mode.displacement = displacement;
        body += displacement * bodyWeights[i];
    }
    voice.bodySignal = body;
    return driven * 0.62 + std::tanh(body * 0.85) * 0.6;
}

double Engine::interactionSample(Voice& voice, double env, double fundamental) noexcept
{
    const auto a = smoothedAmount;
    const auto feedback = a * 7.5 * std::tanh(voice.interactionSignal * 1.3);
    const auto modulator = std::sin(voice.phase * 2.0 + feedback)
        + a * 0.35 * std::sin(voice.phase * 3.0 - feedback * 0.6);
    const auto driven = std::sin(voice.phase + currentIndex * modulator);
    std::array<double, 4> previous {};
    for (size_t i = 0; i < previous.size(); ++i) previous[i] = voice.interactionModes[i].displacement;
    double body = 0.0;
    for (size_t i = 0; i < voice.interactionModes.size(); ++i)
    {
        auto& mode = voice.interactionModes[i];
        // Softening at small displacement and hardening at large displacement create
        // a different dynamical response from Study 01's uniformly stiffening body.
        const auto stiffness = std::sqrt(std::max(0.12, 1.0 - a * 0.8 + a * 9.0 * mode.displacement * mode.displacement));
        const auto frequency = std::min(fundamental * interactionRatios[i] * stiffness, internalRate * 0.35);
        const auto angle = twoPi * frequency / internalRate;
        const auto cross = previous[(i + 1) % 4] - previous[(i + 3) % 4] * 0.65;
        const auto forcing = std::tanh(driven * env * voice.velocity * (1.0 + a * 2.5) + a * 0.65 * cross);
        mode.velocity += forcing * angle * 0.14;
        const auto sine = std::sin(angle), cosine = std::cos(angle);
        const auto q = (mode.displacement * cosine + mode.velocity * sine) * interactionDamping[i];
        const auto v = (mode.velocity * cosine - mode.displacement * sine) * interactionDamping[i];
        mode.displacement = 2.0 * std::tanh(q * 0.5);
        mode.velocity = 2.0 * std::tanh(v * 0.5);
        body += mode.displacement * bodyWeights[i];
    }
    voice.interactionSignal = body;
    return driven * 0.45 + std::sin(body * (0.65 + a * 1.8)) * 0.75;
}

double Engine::phaseDelaySample(Voice& voice, double env, double fundamental) noexcept
{
    const auto a = smoothedAmount;
    const auto nominalDelay = internalRate / (fundamental * 3.0);
    const auto warp = a * (0.65 * std::sin(voice.phase * 1.5 + voice.delaySignal * 0.8)
                           + 0.25 * std::sin(voice.phase * 4.13));
    const auto delay = std::clamp(nominalDelay * (1.0 + warp), 2.0, static_cast<double>(voice.delay.size() - 3));
    // Four-point interpolation reduces read-head artifacts under rapid delay modulation.
    const auto delayed = readDelay(voice.delay, voice.delayCursor, delay);
    const auto deformation = a * (5.5 * delayed + 1.25 * std::sin(voice.phase * 1.5));
    const auto phase = voice.phase + a * (0.4 * std::sin(voice.phase * 3.0) + 2.0 * voice.delaySignal);
    const auto excitation = std::sin(phase + currentIndex * std::sin(voice.phase * 2.0 + deformation));
    const auto returned = std::tanh(excitation * (0.45 + 0.55 * env) * voice.velocity
                                  + a * (0.75 * delayed + 0.15 * std::sin(delayed * 2.8)));
    voice.delay[voice.delayCursor] = static_cast<float>(returned);
    voice.delayCursor = (voice.delayCursor + 1) % voice.delay.size();
    voice.delaySignal = returned;
    return returned * 1.2;
}

double Engine::noteCoupledDelaySample(Voice& voice, double env, double fundamental) noexcept
{
    auto& state = voice.noteDelay;
    const auto a = smoothedAmount;
    const auto excitationStrength = env * voice.velocity;
    const auto response = 0.3 + 0.7 * std::sqrt(excitationStrength);
    // Note-synchronous harmonics and a continuous octave-related phase replace
    // the original read head's incommensurate motion and phase-wrap resets.
    const auto warp = a * response * (0.52 * std::sin(voice.phase * 2.0 + state.signal * 0.35)
                                      + 0.2 * std::sin(voice.noteDelayPhase));
    const auto delay = std::clamp(internalRate / (fundamental * 3.0) * (1.0 + warp),
                                  2.0, static_cast<double>(state.samples.size() - 3));
    const auto delayed = readDelay(state.samples, state.cursor, delay);
    const auto deformation = a * response * (3.6 * delayed + 0.8 * std::sin(voice.noteDelayPhase));
    const auto phase = voice.phase + a * response * (0.65 * std::sin(voice.phase * 3.0) + 1.1 * state.signal);
    const auto excitation = std::sin(phase + currentIndex * std::sin(voice.phase * 2.0 + deformation))
        * excitationStrength * (0.75 + 0.75 * a);
    // Below-unity recirculation decreases with the note envelope. With no excitation,
    // the delay decays rather than becoming an independently sustaining source.
    const auto feedback = (0.28 + 0.32 * a) * (0.35 + 0.65 * env);
    const auto returned = std::tanh(excitation + feedback * delayed);
    state.samples[state.cursor] = static_cast<float>(returned);
    state.cursor = (state.cursor + 1) % state.samples.size();
    state.signal = returned;
    voice.noteDelayPhase = wrap(voice.noteDelayPhase + twoPi * fundamental * 0.5 / internalRate);
    return returned * 1.2;
}

template<bool Split>
double Engine::networkSample(Voice& voice, NetworkState& state, double env, double fundamental, double a, double stress,
                             double learning, double phaseSpeed) noexcept
{
    const auto excitation = env * voice.velocity;
    std::array<double, 6> sine {}, cosine {}, torque {};
    for (size_t i = 0; i < sine.size(); ++i)
    {
        const auto offset=state.offsets[i]+(Split&&i!=0?(voice.modulatorPhase-voice.phase)*networkHarmonics[i]:0);
        sine[i] = std::sin(offset);
        cosine[i] = std::cos(offset);
    }
    for (size_t edge = 0; edge < networkEdges.size(); ++edge)
    {
        const auto from = networkEdges[edge][0], to = networkEdges[edge][1];
        const auto difference = sine[to] * cosine[from] - cosine[to] * sine[from];
        const auto alignment = cosine[to] * cosine[from] + sine[to] * sine[from];
        const auto target = networkSigns[edge] * (networkAdaptive ? 0.2 + 0.8 * (0.5 + 0.5 * alignment) : 0.55);
        state.links[edge] += learning * (0.25 + 0.75 * excitation) * (target - state.links[edge]);
        // At high stress, connections prefer conflicting relative phases while the
        // adaptive rule still favours alignment. The resulting conflict is inside the voice.
        const auto bias = stress * stress * networkFrustration[edge] * (0.3 + 0.6 * excitation);
        const auto pull = state.links[edge] * (stress > 0.0
            ? difference * std::cos(bias) - alignment * std::sin(bias) : difference);
        torque[from] += pull;
        torque[to] -= pull;
    }
    const auto phaseStep = twoPi * fundamental / internalRate;
    const auto coupling = a * (0.8 + 3.6 * a) * (0.45 + 0.55 * excitation) * (1.0 + 3.5 * stress);
    std::array<double, 6> wave {};
    for (size_t i = 0; i < wave.size(); ++i)
    {
        const auto anchor = i == 0 ? 2.5 + 2.0 * stress : 0.9 * (1.0 - 0.8 * a);
        // Keep the network map identical between the two FM source profiles.
        const auto drift = a * 0.65 * networkDetuning[i];
        const auto impulse = a * state.impulse * networkKick[i] * 1.8 * (1.0 + 3.0 * stress);
        const auto feedback = a * (0.18 + 1.4 * stress * stress) * state.signal * networkKick[i];
        const auto force = drift + coupling * torque[i] / 3.0 - a * anchor * sine[i] + impulse + feedback;
        state.offsets[i] = wrap(state.offsets[i] + phaseStep * force * phaseSpeed);
        // The carrier uses its phase directly; only nodes 1–5 are FM modulators.
        if (i != 0)
        {
            wave[i]=std::sin((Split?voice.modulatorPhase:voice.phase)*networkHarmonics[i]+state.offsets[i]);
            if constexpr(Split)wave[i]*=std::clamp((rate*.47-voice.modulatorHz*networkHarmonics[i])/(rate*.08),0.0,1.0);
        }
    }
    // The adaptive links participate in FM generation as well as phase synchronization.
    // This is one evolving voice, rather than a network processing a finished FM signal.
    const auto modulator = wave[1] + a * (0.25 * state.links[1] * wave[2] + 0.5 * state.links[6] * wave[3]
        + 0.35 * state.links[7] * wave[4] + 0.25 * state.links[8] * wave[5]);
    const auto carrier = voice.phase + state.offsets[0];
    auto result = std::sin(carrier + currentIndex * (1.0 + 2.4 * stress) * modulator
        + a * (1.4 + 7.5 * stress) * state.signal);
    if(!samePitchValue(voice.carrierBand,1))result*=voice.carrierBand;
    const auto folded = std::sin(result * std::numbers::pi * (0.5 + 2.5 * stress));
    state.signal = ((1.0 - stress) * result + stress * folded) * excitation;
    return result;
}

template<bool Split>
double Engine::voiceSample(Voice& voice) noexcept
{
    const auto env = envelope(voice);
    const auto fundamental = pitchOverride?voice.carrierHz:voice.frequency*pitchMultiplier;
    const auto amount = smoothedAmount;
    const auto spectralActive = modeWeights[2] > 0.00000001;
    // The Nyquist mask depends on pitch, not on network evolution. Cache the exact
    // values while pitch is unchanged, and invalidate for bends/legato glides.
    const auto pitchBits = std::bit_cast<std::uint64_t>(fundamental);
    if (pitchBits != voice.bandPitchBits)
    {
        for (size_t i = 0; i < voice.referenceBands.size(); ++i)
            voice.referenceBands[i] = std::clamp((rate * 0.47 - fundamental * static_cast<double>(2 * i + 1)) / (rate * 0.08), 0.0, 1.0);
        voice.bandPitchBits = pitchBits;
    }
    double reference = Split?splitReference(voice):0.0, deformed = 0.0;
    for (size_t i = 0; i < partialAmplitudes.size(); ++i)
    {
        const auto harmonic = static_cast<double>(2 * i + 1);
        const auto frequency = fundamental * partialRatios[i];
        const auto referenceBand = voice.referenceBands[i];
        if (!Split&&referenceBand != 0.0)
            reference += partialAmplitudes[i] * std::sin(voice.phase * harmonic) * referenceBand;
        if (spectralActive)
        {
            const auto band = std::clamp((rate * 0.47 - frequency) / (rate * 0.08), 0.0, 1.0);
            deformed += partialAmplitudes[i] * std::sin(voice.partialPhases[i]) * band;
            voice.partialPhases[i] = wrap(voice.partialPhases[i] + twoPi * frequency / internalRate);
        }
        else voice.partialPhases[i] = wrap(voice.phase * harmonic + twoPi * frequency / internalRate);
    }
    const auto body = modeWeights[1] > 0.00000001 ? bodySample(voice, env, fundamental) : 0.0;
    const auto interaction = modeWeights[3] > 0.00000001 ? interactionSample(voice, env, fundamental) : 0.0;
    const auto phaseDelay = modeWeights[4] > 0.00000001 ? phaseDelaySample(voice, env, fundamental) : 0.0;
    const auto noteDelay = modeWeights[5] > 0.00000001 ? noteCoupledDelaySample(voice, env, fundamental) : 0.0;
    const auto network = modeWeights[6] > 0.00000001
        ? networkSample<Split>(voice, voice.network, env, fundamental, amount, 0.0, networkLearning) : 0.0;
    const auto extendedAmount = std::min(1.0, amount * 2.0);
    const auto stress = std::max(0.0, amount * 2.0 - 1.0);
    const auto drivenNetwork = modeWeights[7] > 0.00000001
        ? networkSample<Split>(voice, voice.stressedNetwork, env, fundamental, extendedAmount, stress, networkLearning) : 0.0;
    const auto expressive = modeWeights[8] > 0.00000001
        ? networkSample<Split>(voice, voice.expressiveNetwork, env, fundamental, couplingDimension, stressDimension,
                         expressiveLearning, expressivePhaseSpeed) : 0.0;
    const auto bodySound = reference * (1.0 - amount) + amount * body;
    // Blend back to the source rather than forcibly resetting accumulated partial phases.
    // This gives zero amount a neutral result even after a sweep, without a phase discontinuity.
    const auto spectralSound = reference * (1.0 - amount) + deformed * amount;
    const auto interactionSound = reference * (1.0 - amount) + amount * interaction;
    const auto phaseDelaySound = reference * (1.0 - amount) + amount * phaseDelay;
    const auto noteDelaySound = reference * (1.0 - amount) + amount * noteDelay;
    const auto networkSound = reference * (1.0 - amount) + amount * network;
    const auto drivenNetworkSound = reference * (1.0 - extendedAmount) + extendedAmount * drivenNetwork;
    const auto presence = std::max(couplingDimension, stressDimension * 0.5);
    const auto expressiveSound = reference * (1.0 - presence) + presence * expressive;
    voice.phase = wrap(voice.phase + twoPi * fundamental / internalRate);
    if constexpr(Split)voice.modulatorPhase=wrap(voice.modulatorPhase+twoPi*voice.modulatorHz/internalRate);
    auto sample = (reference * modeWeights[0] + bodySound * modeWeights[1] + spectralSound * modeWeights[2]
        + interactionSound * modeWeights[3] + phaseDelaySound * modeWeights[4] + noteDelaySound * modeWeights[5]
        + networkSound * modeWeights[6] + drivenNetworkSound * modeWeights[7] + expressiveSound * modeWeights[8])
        * env * voice.velocity;
    if (!std::isfinite(sample) || !std::isfinite(voice.bodySignal) || !std::isfinite(voice.interactionSignal))
    {
        ++numericalFaults;
        voice = {};
        return 0.0;
    }
    if(voice.stealSamples>0){sample+=voice.stolenSample*static_cast<double>(voice.stealSamples--)/voice.stealLength;}
    if(pitchOverride&&(pitchSettings.glide.enabled||pitchSettings.autobend.enabled))voice.lastSample=sample;
    return sample;
}

void Engine::render(float* left, float* right, int samples,double* tonePitch) noexcept
{
    for (int sample = 0; sample < samples; ++sample)
    {
        schedule();
        const auto target = playing && pattern == Pattern::sweep && amountSweepEnabled
            ? static_cast<double>(sweepAt(static_cast<double>(clock % loopSamples) / rate)) : targetAmount;
        smoothedAmount += smoothing * (target - smoothedAmount);
        if (modeWeights[8] > 0.00000001 || treatment == Treatment::expressiveNetwork)
        {
            couplingDimension += smoothing * (targetCoupling - couplingDimension);
            stressDimension += smoothing * (targetStress - stressDimension);
            const auto previousResponse = responseDimension;
            responseDimension += smoothing * (targetResponse - responseDimension);
            if (std::bit_cast<std::uint64_t>(previousResponse) != std::bit_cast<std::uint64_t>(responseDimension))
                updateDimensionRates();
        }
        sourceBlend += smoothing * ((source == Source::rich ? 1.0 : 0.0) - sourceBlend);
        if (indexOverride) currentIndex += smoothing * (targetIndex - currentIndex);
        else currentIndex = sourceIndex(Source::simple) + sourceBlend * (sourceIndex(Source::rich) - sourceIndex(Source::simple));
        if (envelopeOverride)
            for (size_t i = 0; i < envelopeValues.size(); ++i) envelopeValues[i] += smoothing * (envelopeTarget[i] - envelopeValues[i]);
        const auto previousBend = smoothedBend;
        smoothedBend += smoothing * (targetBend - smoothedBend);
        if (std::bit_cast<std::uint64_t>(previousBend) != std::bit_cast<std::uint64_t>(smoothedBend))
            pitchMultiplier = std::exp2(smoothedBend / 12.0);
        for (size_t i = 0; i < partialRatios.size(); ++i)
        {
            partialRatios[i] = static_cast<double>(2 * i + 1) + smoothedAmount * partialSpreads[i];
            if (indexOverride)
            {
                const auto position = currentIndex * 32;
                const auto lower = std::min(static_cast<size_t>(position), indexAmplitudes.size() - 2);
                const auto blend = position - static_cast<double>(lower);
                partialAmplitudes[i] = indexAmplitudes[lower][i] * (1.0 - blend) + indexAmplitudes[lower + 1][i] * blend;
            }
            else partialAmplitudes[i] = sourceAmplitudes[0][i] * (1.0 - sourceBlend) + sourceAmplitudes[1][i] * sourceBlend;
        }
        // Legacy body models only consume damping while their crossfade is active.
        // Include the selected treatment so its first fade-in sample is initialized.
        if (modeWeights[1] > 0.00000001 || treatment == Treatment::body)
            for (size_t i = 0; i < bodyDamping.size(); ++i)
                bodyDamping[i] = std::exp(-(7.0 + static_cast<double>(i) * 3.0 + smoothedAmount * 4.0) / internalRate);
        if (modeWeights[3] > 0.00000001 || treatment == Treatment::interaction)
            for (size_t i = 0; i < interactionDamping.size(); ++i)
                interactionDamping[i] = std::exp(-(1.8 + static_cast<double>(i) * 2.0 + smoothedAmount * 1.5) / internalRate);
        for (auto& voice : voices)
            if (!pitchOverride&&voice.active && voice.articulation == Articulation::lead)
            {
                const auto targetFrequency = 440.0 * std::exp2(static_cast<double>(voice.note - 69) / 12.0);
                voice.frequency += smoothing * (targetFrequency - voice.frequency);
            }
        bool split=false;const Voice* latest=nullptr;
        if(pitchOverride)for(auto& voice:voices)if(voice.active)
        {
            updateVoicePitch(voice);split|=voice.splitPhase;
            if(!latest||voice.serial>latest->serial)latest=&voice;
        }
        if(latest)lastNoteHz=latest->frequency;
        if(tonePitch)tonePitch[sample]=latest?latest->tonePitch:0;
        if(split)for(size_t n=0;n<rawBessel.size();++n)
        {
            if(indexOverride)
            {
                const auto position=currentIndex*32;const auto lower=std::min(static_cast<size_t>(position),indexBessel.size()-2);const auto blend=position-static_cast<double>(lower);
                rawBessel[n]=indexBessel[lower][n]*(1-blend)+indexBessel[lower+1][n]*blend;
            }
            else rawBessel[n]=sourceBessel[0][n]*(1-sourceBlend)+sourceBessel[1][n]*sourceBlend;
        }
        for (size_t i = 0; i < modeWeights.size(); ++i)
            modeWeights[i] += smoothing * ((i == static_cast<size_t>(treatment) ? 1.0 : 0.0) - modeWeights[i]);
        double output = 0.0;
        for (int step = 0; step < oversampling; ++step)
        {
            double sum = 0.0;
            for (auto& voice : voices) if (voice.active) sum += voice.splitPhase?voiceSample<true>(voice):voiceSample<false>(voice);
            for (auto& pole : downsampleFilter)
            {
                pole += decimationCoefficient * (sum - pole);
                sum = pole;
            }
            output = sum;
        }
        // Conservative fixed listening level; identical soft ceiling for all candidates.
        const auto result = static_cast<float>(0.8 * std::tanh(output * 0.16 / 0.8));
        left[sample] = result;
        right[sample] = result;
        if (playing) ++clock;
    }
}
}
