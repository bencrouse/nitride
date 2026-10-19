#include "InstrumentSession.h"
#include "PresetStore.h"
#include "NitrideReviewData.h"
#include <stdexcept>

namespace Nitride
{
namespace
{
juce::var resource(const juce::String& filename)
{
    for(int i=0;i<NitrideReviewData::namedResourceListSize;++i)
        if(filename==NitrideReviewData::originalFilenames[i])
        {
            int size=0;const auto* data=NitrideReviewData::getNamedResource(NitrideReviewData::namedResourceList[i],size);
            return juce::JSON::parse(juce::String::fromUTF8(data,size));
        }
    throw std::runtime_error("Missing embedded review resource: "+filename.toStdString());
}
}
const std::vector<StoredPreset>& builtInPresets()
{
    static const auto presets=[]
    {
        std::vector<StoredPreset> result;
        for(size_t i=0;i<factoryPatches().size();++i)result.push_back({"factory:"+juce::String(static_cast<int>(i)),factoryPatches()[i]});
        const auto manifest=resource("builtins.json");const auto* entries=manifest["entries"].getArray();
        if(manifest["format"].toString()!="nitride-review-builtins"||static_cast<int>(manifest["version"])!=1||!entries)throw std::runtime_error("Invalid embedded review catalog");
        for(const auto& entry:*entries)
        {
            auto preset=PresetStore::decode(resource(entry["preset"].toString().fromLastOccurrenceOf("/",false,false)));
            if(!preset||static_cast<int>(entry["program"])!=static_cast<int>(result.size()))throw std::runtime_error("Invalid embedded review preset/program");
            preset->id=entry["id"].toString();result.push_back(*preset);
        }
        return result;
    }();
    return presets;
}
int builtInIndex(const juce::String& id)
{
    const auto& presets=builtInPresets();
    for(size_t i=0;i<presets.size();++i)if(presets[i].id==id)return static_cast<int>(i);
    return -1;
}
}
