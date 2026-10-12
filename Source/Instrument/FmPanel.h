#pragma once
#include "SupportingControls.h"
#include "InstrumentSession.h"

namespace Nitride
{
class FmPanel final:public juce::Component
{
public:
    explicit FmPanel(InstrumentSession&);
    ~FmPanel() override;
    void sync(const Patch&,std::uint64_t);
    void finishEdits();
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    std::function<void()> onClose;
private:
    void selectPage(int);
    void set(Parameter,double);
    void begin(Parameter);
    void end(Parameter);
    void cancelEdits();
    InstrumentSession& session;
    std::uint64_t epoch;
    bool updating=false;
    int page=0;
    static constexpr size_t count=parameterCount-fmRatio;
    std::array<std::unique_ptr<UI::ValueControl>,count> values;
    std::array<std::unique_ptr<juce::ComboBox>,count> choices;
    std::array<bool,count> editing{};
    juce::TextButton source{"SOURCE"},envelopeTab{"ENVELOPE"},modulation{"MODULATION"},close{"CLOSE"};
    juce::ComboBox integers;
    UI::EnvelopeControl envelope{"FM envelope"};
};
}
