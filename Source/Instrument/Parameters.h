#pragma once

#include <array>
#include <cmath>
#include <algorithm>

namespace Nitride
{
enum Parameter
{
    coupling, stress, response, fm, pitch, attack, decay, sustain, release,
    cutoff, resonance, couplingLfoRate, couplingLfoDepth, spaceMix, spaceSize, output,
    glideOn, glideTime, glideTarget, glideLegato, glideCurve,
    autobendOn, autobendTime, autobendDepth, autobendTarget, autobendPhrase, legato,
    filterAttack, filterDecay, filterSustain, filterRelease, filterAmount,
    couplingLfoOn,couplingLfoWave,couplingLfoSync,couplingLfoDivision,
    pitchLfoOn,pitchLfoWave,pitchLfoSync,pitchLfoDivision,pitchLfoRate,pitchLfoDepth,
    fmLfoOn,fmLfoWave,fmLfoSync,fmLfoDivision,fmLfoRate,fmLfoDepth,
    cutoffLfoOn,cutoffLfoWave,cutoffLfoSync,cutoffLfoDivision,cutoffLfoRate,cutoffLfoDepth,
    ampLfoOn,ampLfoWave,ampLfoSync,ampLfoDivision,ampLfoRate,ampLfoDepth,
    stressLfoOn,stressLfoWave,stressLfoSync,stressLfoDivision,stressLfoRate,stressLfoDepth,
    lfoTempo,
    filterKeyTracking,filterVelocity,filterFmDepth,
    fmRatio,fmShape,fmShapeAmount,fmMix,fmAttack,fmDecay,fmSustain,fmRelease,fmEnvelopeAmount,
    fmRoute1Source,fmRoute1Target,fmRoute1Amount,fmRoute2Source,fmRoute2Target,fmRoute2Amount,fmRoute3Source,fmRoute3Target,fmRoute3Amount,
    parameterCount
};
inline constexpr int legacyParameterCount = 16;
inline constexpr int pitchParameterCount = 27; // Preset v2 / 0.3.0 conditions.
inline constexpr int monoHostIndex = 16;
inline constexpr int hostParameterCount = parameterCount + 1;
constexpr int hostIndex(Parameter p) noexcept { return p < legacyParameterCount ? static_cast<int>(p) : static_cast<int>(p) + 1; }
constexpr Parameter conditionIndex(int host) noexcept { return static_cast<Parameter>(host < monoHostIndex ? host : host - 1); }
constexpr int versionHint(Parameter p) noexcept {return p<legacyParameterCount?1:p<pitchParameterCount?2:p<=filterAmount?3:p<=lfoTempo?4:p<=filterFmDepth?5:6;}

enum class ParameterKind { continuous, boolean, choice };
struct HostParameterDefinition
{
    const char* id;
    const char* name;
    double minimum, maximum, skew, initial;
    ParameterKind kind = ParameterKind::continuous;
    const char* choices = "";
};
// Explicit host mapping keeps mono separate from the continuous/choice bank.
// The coupling lane replaces Motion; there is no legacy Motion mode.
inline constexpr auto hostParameters=[] {
    std::array<HostParameterDefinition,parameterCount> definitions {{
    {"coupling","Coupling",0,1,1,.55}, {"stress","Stress",0,1,1,.08},
    {"response","Response",.01,1.6,.35,.35}, {"fm_depth","FM depth",0,4,1,1.65},
    {"pitch","Pitch",-12,12,1,0}, {"amp_attack","Amplitude attack",.001,4,.3,.65},
    {"amp_decay","Amplitude decay",.01,4,.35,.7}, {"amp_sustain","Amplitude sustain",0,1,1,.72},
    {"amp_release","Amplitude release",.02,8,.35,1.4}, {"cutoff","Tone cutoff",40,18000,.3,5500},
    {"resonance","Tone resonance",0,.85,1,.18}, {"lfo_coupling_rate","Coupling LFO rate",.01,20,.3,.25},
    {"lfo_coupling_depth","Coupling LFO depth",-1,1,1,0}, {"space_mix","Space mix",0,.65,1,.25},
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
    {"legato","Legato envelopes",0,1,1,0,ParameterKind::boolean},
    {"filter_attack","Filter attack",.001,4,.3,.01},
    {"filter_decay","Filter decay",.01,4,.35,.3},
    {"filter_sustain","Filter sustain",0,1,1,.5},
    {"filter_release","Filter release",.02,8,.35,.4},
    {"filter_amount","Filter envelope amount",-6,6,1,0}
    }};
    constexpr auto waves="Sine|Triangle|Sawtooth|Square|Sample & Hold";
    constexpr auto divisions="4 bars|2 bars|1 bar|1/1|1/1 dotted|1/1 triplet|1/2|1/2 dotted|1/2 triplet|1/4|1/4 dotted|1/4 triplet|1/8|1/8 dotted|1/8 triplet|1/16|1/16 dotted|1/16 triplet|1/32|1/32 dotted|1/32 triplet|1/64|1/64 dotted|1/64 triplet";
    definitions[couplingLfoOn]={"lfo_coupling_enabled","Coupling LFO",0,1,1,0,ParameterKind::boolean};
    definitions[couplingLfoWave]={"lfo_coupling_wave","Coupling LFO waveform",0,4,1,0,ParameterKind::choice,waves};
    definitions[couplingLfoSync]={"lfo_coupling_sync","Coupling LFO sync",0,1,1,0,ParameterKind::boolean};
    definitions[couplingLfoDivision]={"lfo_coupling_division","Coupling LFO division",0,23,1,9,ParameterKind::choice,divisions};
#define NITRIDE_LFO(prefix,label,on,wave,sync,division,rate,depth,maximum) \
    definitions[on]={"lfo_" prefix "_enabled",label " LFO",0,1,1,0,ParameterKind::boolean}; \
    definitions[wave]={"lfo_" prefix "_wave",label " LFO waveform",0,4,1,0,ParameterKind::choice,waves}; \
    definitions[sync]={"lfo_" prefix "_sync",label " LFO sync",0,1,1,0,ParameterKind::boolean}; \
    definitions[division]={"lfo_" prefix "_division",label " LFO division",0,23,1,9,ParameterKind::choice,divisions}; \
    definitions[rate]={"lfo_" prefix "_rate",label " LFO rate",.01,20,.3,.25}; \
    definitions[depth]={"lfo_" prefix "_depth",label " LFO depth",-maximum,maximum,1,0};
    NITRIDE_LFO("pitch","Pitch",pitchLfoOn,pitchLfoWave,pitchLfoSync,pitchLfoDivision,pitchLfoRate,pitchLfoDepth,24)
    NITRIDE_LFO("fm","FM",fmLfoOn,fmLfoWave,fmLfoSync,fmLfoDivision,fmLfoRate,fmLfoDepth,4)
    NITRIDE_LFO("cutoff","Cutoff",cutoffLfoOn,cutoffLfoWave,cutoffLfoSync,cutoffLfoDivision,cutoffLfoRate,cutoffLfoDepth,6)
    NITRIDE_LFO("amplitude","Amplitude",ampLfoOn,ampLfoWave,ampLfoSync,ampLfoDivision,ampLfoRate,ampLfoDepth,1)
    NITRIDE_LFO("stress","Stress",stressLfoOn,stressLfoWave,stressLfoSync,stressLfoDivision,stressLfoRate,stressLfoDepth,1)
#undef NITRIDE_LFO
    definitions[ampLfoDepth].minimum=0;
    definitions[lfoTempo]={"lfo_tempo","LFO fallback tempo",20,300,1,120};
    definitions[filterKeyTracking]={"filter_key_tracking","Filter key tracking",0,1,1,0};
    definitions[filterVelocity]={"filter_velocity","Filter velocity sensitivity",-6,6,1,0};
    definitions[filterFmDepth]={"filter_fm_depth","Filter FM depth",0,6,1,0};
    definitions[fmRatio]={"fm_ratio","FM ratio",.25,8,.5,2};
    definitions[fmShape]={"fm_shape","FM modulator shape",0,2,1,0,ParameterKind::choice,"Sine|Triangle|Soft saturated"};
    definitions[fmShapeAmount]={"fm_shape_amount","FM shape amount",0,1,1,0};
    definitions[fmMix]={"fm_operator_mix","FM operator mix",0,1,1,0};
    definitions[fmAttack]={"fm_attack","FM envelope attack",.001,4,.3,.01};
    definitions[fmDecay]={"fm_decay","FM envelope decay",.01,4,.3,.3};
    definitions[fmSustain]={"fm_sustain","FM envelope sustain",0,1,1,.5};
    definitions[fmRelease]={"fm_release","FM envelope release",.02,8,.3,.4};
    definitions[fmEnvelopeAmount]={"fm_envelope_amount","FM envelope amount",-4,4,1,0};
#define NITRIDE_FM_ROUTE(n,source,target,amount) \
    definitions[source]={"fm_route_" #n "_source","FM route " #n " source",0,4,1,0,ParameterKind::choice,"Off|FM LFO|Amplitude envelope|Filter envelope|Network activity"}; \
    definitions[target]={"fm_route_" #n "_target","FM route " #n " target",0,3,1,0,ParameterKind::choice,"Depth|Ratio|Shape|Operator mix"}; \
    definitions[amount]={"fm_route_" #n "_amount","FM route " #n " amount",-4,4,1,0};
    NITRIDE_FM_ROUTE(1,fmRoute1Source,fmRoute1Target,fmRoute1Amount)
    NITRIDE_FM_ROUTE(2,fmRoute2Source,fmRoute2Target,fmRoute2Amount)
    NITRIDE_FM_ROUTE(3,fmRoute3Source,fmRoute3Target,fmRoute3Amount)
#undef NITRIDE_FM_ROUTE
    return definitions;
}();
struct LfoParameters {Parameter enabled,wave,sync,division,rate,depth;const char* name;const char* unit;};
struct FmRouteParameters {Parameter source,target,amount;};
inline constexpr std::array<FmRouteParameters,3> fmRoutes{{{fmRoute1Source,fmRoute1Target,fmRoute1Amount},{fmRoute2Source,fmRoute2Target,fmRoute2Amount},{fmRoute3Source,fmRoute3Target,fmRoute3Amount}}};
inline constexpr std::array<LfoParameters,6> lfoParameters {{
    {pitchLfoOn,pitchLfoWave,pitchLfoSync,pitchLfoDivision,pitchLfoRate,pitchLfoDepth,"Pitch","st"},
    {fmLfoOn,fmLfoWave,fmLfoSync,fmLfoDivision,fmLfoRate,fmLfoDepth,"FM depth","index"},
    {cutoffLfoOn,cutoffLfoWave,cutoffLfoSync,cutoffLfoDivision,cutoffLfoRate,cutoffLfoDepth,"Filter cutoff","oct"},
    {ampLfoOn,ampLfoWave,ampLfoSync,ampLfoDivision,ampLfoRate,ampLfoDepth,"Amplitude","%"},
    {couplingLfoOn,couplingLfoWave,couplingLfoSync,couplingLfoDivision,couplingLfoRate,couplingLfoDepth,"Coupling","%"},
    {stressLfoOn,stressLfoWave,stressLfoSync,stressLfoDivision,stressLfoRate,stressLfoDepth,"Stress","%"}
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
