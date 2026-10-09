#include "InstrumentView.h"
#include "PitchMotionPanel.h"

#include <iostream>
#include <vector>

namespace
{
#include "NativeGestureControls.inc"
#define NITRIDE_HOST_VIEW 1
#include "../Studies/InstrumentReview.inc"
}

namespace Nitride
{
class InstrumentView::Panel final : public juce::Component
{
public:
    Panel(InstrumentSession& session, std::function<void()> audioSettings)
        : content(session, std::move(audioSettings))
    {
        addAndMakeVisible(content);
        setSize(content.getWidth(), content.getHeight());
    }
    void resized() override { content.setBounds(getLocalBounds()); }
    InstrumentReview content;
};

InstrumentView::InstrumentView(InstrumentSession& session, std::function<void()> audioSettings)
    : panel(std::make_unique<Panel>(session, std::move(audioSettings)))
{
    setName("Nitride instrument");
    addAndMakeVisible(*panel);
    setSize(1200, 840);
}

InstrumentView::~InstrumentView() = default;
void InstrumentView::resized() { panel->setBounds(getLocalBounds()); }
void InstrumentView::beginPreview() { panel->content.beginPreview(); }
bool InstrumentView::checkInteractions() { return panel->content.checkInteractions(); }
}
