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
    juce::ComboBox rippleTypeChoice,rippleModeChoice;
    UI::ValueControl amount{"Rage",0,1,0};
    UI::ValueControl dry{"Dry level",0,1,.5},drive{"Drive",0,1,0},feedback{"Feedback",-1,1,0};
    std::array<bool,3> rippleEditing{};
    juce::TextButton close{"CLOSE"};
    std::uint64_t epoch=0;
    bool editing=false,rage=false,isRipple=false;
};
}
