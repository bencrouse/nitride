#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Instrument/InstrumentRenderer.h"
#include "Instrument/HostParameters.h"

class NitrideAudioProcessor final : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit NitrideAudioProcessor(juce::File presetDirectory = {});
    ~NitrideAudioProcessor() override;
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
    juce::AudioProcessorValueTreeState& getState() noexcept { return apvts; }
    // Diagnostic access while processing is stopped, or from the render thread.
    SoundStudies::Engine::PitchSnapshot getPitchSnapshot(int note=-1) const noexcept { return renderer.pitchSnapshot(note); }

private:
    Nitride::InstrumentSession session;
    juce::AudioProcessorValueTreeState apvts;
    Nitride::InstrumentRenderer renderer;
    std::array<juce::RangedAudioParameter*,Nitride::hostParameterCount> hostBindings {};
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const juce::String&,float) override;
    juce::ValueTree parameterStateFor(const Nitride::Patch&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NitrideAudioProcessor)
};
