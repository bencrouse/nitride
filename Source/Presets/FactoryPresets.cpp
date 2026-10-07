#include "FactoryPresets.h"

const std::array<FactoryPresets::Preset, 4>& FactoryPresets::getPresets()
{
    // Parameter studies only. These become sound presets when the DSP is implemented.
    static const std::array<Preset, 4> presets {{
        { "01 / Soft machinery", {} },
        { "02 / Glass animal", {
            { "algorithm", 1 }, { "op2_ratio", 3.5f }, { "op3_ratio", 7 }, { "op4_ratio", 11.25f },
            { "op5_ratio", 2.5f }, { "op6_ratio", 13 }, { "op2_level", 82 }, { "op3_level", 67 },
            { "op1_feedback", 34 }, { "op1_attack", 2 }, { "op1_decay", 460 }, { "op1_sustain", 12 },
            { "op1_release", 760 }, { "cutoff", 15800 }, { "resonance", 38 }, { "drive", 41 },
            { "space", 21 }, { "rupture_depth", 88 }, { "rupture_time", 1 }, { "rupture_scatter", 72 }
        } },
        { "03 / Slow apparition", {
            { "algorithm", 2 }, { "op1_attack", 1200 }, { "op1_decay", 2800 }, { "op1_sustain", 80 },
            { "op1_release", 5400 }, { "op1_feedback", 3 }, { "op2_ratio", 1.25f }, { "op3_ratio", 0.5f },
            { "op3_level", 74 }, { "op5_ratio", 2 }, { "op5_level", 65 }, { "cutoff", 4600 },
            { "resonance", 12 }, { "drive", 6 }, { "lfo_rate", 0.1f }, { "lfo_depth", 38 },
            { "space", 76 }, { "echo", 34 }, { "rupture_mode", 2 }, { "rupture_depth", 46 },
            { "rupture_time", 8 }, { "rupture_scatter", 18 }
        } },
        { "04 / Pressure bloom", {
            { "op1_ratio", 0.5f }, { "op2_ratio", 1 }, { "op2_level", 92 }, { "op3_ratio", 1.5f },
            { "op3_level", 78 }, { "op1_feedback", 62 }, { "op1_attack", 18 }, { "op1_sustain", 45 },
            { "op1_release", 2400 }, { "op4_ratio", 0.5f }, { "cutoff", 3200 }, { "resonance", 62 },
            { "drive", 68 }, { "lfo_depth", 46 }, { "space", 48 }, { "echo", 28 },
            { "rupture_mode", 1 }, { "rupture_depth", 82 }, { "rupture_time", 4 }, { "rupture_scatter", 42 }
        } }
    }};
    return presets;
}
