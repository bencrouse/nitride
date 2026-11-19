#pragma once
#include <array>
#include <cstdint>

namespace Nitride::DSP
{
enum class RippleType { subtle,medium,hard };
enum class RippleMode { waveshape,feedback };

// Independent same-frequency all-pass cascade, not an Arturia-identical model.
// Normalized scattering states preserve energy even as coefficients move.
class RippleFilter
{
public:
    void prepare(double rate,RippleType type=RippleType::subtle,RippleMode mode=RippleMode::waveshape) noexcept;
    void reset() noexcept;
    void configure(RippleType,RippleMode) noexcept;
    double process(double input,double frequency,double resonance,double dry,double drive,double feedback) noexcept;
    struct Diagnostics {std::uint64_t faults=0;double maximumResidual=0;};
    Diagnostics diagnostics()const noexcept{return diagnostic;}
    static constexpr int stages(RippleType type) noexcept{return 2<<static_cast<int>(type);}
    // Control-only post-sum calibration. Never follows the audio envelope or
    // compensates individual notches. The gain ceiling is not an audio limiter.
    static double makeupGain(RippleMode,double dry,double drive,double feedback) noexcept;
private:
    struct Section {double outer=0,inner=0;};
    struct Bank {std::array<Section,8> state{};};
    double cascade(Bank&,int count,double input) noexcept;
    std::array<Bank,6> banks{};
    std::array<double,6> weights{1,0,0,0,0,0},steps{};
    double sampleRate=192000,lastFrequency=-1,lastResonance=-1;
    double k1=0,t1=1,k2=0,t2=1;
    double lastDry=-1,lastDrive=-1,lastFeedback=2;
    std::array<double,2> makeup{1,1};
    int target=0,remaining=0,transitionLength=960;
    Diagnostics diagnostic;
};
}
