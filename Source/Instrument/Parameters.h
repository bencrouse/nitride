#pragma once

#include <array>
#include <cmath>
#include <algorithm>

namespace Nitride
{
enum Parameter
{
    coupling, stress, response, fm, pitch, attack, decay, sustain, release,
    cutoff, resonance, motionRate, motionDepth, spaceMix, spaceSize, output,
    glideOn, glideTime, glideTarget, glideLegato, glideCurve,
    autobendOn, autobendTime, autobendDepth, autobendTarget, autobendPhrase, legato,
    parameterCount
};
inline constexpr int legacyParameterCount = 16;
inline constexpr int monoHostIndex = 16;
inline constexpr int hostParameterCount = parameterCount + 1;
constexpr int hostIndex(Parameter p) noexcept { return p < legacyParameterCount ? static_cast<int>(p) : static_cast<int>(p) + 1; }
constexpr Parameter conditionIndex(int host) noexcept { return static_cast<Parameter>(host < monoHostIndex ? host : host - 1); }

enum class ParameterKind { continuous, boolean, choice };
struct HostParameterDefinition
{
    const char* id;
    const char* name;
    double minimum, maximum, skew, initial;
    ParameterKind kind = ParameterKind::continuous;
    const char* choices = "";
};
// First 16 float IDs followed by mono retain their version-1 host positions.
// New controls are appended AFTER mono, with version hint 2.
inline constexpr std::array<HostParameterDefinition,parameterCount> hostParameters {{
    {"coupling","Coupling",0,1,1,.55}, {"stress","Stress",0,1,1,.08},
    {"response","Response",.01,1.6,.35,.35}, {"fm_depth","FM depth",0,4,1,1.65},
    {"pitch","Pitch",-12,12,1,0}, {"amp_attack","Amplitude attack",.001,4,.3,.65},
    {"amp_decay","Amplitude decay",.01,4,.35,.7}, {"amp_sustain","Amplitude sustain",0,1,1,.72},
    {"amp_release","Amplitude release",.02,8,.35,1.4}, {"cutoff","Tone cutoff",40,18000,.3,5500},
    {"resonance","Tone resonance",0,.85,1,.18}, {"motion_rate","Motion rate",.05,8,.4,.25},
    {"motion_depth","Motion depth",0,1,1,.08}, {"space_mix","Space mix",0,.65,1,.25},
    {"space_size","Space size",0,1,1,.7}, {"output","Output",-24,0,1,-6},
    {"glide_enabled","Glide",0,1,1,0,ParameterKind::boolean},
    {"glide_time","Glide time",.001,5,.3,.15},
    {"glide_target","Glide destination",0,4,1,2,ParameterKind::choice,"Carrier|Modulators|All|Opposed|Pitch + Tone"},
    {"glide_legato","Glide legato only",0,1,1,0,ParameterKind::boolean},
    {"glide_curve","Glide curve",0,2,1,0,ParameterKind::choice,"Smooth|Linear|Classic"},
    {"autobend_enabled","Autobend",0,1,1,0,ParameterKind::boolean},
    {"autobend_time","Autobend time",.001,5,.3,.18},
    {"autobend_depth","Autobend depth",-36,36,1,12},
    {"autobend_target","Autobend destination",0,4,1,2,ParameterKind::choice,"Carrier|Modulators|All|Opposed|Pitch + Tone"},
    {"autobend_phrase","Autobend phrase only",0,1,1,0,ParameterKind::boolean},
    {"legato","Legato envelopes",0,1,1,0,ParameterKind::boolean}
}};
inline constexpr const char* monoParameterId = "mono";
constexpr std::array<double,parameterCount> defaultParameterValues()
{
    std::array<double,parameterCount> result{};
    for(size_t i=0;i<result.size();++i)result[i]=hostParameters[i].initial;
    return result;
}
inline double clampParameter(Parameter p,double value) noexcept
{
    const auto& definition=hostParameters[static_cast<size_t>(p)];
    if(!std::isfinite(value))return definition.initial;
    value=std::clamp(value,definition.minimum,definition.maximum);
    return definition.kind==ParameterKind::continuous?value:std::round(value);
}
}
