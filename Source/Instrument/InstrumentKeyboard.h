#pragma once
#include "InstrumentSession.h"

namespace Nitride
{
class InstrumentKeyboard final : public juce::MidiKeyboardComponent
{
public:
    InstrumentKeyboard(InstrumentSession& s):juce::MidiKeyboardComponent(s.keyboard,horizontalKeyboard),session(s){}
private:
    bool hostDown(int note) const noexcept {return session.editorMidi.hostHeld[static_cast<size_t>(note)].load(std::memory_order_relaxed)!=0;}
    void drawWhiteNote(int note,juce::Graphics& g,juce::Rectangle<float> area,bool down,bool over,juce::Colour line,juce::Colour label) override
    {juce::MidiKeyboardComponent::drawWhiteNote(note,g,area,down||hostDown(note),over,line,label);}
    void drawBlackNote(int note,juce::Graphics& g,juce::Rectangle<float> area,bool down,bool over,juce::Colour fill) override
    {juce::MidiKeyboardComponent::drawBlackNote(note,g,area,down||hostDown(note),over,fill);}
    InstrumentSession& session;
};
}
