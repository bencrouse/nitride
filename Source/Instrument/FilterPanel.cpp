#include "FilterPanel.h"

namespace Nitride
{
FilterPanel::FilterPanel(InstrumentSession& s):session(s),epoch(s.restoreEpoch.load())
{
    setName("Advanced filter");setTitle(getName());setWantsKeyboardFocus(true);
    setDescription("Classic retains the shared Tone filter. Rage and Ripple run per voice before amplitude. Ripple uses a same-frequency all-pass cascade with waveshaping or signed feedback.");
    family.setName("Filter family");family.setTitle("Filter family");family.addItemList({"Classic","Rage","Ripple"},1);
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
    rippleTypeChoice.setName("Ripple type");rippleTypeChoice.setTitle("Ripple type");rippleTypeChoice.addItemList({"Subtle","Medium","Hard"},1);
    rippleTypeChoice.onChange=[this]{choose(rippleType,rippleTypeChoice.getSelectedId()-1);};
    rippleModeChoice.setName("Ripple mode");rippleModeChoice.setTitle("Ripple mode");rippleModeChoice.addItemList({"Waveshaper","Feedback"},1);
    rippleModeChoice.onChange=[this]{choose(rippleMode,rippleModeChoice.getSelectedId()-1);};
    const std::array<UI::ValueControl*,3> controls{&dry,&drive,&feedback};
    constexpr std::array parameters{rippleDry,rippleDrive,rippleFeedback};
    for(size_t i=0;i<controls.size();++i)
    {
        auto* c=controls[i];const auto p=parameters[i];c->setName(hostParameters[p].name);c->setTitle(c->getName());c->unit=UI::ValueUnit::normalized;
        c->format=[](double v){return juce::String(v*100,0)+"%";};
        c->onBegin=[this,i,p]{if(epoch!=session.restoreEpoch.load())sync(session.readVisible(),session.restoreEpoch.load());if(!rippleEditing[i]){session.beginEdit({hostIndex(p)});rippleEditing[i]=true;}};
        c->onChange=[this,i,p](double value){if(rippleEditing[i])session.setIfCurrent(p,value,epoch);};
        c->onEnd=[this,i,p]{if(rippleEditing[i]){rippleEditing[i]=false;session.endEdit({hostIndex(p)});}};
        addChildComponent(*c);
    }
    dry.setTooltip("Adds original input to the all-pass branch, not a dry/wet crossfade. Shared control-only makeup preserves the blend; no audio-level normalization.");
    drive.setTooltip("Post-all-pass waveshaping. Zero is the linear all-pass path. Independent of Rage drive.");
    feedback.setTooltip("Signed all-pass feedback, internally bounded to +/-0.94 with explicit input attenuation. Not an output limiter.");
    addChildComponent(rippleTypeChoice);addChildComponent(rippleModeChoice);
    for(auto* c:std::array<juce::Component*,4>{&family,&character,&amount,&close})addAndMakeVisible(*c);
    sync(session.readVisible(),epoch);
}
FilterPanel::~FilterPanel(){finishEdits();}
void FilterPanel::choose(Parameter parameter,double value)
{
    if(value<0)return;
    if(epoch!=session.restoreEpoch.load()){sync(session.readVisible(),session.restoreEpoch.load());return;}
    finishEdits();
    if(parameter==filterFamily)
    {
        if(value==2)
        {
            session.beginEdit({hostIndex(rippleOn)});session.setIfCurrent(rippleOn,1,epoch);session.endEdit({hostIndex(rippleOn)});
        }
        else if(session.read().values[rippleOn]>=.5)
        {
            session.beginEdit({hostIndex(filterFamily),hostIndex(rippleOn)});
            session.setGroupIfCurrent({{rippleOn,0},{filterFamily,value}},epoch);
            session.endEdit({hostIndex(filterFamily),hostIndex(rippleOn)});
        }
        else
        {
            // Do not introduce appended history indices when Ripple was untouched.
            session.beginEdit({hostIndex(filterFamily)});session.setIfCurrent(filterFamily,value,epoch);session.endEdit({hostIndex(filterFamily)});
        }
    }
    else{session.beginEdit({hostIndex(parameter)});session.setIfCurrent(parameter,value,epoch);session.endEdit({hostIndex(parameter)});}
    sync(session.readVisible(),epoch);
}
void FilterPanel::finishEdits()
{
    if(editing){editing=false;session.endEdit({hostIndex(rageAmount)});}amount.cancelGesture();
    const std::array<UI::ValueControl*,3> controls{&dry,&drive,&feedback};constexpr std::array parameters{rippleDry,rippleDrive,rippleFeedback};
    for(size_t i=0;i<controls.size();++i){if(rippleEditing[i]){rippleEditing[i]=false;session.endEdit({hostIndex(parameters[i])});}controls[i]->cancelGesture();}
}
void FilterPanel::sync(const Patch& patch,std::uint64_t generation)
{
    if(epoch!=generation){editing=false;rippleEditing={};amount.cancelGesture();dry.cancelGesture();drive.cancelGesture();feedback.cancelGesture();family.hidePopup();character.hidePopup();rippleTypeChoice.hidePopup();rippleModeChoice.hidePopup();epoch=generation;}
    isRipple=patch.values[rippleOn]>=.5;rage=!isRipple&&patch.values[filterFamily]>=.5;
    family.setSelectedId(isRipple?3:static_cast<int>(patch.values[filterFamily])+1,juce::dontSendNotification);
    character.setSelectedId(static_cast<int>(patch.values[rageCharacter])+1,juce::dontSendNotification);
    amount.setValue(patch.values[rageAmount]);character.setEnabled(rage);amount.setEnabled(rage);
    character.setVisible(!isRipple);amount.setVisible(!isRipple);
    rippleTypeChoice.setVisible(isRipple);rippleModeChoice.setVisible(isRipple);dry.setVisible(isRipple);drive.setVisible(isRipple);feedback.setVisible(isRipple);
    rippleTypeChoice.setSelectedId(static_cast<int>(patch.values[rippleType])+1,juce::dontSendNotification);
    rippleModeChoice.setSelectedId(static_cast<int>(patch.values[rippleMode])+1,juce::dontSendNotification);
    dry.setValue(patch.values[rippleDry]);drive.setValue(patch.values[rippleDrive]);feedback.setValue(patch.values[rippleFeedback]);
    drive.setEnabled(patch.values[rippleMode]==0);feedback.setEnabled(patch.values[rippleMode]==1);repaint();
}
void FilterPanel::paint(juce::Graphics& g)
{
    g.fillAll(UI::chassis());g.setColour(UI::accent().withAlpha(.5f));g.drawRect(getLocalBounds(),1);
    UI::text(g,"TONE / FILTER FAMILY",{20,16,430,20},UI::ink(),12);
    UI::text(g,"FAMILY",{20,70,250,18},UI::muted(),9);
    UI::text(g,isRipple?"TYPE / PHASE DEPTH":"CHARACTER",{300,70,260,18},UI::muted(),9);
    if(isRipple)
    {
        UI::text(g,"MODE",{20,126,250,18},UI::muted(),9);
        UI::text(g,"PER VOICE / SAME-FREQUENCY ALL-PASS CASCADE",{20,272,540,24},UI::ink(),10);
        UI::text(g,"Cutoff centers phase delay; resonance narrows its bandwidth.",{20,306,540,24},UI::muted(),10);
        UI::text(g,"Independent Ripple-inspired model; no Pigments equivalence claimed.",{20,342,540,24},UI::muted(),9);return;
    }
    UI::text(g,rage?"PER VOICE / NONLINEAR RESONANCE FEEDBACK":"CLASSIC / ORIGINAL SHARED TONE PATH",{20,198,540,24},UI::ink(),10);
    UI::text(g,rage?"Cutoff, resonance, tracking and Filter Envelope apply per voice.":"The original filter and all existing sounds remain unchanged.",{20,234,540,24},UI::muted(),10);
    UI::text(g,rage?"Filter FM uses each voice before its filter and amplitude envelope.":"Rage settings stay saved while Classic is selected.",{20,266,540,24},UI::muted(),10);
    UI::text(g,"Independent Rage-inspired DSP; not an Arturia-identical model.",{20,320,540,24},UI::muted(),9);
}
void FilterPanel::resized()
{
    close.setBounds(getWidth()-100,12,80,28);family.setBounds(20,96,260,32);character.setBounds(300,96,260,32);amount.setBounds(20,148,540,32);
    rippleTypeChoice.setBounds(300,96,260,32);rippleModeChoice.setBounds(300,136,260,28);
    dry.setBounds(20,180,260,32);drive.setBounds(300,180,260,32);feedback.setBounds(20,226,540,32);
}
bool FilterPanel::keyPressed(const juce::KeyPress& key)
{if(key.getKeyCode()!=juce::KeyPress::escapeKey)return false;close.onClick();return true;}
}
