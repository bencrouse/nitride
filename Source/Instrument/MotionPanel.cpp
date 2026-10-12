#include "MotionPanel.h"
#include "NativeGestureControls.h"

namespace Nitride
{
MotionPanel::MotionPanel(InstrumentSession& state):session(state),epoch(state.restoreEpoch.load())
{
    setName("Motion LFO bank");destination.setName("LFO destination");wave.setName("LFO waveform");division.setName("LFO subdivision");
    for(size_t i=0;i<lfoParameters.size();++i)destination.addItem(lfoParameters[i].name,static_cast<int>(i)+1);
    wave.addItemList({"Sine","Triangle","Sawtooth","Square","Sample & Hold"},1);
    for(size_t i=0;i<DSP::subdivisions.size();++i)division.addItem(DSP::subdivisions[i].name,static_cast<int>(i)+1);
    destination.onChange=[this]{if(updating)return;finishDrags();selected=static_cast<size_t>(std::max(0,destination.getSelectedId()-1));sync(session.readVisible(),session.restoreEpoch.load());};
    wave.onChange=[this]{if(!updating)set(lane().wave,wave.getSelectedId()-1);};division.onChange=[this]{if(!updating)set(lane().division,division.getSelectedId()-1);};
    enabled.setButtonText("ON");enabled.setName("Enable selected LFO");enabled.setClickingTogglesState(true);
    syncButton.setButtonText("SYNC");syncButton.setName("Sync selected LFO");syncButton.setClickingTogglesState(true);
    enabled.onClick=[this]{set(lane().enabled,enabled.getToggleState()?1:0);};syncButton.onClick=[this]{set(lane().sync,syncButton.getToggleState()?1:0);};
    destination.setTooltip("Select which lane to edit. Other enabled lanes keep running with their own rate and depth.");
    wave.setTooltip("Each destination has its own waveform. Sample & Hold changes once per cycle.");
    syncButton.setTooltip("Use musical subdivisions and host tempo/position. Without host tempo, use the editable fallback BPM.");
    for(auto* c:{&destination,&wave,&division})addAndMakeVisible(*c);addAndMakeVisible(enabled);addAndMakeVisible(syncButton);
    slider(rate,0);slider(depth,1);slider(tempo,2);
    rate.textFromValueFunction=[](double v){return juce::String(v,2)+" Hz";};
    depth.valueFromTextFunction=[](const juce::String& text){auto value=text.trim();if(value.startsWithIgnoreCase("DEPTH"))value=value.substring(5).trimStart();return value.getDoubleValue();};
    tempo.textFromValueFunction=[](double v){return juce::String(v,1)+" BPM";};
    rate.setTooltip("Independent free-running rate, 0.01-20 Hz. Double-click resets.");
    tempo.setTooltip("Fallback tempo for Standalone or a host without BPM. Host tempo takes priority.");
    sync(session.readVisible(),epoch);
}
MotionPanel::~MotionPanel(){finishDrags();}
void MotionPanel::finishDrags()
{for(size_t i=0;i<dragging.size();++i)if(dragging[i]){session.endEdit({hostIndex(edited[i])});dragging[i]=false;cancelled[i]=true;}}
void MotionPanel::slider(juce::Slider& s,size_t index)
{
    s.setSliderStyle(juce::Slider::LinearBar);s.setColour(juce::Slider::trackColourId,UI::accent().withAlpha(.18f));
    s.setColour(juce::Slider::backgroundColourId,juce::Colour(0xff252b27));s.setColour(juce::Slider::textBoxTextColourId,UI::ink());
    s.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);s.valueFromTextFunction=[](const juce::String& text){return text.getDoubleValue();};
    s.onDragStart=[this,index]{if(epoch!=session.restoreEpoch.load())sync(session.readVisible(),session.restoreEpoch.load());edited[index]=index==0?lane().rate:index==1?lane().depth:lfoTempo;cancelled[index]=false;dragging[index]=true;session.beginEdit({hostIndex(edited[index])});};
    s.onValueChange=[this,&s,index]{if(updating)return;if(epoch!=session.restoreEpoch.load()||cancelled[index]){sync(session.readVisible(),session.restoreEpoch.load());return;}
        const auto parameter=index==0?lane().rate:index==1?lane().depth:lfoTempo;auto v=s.getValue();if(index==1&&selected>=3)v/=100;set(parameter,v);};
    s.onDragEnd=[this,index]{if(dragging[index])session.endEdit({hostIndex(edited[index])});dragging[index]=cancelled[index]=false;};
    addAndMakeVisible(s);
}
void MotionPanel::set(Parameter p,double value)
{
    if(updating)return;
    // Non-pointer changes are complete single-control history transactions.
    const auto pointer=std::any_of(dragging.begin(),dragging.end(),[](bool d){return d;});
    if(!pointer&&epoch==session.restoreEpoch.load())session.beginEdit({hostIndex(p)});
    session.setIfCurrent(p,value,epoch);
    if(!pointer)session.endEdit({hostIndex(p)});
    sync(session.readVisible(),session.restoreEpoch.load());
}
void MotionPanel::sync(const Patch& values,std::uint64_t generation)
{
    if(epoch!=generation){for(size_t i=0;i<dragging.size();++i)if(dragging[i]){dragging[i]=false;cancelled[i]=true;}epoch=generation;}
    patch=values;updating=true;const auto& p=lane();
    destination.setSelectedId(static_cast<int>(selected)+1,juce::dontSendNotification);wave.setSelectedId(static_cast<int>(values.values[p.wave])+1,juce::dontSendNotification);
    division.setSelectedId(static_cast<int>(values.values[p.division])+1,juce::dontSendNotification);
    enabled.setToggleState(values.values[p.enabled]>=.5,juce::dontSendNotification);syncButton.setToggleState(values.values[p.sync]>=.5,juce::dontSendNotification);
    const auto configure=[&](juce::Slider& s,Parameter parameter,double scale)
    {const auto& d=hostParameters[static_cast<size_t>(parameter)];s.setName(d.name);s.setNormalisableRange({d.minimum*scale,d.maximum*scale,0,d.skew});s.setDoubleClickReturnValue(true,d.initial*scale);s.setValue(values.values[parameter]*scale,juce::dontSendNotification);};
    configure(rate,p.rate,1);configure(depth,p.depth,selected>=3?100:1);configure(tempo,lfoTempo,1);
    depth.textFromValueFunction=[this](double v){return "DEPTH  "+juce::String(v,selected>=3?0:2)+" "+lane().unit;};
    depth.setTooltip("Independent modulation amount for "+juce::String(p.name)+". Zero is neutral; negative depth inverts bipolar destinations.");
    rate.setVisible(values.values[p.sync]<.5);division.setVisible(values.values[p.sync]>=.5);updating=false;rate.updateText();depth.updateText();tempo.updateText();repaint();
}
void MotionPanel::paint(juce::Graphics& g)
{
    const juce::Rectangle<float> plot{0,91,205,27};g.setColour(UI::glass());g.fillRoundedRectangle(plot,3);juce::Path path;
    const auto waveform=static_cast<DSP::LfoWave>(static_cast<int>(patch.values[lane().wave]));
    for(int i=0;i<190;++i)
    {
        const auto p=static_cast<double>(i)/190;const auto y=DSP::lfoWave(waveform,p,waveform==DSP::LfoWave::sampleHold?i/32:0,selected+1);
        const auto point=juce::Point<float>{8+static_cast<float>(p)*189,104.5f-static_cast<float>(y)*9};if(i==0)path.startNewSubPath(point);else path.lineTo(point);
    }
    g.setColour(UI::signal().withAlpha(.65f));g.strokePath(path,juce::PathStrokeType(1));
    g.setColour(UI::accent());g.fillEllipse(7+session.live.lfoPhases[selected].load()*189,102,4,4);
    UI::text(g,juce::String(session.live.lfoBpm.load(),1)+" BPM "+(session.live.lfoHostTempo.load()?"HOST":"FALLBACK"),{0,121,getWidth(),14},UI::muted(),8);
}
void MotionPanel::resized()
{
    const auto width=getWidth();destination.setBounds(0,0,166,24);wave.setBounds(174,0,width-174,24);
    enabled.setBounds(0,30,62,24);syncButton.setBounds(70,30,62,24);rate.setBounds(140,30,width-140,24);division.setBounds(rate.getBounds());
    depth.setBounds(0,60,width,24);tempo.setBounds(213,91,width-213,27);
}
}
