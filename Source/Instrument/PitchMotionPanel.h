#pragma once

#include "InstrumentSession.h"

namespace Nitride
{
class PitchMotionPanel final : public juce::Component
{
public:
    PitchMotionPanel(InstrumentSession&,bool autobend);
    ~PitchMotionPanel() override;
    void sync(const Patch&,std::uint64_t restoreEpoch);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void configureSlider(juce::Slider&,Parameter,bool time);
    void configureChoice(juce::ComboBox&,Parameter,const juce::StringArray&);
    void set(Parameter,double);
    InstrumentSession& session;
    bool autoMode,updating=false;
    std::uint64_t epoch=0;
    std::array<bool,2> dragging{},cancelled{};
    juce::TextButton enabled;
    juce::ComboBox destination,trigger,curve;
    juce::Slider time,depth;
};
}
