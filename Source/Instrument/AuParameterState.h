#pragma once
#include "Parameters.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace Nitride
{
// Pinned AudioUnitSDK SaveState layout: big-endian scope/element/count followed
// by ID/float pairs. Only the exact known global table is adapted. JUCE restores
// our full document, not this redundant table. Unknown layouts remain untouched.
inline std::optional<std::vector<std::uint8_t>> legacyAuParameterData(const std::uint8_t* bytes,size_t length)
{
    if(length!=12+hostParameterCount*8)return std::nullopt;
    const auto word=[&](size_t at){return (std::uint32_t(bytes[at])<<24)|(std::uint32_t(bytes[at+1])<<16)|(std::uint32_t(bytes[at+2])<<8)|bytes[at+3];};
    if(word(0)!=0||word(4)!=0||word(8)!=hostParameterCount)return std::nullopt;
    const auto hash=[](const char* text){std::uint32_t value=0;while(*text)value=31*value+static_cast<unsigned char>(*text++);return value&0x7fffffffu;};
    static_assert(hostParameterCount==92,"Review the legacy AU adapter when adding controls");
    std::array<bool,hostParameterCount> seen{};std::vector<std::uint8_t> result(bytes,bytes+12);
    for(size_t at=12;at<length;at+=8)
    {
        int host=-1;
        for(int h=0;h<hostParameterCount;++h)if(word(at)==hash(h==monoHostIndex?monoParameterId:hostParameters[conditionIndex(h)].id))
        {
            host=h;break;
        }
        if(host<0||seen[static_cast<size_t>(host)])return std::nullopt;
        seen[static_cast<size_t>(host)]=true;
        if(host>=v6HostParameterCount){if(word(at+4)!=0)return std::nullopt;}
        else result.insert(result.end(),bytes+at,bytes+at+8);
    }
    result[8]=result[9]=result[10]=0;result[11]=v6HostParameterCount;return result;
}
}
