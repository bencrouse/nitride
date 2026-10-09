#include "PitchMotionPanel.h"

namespace Nitride
{
PitchMotionPanel::PitchMotionPanel(InstrumentSession& state,bool autobend):session(state),autoMode(autobend),epoch(state.restoreEpoch.load())
{
    setName(autoMode?"Autobend controls":"Glide controls");
    enabled.setButtonText(autoMode?"AUTOBEND":"GLIDE");enabled.setClickingTogglesState(true);
    enabled.setName(autoMode?"Enable Autobend":"Enable Glide");
    enabled.onClick=[this]{set(autoMode?autobendOn:glideOn,enabled.getToggleState()?1:0);};
    enabled.setTooltip(autoMode?"Each note can start above or below its destination and settle into tune":"Slide between notes; retarget from the current pitch");
    addAndMakeVisible(enabled);
    const auto targetParameter=autoMode?autobendTarget:glideTarget;
    configureChoice(destination,targetParameter,juce::StringArray::fromTokens(hostParameters[static_cast<size_t>(targetParameter)].choices,"|",""));
    configureChoice(trigger,autoMode?autobendPhrase:glideLegato,autoMode?juce::StringArray{"Each note","New phrase"}:juce::StringArray{"Always","Legato only"});
    configureSlider(time,autoMode?autobendTime:glideTime,true);
    if(autoMode)configureSlider(depth,autobendDepth,false);
    else configureChoice(curve,glideCurve,juce::StringArray::fromTokens(hostParameters[glideCurve].choices,"|",""));
    destination.setTooltip("Carrier and Modulators move independently. Opposed moves them equally in opposite directions. Pitch + Tone also moves the shared filter.");
    trigger.setTooltip(autoMode?"New phrase bends only the first strike before all physical keys have been released":"Legato only slides when keys overlap, or when returning to a held key");
    curve.setTooltip("Smooth and Linear arrive in the displayed time. Classic is an exponential frequency slide: about 3 x Time reaches 95%.");
    sync(state.readVisible(),epoch);
}
PitchMotionPanel::~PitchMotionPanel()
{
    for(size_t i=0;i<dragging.size();++i)if(dragging[i])session.endEdit({hostIndex(i==0?(autoMode?autobendTime:glideTime):autobendDepth)});
}
void PitchMotionPanel::configureChoice(juce::ComboBox& box,Parameter parameter,const juce::StringArray& choices)
{
    box.setName(hostParameters[static_cast<size_t>(parameter)].name);
    box.addItemList(choices,1);box.onChange=[this,&box,parameter]{if(!updating)set(parameter,box.getSelectedId()-1);};addAndMakeVisible(box);
}
void PitchMotionPanel::configureSlider(juce::Slider& slider,Parameter parameter,bool isTime)
{
    const auto& definition=hostParameters[static_cast<size_t>(parameter)];
    slider.setName(definition.name);slider.setSliderStyle(juce::Slider::LinearBar);
    slider.setNormalisableRange({definition.minimum,definition.maximum,0,definition.skew});slider.setDoubleClickReturnValue(true,definition.initial);
    slider.setColour(juce::Slider::trackColourId,juce::Colour(0xfff87946).withAlpha(.18f));
    slider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffe5dfd2));
    slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::backgroundColourId,juce::Colour(0xff252b27));
    slider.textFromValueFunction=[isTime](double v) { return isTime?(v>=1?juce::String(v,2)+" s":juce::String(v*1000,0)+" ms"):(v>0?"+":"")+juce::String(v,std::abs(v-std::round(v))<.001?0:1)+" st"; };
    slider.valueFromTextFunction=[isTime](const juce::String& text) {const auto v=text.getDoubleValue();return isTime&&text.containsIgnoreCase("ms")?v/1000:v;};
    slider.updateText();
    slider.setTooltip(isTime?"Drag to set time; double-click resets. Text entry accepts ms or s.":"Positive depth falls from above the note; negative depth scoops upward. +/-36 semitones.");
    const size_t index=isTime?0:1;
    slider.onDragStart=[this,index,parameter] {
        if(epoch!=session.restoreEpoch.load())sync(session.readVisible(),session.restoreEpoch.load());
        cancelled[index]=false;dragging[index]=true;session.beginEdit({hostIndex(parameter)});
    };
    slider.onValueChange=[this,&slider,index,parameter] {
        if(updating)return;
        if(epoch!=session.restoreEpoch.load()||cancelled[index]){sync(session.readVisible(),session.restoreEpoch.load());return;}
        set(parameter,slider.getValue());
    };
    slider.onDragEnd=[this,index,parameter] {if(dragging[index])session.endEdit({hostIndex(parameter)});dragging[index]=cancelled[index]=false;};
    addAndMakeVisible(slider);
}
void PitchMotionPanel::set(Parameter parameter,double value)
{
    if(updating)return;session.set(parameter,value);sync(session.readVisible(),session.restoreEpoch.load());
}
void PitchMotionPanel::sync(const Patch& patch,std::uint64_t restoreEpoch)
{
    if(epoch!=restoreEpoch)
    {
        for(size_t i=0;i<dragging.size();++i)if(dragging[i]){cancelled[i]=true;dragging[i]=false;}
        epoch=restoreEpoch;
    }
    updating=true;
    enabled.setToggleState(patch.values[autoMode?autobendOn:glideOn]>=.5,juce::dontSendNotification);
    destination.setSelectedId(static_cast<int>(patch.values[autoMode?autobendTarget:glideTarget])+1,juce::dontSendNotification);
    trigger.setSelectedId(static_cast<int>(patch.values[autoMode?autobendPhrase:glideLegato])+1,juce::dontSendNotification);
    time.setValue(patch.values[autoMode?autobendTime:glideTime],juce::dontSendNotification);
    if(autoMode)depth.setValue(patch.values[autobendDepth],juce::dontSendNotification);
    else curve.setSelectedId(static_cast<int>(patch.values[glideCurve])+1,juce::dontSendNotification);
    updating=false;repaint();
}
void PitchMotionPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff0d100f));g.fillRoundedRectangle(getLocalBounds().toFloat(),4);
    g.setFont(juce::Font(juce::FontOptions("Menlo",9,juce::Font::plain)));g.setColour(juce::Colour(0xff969b8f));
    g.drawText("TIME",6,27,40,28,juce::Justification::left);
    if(autoMode)g.drawText("DEPTH",118,27,40,28,juce::Justification::left);
}
void PitchMotionPanel::resized()
{
    enabled.setBounds(0,0,96,23);destination.setBounds(102,0,getWidth()-102,23);
    if(autoMode){time.setBounds(43,29,65,26);depth.setBounds(158,29,getWidth()-158,26);trigger.setBounds(0,61,getWidth(),23);}
    else{time.setBounds(46,29,getWidth()-46,26);trigger.setBounds(0,61,110,23);curve.setBounds(116,61,getWidth()-116,23);}
}
}
