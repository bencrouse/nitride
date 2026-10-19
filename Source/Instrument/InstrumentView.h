#pragma once

#include "InstrumentSession.h"
#include "PresetPresentation.h"

namespace Nitride
{
class InstrumentView final : public juce::Component
{
public:
    explicit InstrumentView(InstrumentSession&, std::function<void()> showAudioSettings = {}, PresetPresentation = PresetPresentation::standalone);
    ~InstrumentView() override;
    void resized() override;
    void beginPreview();
    bool checkInteractions();

private:
    class Panel;
    std::unique_ptr<Panel> panel;
};
}
