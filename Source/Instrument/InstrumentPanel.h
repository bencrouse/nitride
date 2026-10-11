#pragma once
#include "InstrumentSession.h"

namespace Nitride
{
class InstrumentPanel : public juce::Component
{
public:
    InstrumentPanel(InstrumentSession&,std::function<void()> audioSettings={});
    ~InstrumentPanel() override;
    void resized() override;
    void beginPreview();
    bool checkInteractions();
private:
    class Content;
    std::unique_ptr<Content> content;
};
}
