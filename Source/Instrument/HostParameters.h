#pragma once

#include "InstrumentSession.h"

namespace Nitride
{
struct HostParameterDefinition
{
    const char* id;
    const char* name;
    float minimum, maximum, skew;
};

// This order and these version-1 IDs are the automation contract. Do not rename
// IDs or repurpose parameters when changing presentation or adding controls.
inline constexpr std::array<HostParameterDefinition, parameterCount> hostParameters {{
    {"coupling", "Coupling", 0, 1, 1},
    {"stress", "Stress", 0, 1, 1},
    {"response", "Response", .01f, 1.6f, .35f},
    {"fm_depth", "FM depth", 0, 4, 1},
    {"pitch", "Pitch", -12, 12, 1},
    {"amp_attack", "Amplitude attack", .001f, 4, .3f},
    {"amp_decay", "Amplitude decay", .01f, 4, .35f},
    {"amp_sustain", "Amplitude sustain", 0, 1, 1},
    {"amp_release", "Amplitude release", .02f, 8, .35f},
    {"cutoff", "Tone cutoff", 40, 18000, .3f},
    {"resonance", "Tone resonance", 0, .85f, 1},
    {"motion_rate", "Motion rate", .05f, 8, .4f},
    {"motion_depth", "Motion depth", 0, 1, 1},
    {"space_mix", "Space mix", 0, .65f, 1},
    {"space_size", "Space size", 0, 1, 1},
    {"output", "Output", -24, 0, 1}
}};
inline constexpr const char* monoParameterId = "mono";
}
