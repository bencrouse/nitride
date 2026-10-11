#pragma once
#include "InstrumentSession.h"
#include "../DSP/Lfo.h"

namespace Nitride
{
class MotionPanel final : public juce::Component
{
public:
    explicit MotionPanel(InstrumentSession&);
    ~MotionPanel() override;
    void sync(const Patch&,std::uint64_t epoch);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    const LfoParameters& lane()const {return lfoParameters[selected];}
    void set(Parameter,double);
    void finishDrags();
    void slider(juce::Slider&,size_t);
    InstrumentSession& session;
    juce::ComboBox destination,wave,division;
    juce::TextButton enabled,syncButton;
    juce::Slider rate,depth,tempo;
    std::array<bool,3> dragging{},cancelled{};std::array<Parameter,3> edited{};
    Patch patch;size_t selected=4;std::uint64_t epoch=0;bool updating=false;
};
}
