#pragma once
#include <algorithm>
#include <cmath>

namespace Nitride::DSP
{
inline constexpr int filterTrackingReference=60;
inline double filterNoteOctaves(int note,double tracking,double velocity,double sensitivity) noexcept
{
    return tracking*static_cast<double>(note-filterTrackingReference)/12+sensitivity*velocity;
}
// The base cutoff is smoothed multiplicatively (linearly in log frequency).
// Envelope, LFO, Pitch + Tone and note offsets are additive octave quantities.
// FM is the instantaneous pre-filter oscillator mix: never smooth this signal
// or the final frequency. Only automation of the depth itself is dezippered.
inline double modulatedCutoff(double base,double controlOctaves,double fmDepth,double oscillator,double sampleRate) noexcept
{
    const auto upper=sampleRate*.42,lower=std::min(10.0,upper);
    auto octaves=controlOctaves;
    if(fmDepth>0)octaves+=fmDepth*std::clamp(oscillator,-1.0,1.0);
    // Production offsets are bounded well inside this exponent range. The guard
    // also keeps extreme finite inputs from overflowing before the final clamp.
    const auto frequency=std::abs(octaves)>0?base*std::exp2(std::clamp(octaves,-128.0,128.0)):base;
    return std::clamp(frequency,lower,upper);
}
}
