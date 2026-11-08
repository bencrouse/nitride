#pragma once
#include <array>
#include <cstdint>

namespace Nitride::DSP
{
enum class RageCharacter { diode,distortion,softclip,tape,transistor };

// Independent nonlinear-feedback model, not Arturia code or a circuit-identical
// emulation. A TPT resonator with a nonlinear band-to-low integrator return in
// parallel with its linear coupling; low feeds back into the resonator input.
class RageFilter
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setCharacter(RageCharacter) noexcept;
    double process(double input,double cutoff,double resonance,double amount) noexcept;
    struct Diagnostics {std::uint64_t faults=0,solverFallbacks=0;double maximumResidual=0;};
    Diagnostics diagnostics() const noexcept {return diagnostic;}
private:
    struct Shape {double value,derivative;};
    struct Colour {double value,derivative,primitive;};
    struct ReturnContext
    {
        double drive=1,previous=0,primitive=0,compression=1,excitationCompression=1;
        double sourceGain=1,sourceFeedback=0,sourceInput=0,sourceWet=0;
    };
    static Colour colour(RageCharacter,double value,bool primitive=true) noexcept;
    static Shape excitationShape(RageCharacter,double value) noexcept;
    Shape excitation(double input,double band,double amount) const noexcept;
    Shape feedback(double value,double previousState,double amount) const noexcept;
    Shape shape(RageCharacter,double value,double previousState) const noexcept;
    std::array<ReturnContext,5> returnContext{};
    double sampleRate=192000,coefficient=0,cachedCutoff=-1;
    double state1=0,state2=0,previousBand=0,tapeEnvelope=0;
    double tapeTone=0,distortionTone=0,transistorLow=0,transistorEnvelope=0;
    double tapeAlpha=0,distortionAlpha=0,transistorAlpha=0,transistorEnvelopeAlpha=0;
    double dcInput=0,dcOutput=0,dcPole=0,envelopeAttack=0,envelopeRelease=0;
    RageCharacter target=RageCharacter::diode;
    std::array<double,5> weights{1,0,0,0,0},weightStep{};
    int transition=0,transitionLength=1;
    Diagnostics diagnostic;
};
}
