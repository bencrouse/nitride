#include "PluginEditor.h"

NitrideAudioProcessorEditor::NitrideAudioProcessorEditor(NitrideAudioProcessor& owner)
    : AudioProcessorEditor(owner),view(owner.getInstrumentSession())
{
    addAndMakeVisible(view);
    setSize(1200,840);
}

void NitrideAudioProcessorEditor::resized() { view.setBounds(getLocalBounds()); }
