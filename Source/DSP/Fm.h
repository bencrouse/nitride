#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Nitride::DSP
{
enum class FmShape { sine,triangle,saturated };
enum class FmSource { off,lfo,amplitude,filter,network };
enum class FmTarget { depth,ratio,shape,mix };
struct FmRoute {FmSource source=FmSource::off;FmTarget target=FmTarget::depth;double amount=0;};
struct FmSettings
{
    double ratio=2,shapeAmount=0,mix=0,envelopeAmount=0;
    FmShape shape=FmShape::sine;
    std::array<double,4> envelope{.01,.3,.5,.4};
    std::array<FmRoute,3> routes{};
};
struct FmValues {double index=0,ratio=2,shape=0,mix=0;};
inline FmValues fmDestinations(double index,double ratio,double shape,double mix,const std::array<double,4>& offsets) noexcept
{
    return {std::clamp(index+offsets[0],0.0,4.0),std::clamp(ratio*std::exp2(std::clamp(offsets[1],-12.0,12.0)),.25,8.0),
        std::clamp(shape+offsets[2],0.0,1.0),std::clamp(mix+offsets[3],0.0,1.0)};
}
inline FmValues combineFm(const FmSettings& s,double index,double envelope,const std::array<double,5>& sources) noexcept
{
    std::array<double,4> offsets{s.envelopeAmount*envelope,0,0,0};
    for(const auto& route:s.routes)offsets[static_cast<size_t>(route.target)]+=route.amount*sources[static_cast<size_t>(route.source)];
    return fmDestinations(index,s.ratio,s.shapeAmount,s.mix,offsets);
}
// A finite harmonic triangle and a smooth cubic saturator avoid discontinuous
// modulators. The same coefficients drive the reference and primary network node.
inline std::array<double,5> fmHarmonics(FmShape shape,double amount) noexcept
{
    std::array<double,5> result{1,0,0,0,0};
    if(shape==FmShape::sine)return result;
    std::array<double,5> shaped{};
    if(shape==FmShape::triangle)
    {
        double peak=0;for(size_t k=0;k<shaped.size();++k){const auto h=static_cast<double>(2*k+1);peak+=1/(h*h);shaped[k]=(k%2==0?1:-1)/(h*h);}
        for(auto& value:shaped)value/=peak;
    }
    else shaped={1.125,.125,0,0,0}; // 1.5 sin(p) - 0.5 sin(p)^3
    for(size_t k=0;k<result.size();++k)result[k]=result[k]*(1-amount)+shaped[k]*amount;
    return result;
}
inline double fmWaveFromSine(double sine,const std::array<double,5>& harmonics) noexcept
{
    // sin((h+2)p) = 2 cos(2p) sin(hp) - sin((h-2)p). Derive
    // cos(2p) from sin(p): one transcendental for all five odd harmonics.
    auto previous=-sine;const auto step=2*(1-2*sine*sine);double result=0;
    for(const auto coefficient:harmonics){result+=coefficient*sine;const auto next=step*sine-previous;previous=sine;sine=next;}
    return result;
}
inline double fmWave(double phase,const std::array<double,5>& harmonics) noexcept
{return fmWaveFromSine(std::sin(phase),harmonics);}
inline constexpr size_t fmSidebands=25;
using FmCoefficients=std::array<double,fmSidebands>;
// Constructed once by prepare(), never lazily from a render callback.
class FmTables
{
public:
    FmTables();
    FmCoefficients coefficients(FmShape,double amount,double index) const noexcept;
private:
    std::array<std::array<std::array<FmCoefficients,129>,17>,3> table{};
};
const FmTables& fmTables();
}
