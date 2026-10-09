#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "../DSP/PitchMotion.h"

namespace SoundStudies
{
enum class Treatment { fm, body, spectral, interaction, phaseDelay, noteCoupledDelay, adaptiveNetwork, stressedNetwork, expressiveNetwork };
enum class Pattern { pad, hits, velocity, sweep, lead };
enum class Source { simple, rich };
enum class Articulation { pad, hit, lead };

// Independent of the plugin and JUCE: shared FM sources and isolated listening hypotheses.
class Engine
{
public:
    struct NetworkSnapshot
    {
        std::array<float, 6> offsets {};
        std::array<float, 9> links {};
        float level = 0.0f, coherence = 1.0f;
        int activeVoices = 0;
    };
    void prepare(double sampleRate);
    void setTreatment(Treatment mode) noexcept { treatment = mode; }
    void setSource(Source selected) noexcept { source = selected; }
    void setNetworkAdaptive(bool enabled) noexcept { networkAdaptive = enabled; }
    void setAmountSweepEnabled(bool enabled) noexcept { amountSweepEnabled = enabled; }
    void setAmount(float value) noexcept;
    void setPitchBend(float semitones) noexcept;
    void setNetworkDimensions(float coupling, float stress, double responseSeconds) noexcept;
    void setFmIndex(double index) noexcept;
    void setAmplitudeEnvelope(double attack, double decay, float sustain, double release) noexcept;
    void setPitchMotion(const PitchSettings&) noexcept;
    void beginNoteGroup() noexcept;
    void start(Pattern pattern) noexcept;
    void stop() noexcept;
    void noteOn(int note, float velocity, Articulation articulation = Articulation::pad, bool overlapping = false, bool returning = false) noexcept;
    void noteOff(int note) noexcept;
    void allNotesOff() noexcept;
    void render(float* left, float* right, int samples, double* tonePitch = nullptr) noexcept;
    struct PitchSnapshot { int note=-1;double carrierHz=0,modulatorHz=0,glideHz=0,autobendSemitones=0; };
    PitchSnapshot getPitchSnapshot(int note=-1) const noexcept;
    float getAmount() const noexcept { return static_cast<float>(smoothedAmount); }
    NetworkSnapshot getNetworkSnapshot() const noexcept;
    std::uint64_t getNumericalFaults() const noexcept { return numericalFaults; }
    static double patternSeconds(Pattern) noexcept;
    static float sweepAt(double seconds) noexcept;
    static double bessel(int order, double index) noexcept;
    static double partialRatio(int partial, double amount) noexcept;
    static double sourceIndex(Source) noexcept;

private:
    struct Mode { double displacement = 0.0, velocity = 0.0; };
    struct DelayState
    {
        std::array<float, 4096> samples {};
        std::size_t cursor = 0;
        double signal = 0.0;
    };
    struct NetworkState
    {
        std::array<double, 6> offsets {};
        std::array<double, 9> links {};
        double impulse = 0.0, signal = 0.0;
    };
    struct Voice
    {
        bool active = false, released = false;
        Articulation articulation = Articulation::pad;
        int note = 60;
        double frequency = 261.625565;
        GlideMotion glide;
        AutobendMotion autobend;
        PitchRouting glideRouting,autobendRouting;
        double carrierHz=261.625565,modulatorHz=261.625565,modulatorPhase=0,tonePitch=0,autoOffset=0,autoDepth=0;
        double carrierBand=1;
        bool splitPhase=false;
        std::array<double,13> positiveBands{},negativeBands{};
        double previousCarrierHz=0,previousModulatorHz=0;
        double velocity = 0.7, envelope = 0.0;
        double targetVelocity=.7,velocityStep=0,lastSample=0,stolenSample=0;
        int velocitySamples=0,stealSamples=0,stealLength=1;
        double age = 0.0, releaseAge = 0.0, releaseLevel = 0.0;
        double attackStart = 0.0;
        double phase = 0.0, bodySignal = 0.0;
        double interactionSignal = 0.0, delaySignal = 0.0;
        double noteDelayPhase = 0.0;
        std::uint64_t bandPitchBits = 0;
        std::array<double, 12> referenceBands {};
        std::array<double, 12> partialPhases {};
        std::array<Mode, 4> modes {};
        std::array<Mode, 4> interactionModes {};
        std::array<float, 4096> delay {};
        std::size_t delayCursor = 0;
        DelayState noteDelay;
        NetworkState network, stressedNetwork, expressiveNetwork;
        std::uint64_t serial = 0;
    };

    void schedule() noexcept;
    // Compile constant-ratio and moving-ratio paths separately: inactive motion
    // must not add branches inside every partial and network oscillator evaluation.
    template<bool Split> double voiceSample(Voice&) noexcept;
    double envelope(Voice&) noexcept;
    double bodySample(Voice&, double env, double fundamental) noexcept;
    double interactionSample(Voice&, double env, double fundamental) noexcept;
    double phaseDelaySample(Voice&, double env, double fundamental) noexcept;
    double noteCoupledDelaySample(Voice&, double env, double fundamental) noexcept;
    template<bool Split> double networkSample(Voice&, NetworkState&, double env, double fundamental, double amount, double stress,
                         double learning, double phaseSpeed = 1.0) noexcept;
    void updateDimensionRates() noexcept;
    void updateVoicePitch(Voice&) noexcept;
    double splitReference(const Voice&) const noexcept;

    std::array<Voice, 12> voices {};
    std::array<double, 12> partialAmplitudes {};
    std::array<std::array<double, 12>, 2> sourceAmplitudes {};
    std::array<std::array<double, 12>, 129> indexAmplitudes {};
    std::array<std::array<double,13>,129> indexBessel{};
    std::array<std::array<double,13>,2> sourceBessel{};
    std::array<double,13> rawBessel{};
    std::array<double, 12> partialSpreads {}, partialRatios {};
    std::array<double, 4> bodyDamping {};
    std::array<double, 4> interactionDamping {};
    std::array<double, 9> modeWeights { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<double, 4> downsampleFilter {};
    Treatment treatment = Treatment::fm;
    Source source = Source::simple;
    Pattern pattern = Pattern::pad;
    double rate = 48000.0, internalRate = 192000.0;
    double targetAmount = 0.5, smoothedAmount = 0.5;
    double smoothing = 0.0, decimationCoefficient = 0.0;
    double sourceBlend = 0.0, currentIndex = 1.65;
    bool indexOverride = false, envelopeOverride = false;
    bool pitchOverride=false;
    PitchSettings pitchSettings;
    double groupOrigin=0,lastNoteHz=0,toneAnchorHz=0;
    double targetIndex = 2.7;
    std::array<double, 4> envelopeTarget { 0.006, 0.16, 0.62, 0.16 };
    std::array<double, 4> envelopeValues { 0.006, 0.16, 0.62, 0.16 };
    double targetBend = 0.0, smoothedBend = 0.0, pitchMultiplier = 1.0;
    double networkLearning = 0.0, networkImpulseDecay = 0.0;
    double targetCoupling = 0.65, targetStress = 0.2, targetResponse = 0.025;
    double couplingDimension = 0.65, stressDimension = 0.2, responseDimension = 0.025;
    double expressiveLearning = 0.0, expressiveImpulseDecay = 0.0, expressivePhaseSpeed = 1.0;
    bool networkAdaptive = true;
    bool amountSweepEnabled = true;
    bool playing = false;
    std::int64_t clock = 0, loopSamples = 384000;
    std::uint64_t nextSerial = 0, numericalFaults = 0;
};
}
