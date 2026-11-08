#include "FilterPanel.h"

namespace Nitride
{
FilterPanel::FilterPanel(InstrumentSession& s):session(s),epoch(s.restoreEpoch.load())
{
    setName("Advanced filter");setTitle(getName());setWantsKeyboardFocus(true);
    setDescription("Classic retains the shared Tone filter. Rage filters each voice before its amplitude envelope; distortion is inside resonance feedback.");
    family.setName("Filter family");family.setTitle("Filter family");family.addItemList({"Classic","Rage"},1);
    family.onChange=[this]{choose(filterFamily,family.getSelectedId()-1);};
    character.setName("Rage character");character.setTitle("Rage character");
    character.addItemList({"Diode","Distortion","Softclip","Tape","Transistor"},1);
    character.onChange=[this]{choose(rageCharacter,character.getSelectedId()-1);};
    amount.setName("Rage");amount.setTitle("Rage");amount.unit=UI::ValueUnit::normalized;
    amount.format=[](double v){return juce::String(v*100,0)+"%";};
    amount.setTooltip("Drives distortion inside each voice's resonance feedback loop. Zero is the clean Rage core, not Classic. Character settings are retained when Classic is selected.");
    amount.onBegin=[this]
    {
        if(epoch!=session.restoreEpoch.load())sync(session.readVisible(),session.restoreEpoch.load());
        if(!editing){session.beginEdit({hostIndex(rageAmount)});editing=true;}
    };
    amount.onChange=[this](double v){if(editing)session.setIfCurrent(rageAmount,v,epoch);};
    amount.onEnd=[this]{finishEdits();};
    close.setName("Close advanced filter");close.onClick=[this]{finishEdits();if(onClose)onClose();};
    for(auto* c:std::array<juce::Component*,4>{&family,&character,&amount,&close})addAndMakeVisible(*c);
    sync(session.readVisible(),epoch);
}
FilterPanel::~FilterPanel(){finishEdits();}
void FilterPanel::choose(Parameter parameter,double value)
{
    if(value<0)return;
    if(epoch!=session.restoreEpoch.load()){sync(session.readVisible(),session.restoreEpoch.load());return;}
    finishEdits();session.beginEdit({hostIndex(parameter)});session.setIfCurrent(parameter,value,epoch);session.endEdit({hostIndex(parameter)});
    sync(session.readVisible(),epoch);
}
void FilterPanel::finishEdits()
{if(editing){editing=false;session.endEdit({hostIndex(rageAmount)});}amount.cancelGesture();}
void FilterPanel::sync(const Patch& patch,std::uint64_t generation)
{
    if(epoch!=generation){editing=false;amount.cancelGesture();family.hidePopup();character.hidePopup();epoch=generation;}
    rage=patch.values[filterFamily]>=.5;
    family.setSelectedId(static_cast<int>(patch.values[filterFamily])+1,juce::dontSendNotification);
    character.setSelectedId(static_cast<int>(patch.values[rageCharacter])+1,juce::dontSendNotification);
    amount.setValue(patch.values[rageAmount]);character.setEnabled(rage);amount.setEnabled(rage);repaint();
}
void FilterPanel::paint(juce::Graphics& g)
{
    g.fillAll(UI::chassis());g.setColour(UI::accent().withAlpha(.5f));g.drawRect(getLocalBounds(),1);
    UI::text(g,"TONE / FILTER FAMILY",{20,16,430,20},UI::ink(),12);
    UI::text(g,"FAMILY",{20,70,250,18},UI::muted(),9);
    UI::text(g,"CHARACTER",{300,70,260,18},UI::muted(),9);
    UI::text(g,rage?"PER VOICE / NONLINEAR RESONANCE FEEDBACK":"CLASSIC / ORIGINAL SHARED TONE PATH",{20,198,540,24},UI::ink(),10);
    UI::text(g,rage?"Cutoff, resonance, tracking and Filter Envelope apply per voice.":"The original filter and all existing sounds remain unchanged.",{20,234,540,24},UI::muted(),10);
    UI::text(g,rage?"Filter FM uses each voice before its filter and amplitude envelope.":"Rage settings stay saved while Classic is selected.",{20,266,540,24},UI::muted(),10);
    UI::text(g,"Independent Rage-inspired DSP; not an Arturia-identical model.",{20,320,540,24},UI::muted(),9);
}
void FilterPanel::resized()
{close.setBounds(getWidth()-100,12,80,28);family.setBounds(20,96,260,32);character.setBounds(300,96,260,32);amount.setBounds(20,148,540,32);}
bool FilterPanel::keyPressed(const juce::KeyPress& key)
{if(key.getKeyCode()!=juce::KeyPress::escapeKey)return false;close.onClick();return true;}
}
