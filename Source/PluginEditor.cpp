#include "PluginEditor.h"

NitrideAudioProcessorEditor::NitrideAudioProcessorEditor(NitrideAudioProcessor& owner)
    : AudioProcessorEditor(owner),view(owner.getInstrumentSession(),{},juce::JUCEApplicationBase::isStandaloneApp()?Nitride::PresetPresentation::standalone:Nitride::PresetPresentation::host)
{
    addAndMakeVisible(view);
    setSize(view.getWidth(),view.getHeight());
}

void NitrideAudioProcessorEditor::resized() { view.setBounds(getLocalBounds()); }
