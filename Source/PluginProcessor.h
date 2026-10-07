#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Instrument/InstrumentRenderer.h"

class NitrideAudioProcessor final : public juce::AudioProcessor
{
public:
    NitrideAudioProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 14.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return session.program.load(); }
    void setCurrentProgram(int index) override { applyPreset(index); }
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    Nitride::InstrumentSession& getInstrumentSession() noexcept { return session; }
    void applyPreset(int);

private:
    Nitride::InstrumentSession session;
    Nitride::InstrumentRenderer renderer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NitrideAudioProcessor)
};
