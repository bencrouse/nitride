#pragma once
#include "InstrumentSession.h"
#include "PresetPresentation.h"

namespace Nitride
{
class InstrumentPanel : public juce::Component
{
public:
    InstrumentPanel(InstrumentSession&,std::function<void()> audioSettings={},PresetPresentation=PresetPresentation::standalone);
    ~InstrumentPanel() override;
    void resized() override;
    void beginPreview();
    bool checkInteractions();
private:
    class Content;
    std::unique_ptr<Content> content;
};
}
