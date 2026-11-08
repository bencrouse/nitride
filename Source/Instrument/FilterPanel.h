#pragma once
#include "SupportingControls.h"
#include "InstrumentSession.h"

namespace Nitride
{
class FilterPanel final:public juce::Component
{
public:
    explicit FilterPanel(InstrumentSession&);
    ~FilterPanel() override;
    void sync(const Patch&,std::uint64_t);
    void finishEdits();
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    std::function<void()> onClose;
private:
    void choose(Parameter,double);
    InstrumentSession& session;
    juce::ComboBox family,character;
    UI::ValueControl amount{"Rage",0,1,0};
    juce::TextButton close{"CLOSE"};
    std::uint64_t epoch=0;
    bool editing=false,rage=false;
};
}
