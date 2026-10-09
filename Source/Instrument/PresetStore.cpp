#include "PresetStore.h"
#include "HostParameters.h"

namespace Nitride
{
juce::File PresetStore::defaultDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Application Support/nitride/Presets");
}

juce::var PresetStore::encode(const StoredPreset& preset)
{
    auto* object = new juce::DynamicObject();
    object->setProperty("format", "nitride-preset"); object->setProperty("version", 2);
    object->setProperty("id", preset.id); object->setProperty("name", preset.patch.name);
    object->setProperty("mono", preset.patch.mono); object->setProperty("chord", preset.patch.chord);
    auto* values = new juce::DynamicObject();
    for (size_t i = 0; i < parameterCount; ++i) values->setProperty(hostParameters[i].id, preset.patch.values[i]);
    object->setProperty("values", juce::var(values));
    return juce::var(object);
}

std::optional<StoredPreset> PresetStore::decode(const juce::var& data)
{
    const auto version=static_cast<int>(data["version"]);
    if (!data.isObject() || data["format"].toString() != "nitride-preset" || (version!=1&&version!=2)) return std::nullopt;
    StoredPreset preset;
    preset.id = data["id"].toString(); preset.patch.name = data["name"].toString().trim();
    if (preset.id.isEmpty() || preset.id.length() > 128 || preset.patch.name.isEmpty() || preset.patch.name.length() > 256) return std::nullopt;
    if (!data["mono"].isBool() || !data["chord"].isBool()) return std::nullopt;
    preset.patch.mono = static_cast<bool>(data["mono"]); preset.patch.chord = static_cast<bool>(data["chord"]);
    const auto values = data["values"];
    if (!values.isObject()) return std::nullopt;
    const auto count=version==1?legacyParameterCount:parameterCount;
    for (size_t i = 0; i < static_cast<size_t>(count); ++i)
    {
        const auto value = values[hostParameters[i].id];
        if (!value.isDouble() && !value.isInt() && !value.isInt64()) return std::nullopt;
        const auto number = static_cast<double>(value);
        if (!std::isfinite(number) || number < hostParameters[i].minimum - 1.0e-6 || number > hostParameters[i].maximum + 1.0e-6) return std::nullopt;
        if(hostParameters[i].kind!=ParameterKind::continuous&&std::abs(number-std::round(number))>1.0e-6)return std::nullopt;
        preset.patch.values[i] = InstrumentSession::clamp(static_cast<Parameter>(i), number);
    }
    if(version==1&&preset.patch.mono){preset.patch.values[glideOn]=1;preset.patch.values[glideTime]=.025;preset.patch.values[glideCurve]=2;}
    return preset;
}

juce::Result PresetStore::exportFile(const juce::File& file, const StoredPreset& preset)
{
    const auto valid = decode(encode(preset));
    if (!valid) return juce::Result::fail("This sound cannot be saved: invalid preset conditions.");
    if (!file.getParentDirectory().createDirectory()) return juce::Result::fail("Cannot create the preset folder.");
    juce::TemporaryFile temporary(file);
    if (!temporary.getFile().replaceWithText(juce::JSON::toString(encode(*valid)))) return juce::Result::fail("Cannot write the preset file.");
    if (!temporary.overwriteTargetFileWithTemporary()) return juce::Result::fail("Cannot replace the preset file.");
    return juce::Result::ok();
}

juce::Result PresetStore::save(const StoredPreset& preset) const
{
    // File names are generated UUIDs, never user-supplied names or imported paths.
    const auto id = juce::Uuid(preset.id);
    if (id.isNull() || id.toString() != preset.id) return juce::Result::fail("Invalid preset identifier.");
    return exportFile(folder.getChildFile(preset.id + ".nitridepreset"), preset);
}

std::optional<StoredPreset> PresetStore::importFile(const juce::File& file)
{
    if (!file.existsAsFile() || file.getSize() > 65536) return std::nullopt;
    return decode(juce::JSON::parse(file.loadFileAsString()));
}

std::vector<StoredPreset> PresetStore::loadAll() const
{
    std::vector<StoredPreset> presets;
    for (const auto& file : folder.findChildFiles(juce::File::findFiles, false, "*.nitridepreset"))
        if (auto preset = importFile(file)) presets.push_back(std::move(*preset));
    std::sort(presets.begin(), presets.end(), [](const auto& a, const auto& b) { return a.patch.name.compareNatural(b.patch.name) < 0; });
    return presets;
}
}
