#pragma once

#include <array>
#include <vector>

namespace FactoryPresets
{
struct Value { const char* id; float value; };
struct Preset { const char* name; std::vector<Value> overrides; };
const std::array<Preset, 4>& getPresets();
}
