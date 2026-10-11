#include "PluginEditor.h"

NitrideAudioProcessorEditor::NitrideAudioProcessorEditor(NitrideAudioProcessor& owner)
    : AudioProcessorEditor(owner),view(owner.getInstrumentSession())
{
    addAndMakeVisible(view);
    setSize(view.getWidth(),view.getHeight());
}

void NitrideAudioProcessorEditor::resized() { view.setBounds(getLocalBounds()); }
