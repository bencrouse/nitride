#include "InstrumentView.h"
#include "InstrumentPanel.h"

namespace Nitride
{
class InstrumentView::Panel final : public juce::Component
{
public:
    Panel(InstrumentSession& session, std::function<void()> audioSettings, PresetPresentation presentation)
        : content(session, std::move(audioSettings), presentation)
    {
        addAndMakeVisible(content);
        setSize(content.getWidth(), content.getHeight());
    }
    void resized() override { content.setBounds(getLocalBounds()); }
    InstrumentPanel content;
};

InstrumentView::InstrumentView(InstrumentSession& session, std::function<void()> audioSettings, PresetPresentation presentation)
    : panel(std::make_unique<Panel>(session, std::move(audioSettings), presentation))
{
    setName("Nitride instrument");
    addAndMakeVisible(*panel);
    setSize(1200, 960);
}

InstrumentView::~InstrumentView() = default;
void InstrumentView::resized() { panel->setBounds(getLocalBounds()); }
void InstrumentView::beginPreview() { panel->content.beginPreview(); }
bool InstrumentView::checkInteractions() { return panel->content.checkInteractions(); }
}
