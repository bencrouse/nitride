#pragma once

#include "InstrumentSession.h"

namespace Nitride
{
class InstrumentView final : public juce::Component
{
public:
    explicit InstrumentView(InstrumentSession&, std::function<void()> showAudioSettings = {});
    ~InstrumentView() override;
    void resized() override;
    void beginPreview();
    bool checkInteractions();

private:
    class Panel;
    std::unique_ptr<Panel> panel;
};
}
