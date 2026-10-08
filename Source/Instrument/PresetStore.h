#pragma once

#include "InstrumentSession.h"

namespace Nitride
{
class PresetStore
{
public:
    explicit PresetStore(juce::File directory = defaultDirectory()) : folder(std::move(directory)) {}
    static juce::File defaultDirectory();
    std::vector<StoredPreset> loadAll() const;
    juce::Result save(const StoredPreset&) const;
    static juce::var encode(const StoredPreset&);
    static std::optional<StoredPreset> decode(const juce::var&);
    static juce::Result exportFile(const juce::File&, const StoredPreset&);
    static std::optional<StoredPreset> importFile(const juce::File&);
    const juce::File& directory() const noexcept { return folder; }
private:
    juce::File folder;
};
}
