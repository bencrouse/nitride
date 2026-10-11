#pragma once
#include <array>

namespace Nitride::DSP
{
struct ModulationFrame {double pitch=0,fm=0,coupling=0,stress=0;};
struct NetworkSnapshot
{
    std::array<float,6> offsets{};
    std::array<float,9> links{};
    float level=0,coherence=1;
    int activeVoices=0;
};
struct PitchSnapshot {int note=-1;double carrierHz=0,modulatorHz=0,glideHz=0,autobendSemitones=0;};
}
