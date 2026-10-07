#pragma once

#include "PluginProcessor.h"
#include "Instrument/InstrumentView.h"

class NitrideAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit NitrideAudioProcessorEditor(NitrideAudioProcessor&);
    ~NitrideAudioProcessorEditor() override = default;
    void resized() override;

private:
    Nitride::InstrumentView view;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NitrideAudioProcessorEditor)
};
