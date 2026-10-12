#include "FmPanel.h"

namespace Nitride
{
FmPanel::FmPanel(InstrumentSession& s):session(s),epoch(s.restoreEpoch.load())
{
    setName("Advanced FM");setTitle(getName());setWantsKeyboardFocus(true);
    setDescription(routingDescription);
    source.setName("FM source page");envelopeTab.setName("FM envelope page");modulation.setName("FM modulation page");
    source.onClick=[this]{selectPage(0);};envelopeTab.onClick=[this]{selectPage(1);};modulation.onClick=[this]{selectPage(2);};
    close.setName("Close advanced FM");close.onClick=[this]{finishEdits();if(onClose)onClose();};
    for(auto* button:{&source,&envelopeTab,&modulation,&close})addAndMakeVisible(button);
    for(int p=fmRatio;p<parameterCount;++p)
    {
        const auto parameter=static_cast<Parameter>(p);const auto i=static_cast<size_t>(p-fmRatio);const auto& d=hostParameters[static_cast<size_t>(p)];
        if(d.kind==ParameterKind::choice)
        {
            auto& control=choices[i];control=std::make_unique<juce::ComboBox>();control->setName(d.name);control->setTitle(d.name);
            control->addItemList(juce::StringArray::fromTokens(d.choices,"|",""),1);
            control->onChange=[this,parameter,i]{if(!updating)set(parameter,choices[i]->getSelectedId()-1);};addChildComponent(*control);
        }
        else
        {
            juce::String caption=d.name;caption=caption.replace("FM envelope ","").replace("FM ","");
            if(parameter>=fmRoute1Amount)caption="Amount";
            auto& control=values[i];control=std::make_unique<UI::ValueControl>(caption,d.minimum,d.maximum,d.initial,
                parameter==fmRatio||parameter==fmAttack||parameter==fmDecay||parameter==fmRelease);
            control->setName(d.name);control->setTitle(d.name);
            control->onBegin=[this,parameter]{begin(parameter);};control->onEnd=[this,parameter]{end(parameter);};
            control->onChange=[this,parameter](double v){set(parameter,v);};addChildComponent(*control);
            if(parameter==fmRatio)control->format=[](double v){return "1:"+juce::String(v,3);};
            else if(parameter==fmAttack||parameter==fmDecay||parameter==fmRelease)control->format=[](double v){return v>=1?juce::String(v,2)+" s":juce::String(v*1000,0)+" ms";};
            else if(parameter==fmShapeAmount||parameter==fmMix||parameter==fmSustain)control->format=[](double v){return juce::String(v*100,0)+"%";};
            if(parameter==fmRatio)control->unit=UI::ValueUnit::ratio;
            else if(parameter==fmAttack||parameter==fmDecay||parameter==fmRelease)control->unit=UI::ValueUnit::seconds;
            else if(parameter==fmShapeAmount||parameter==fmMix||parameter==fmSustain)control->unit=UI::ValueUnit::normalized;
            else if(parameter==fmEnvelopeAmount)control->unit=UI::ValueUnit::index;
        }
    }
    integers.setName("Harmonic FM ratio");integers.setTextWhenNothingSelected("Integer ratio...");
    for(int i=1;i<=8;++i)integers.addItem("1:"+juce::String(i),i);
    integers.onChange=[this]{if(!updating&&integers.getSelectedId()>0)set(fmRatio,integers.getSelectedId());};addChildComponent(integers);
    values[fmRatio-fmRatio]->setTooltip("Carrier:primary-modulator ratio. Continuous inharmonic values; integer shortcuts. Companion network nodes keep their established anchors.");
    values[fmMix-fmRatio]->setTooltip("0% carrier / 100% audible primary-network modulator. The internal feedback carrier is unchanged.");
    values[fmEnvelopeAmount-fmRatio]->setTooltip("Signed FM-index amount. Per-voice envelope, independent of amplitude and shared filter ADSR.");
    envelope.onBegin=[this]{for(const auto p:{fmAttack,fmDecay,fmSustain,fmRelease})begin(p);};
    envelope.onEnd=[this]{for(const auto p:{fmAttack,fmDecay,fmSustain,fmRelease})end(p);};
    envelope.onChange=[this](std::array<double,4> v){session.setGroupIfCurrent({{fmAttack,v[0]},{fmDecay,v[1]},{fmSustain,v[2]},{fmRelease,v[3]}},epoch);};addChildComponent(envelope);
    sync(session.readVisible(),epoch);selectPage(0);
}
FmPanel::~FmPanel(){finishEdits();}
void FmPanel::begin(Parameter p)
{
    const auto i=static_cast<size_t>(p-fmRatio);if(epoch!=session.restoreEpoch.load())sync(session.readVisible(),session.restoreEpoch.load());
    if(!editing[i]){session.beginEdit({hostIndex(p)});editing[i]=true;}
}
void FmPanel::end(Parameter p)
{const auto i=static_cast<size_t>(p-fmRatio);if(editing[i]){editing[i]=false;session.endEdit({hostIndex(p)});}}
void FmPanel::set(Parameter p,double value){session.setIfCurrent(p,value,epoch);}
void FmPanel::finishEdits()
{for(int p=fmRatio;p<parameterCount;++p)end(static_cast<Parameter>(p));cancelEdits();}
void FmPanel::cancelEdits(){editing={};for(auto& v:values)if(v)v->cancelGesture();envelope.cancelGesture();}
void FmPanel::sync(const Patch& patch,std::uint64_t generation)
{
    if(epoch!=generation){cancelEdits();epoch=generation;}updating=true;
    for(size_t i=0;i<count;++i){const auto value=patch.values[fmRatio+i];if(values[i])values[i]->setValue(value);if(choices[i])choices[i]->setSelectedId(static_cast<int>(value)+1,juce::dontSendNotification);}
    const auto ratio=patch.values[fmRatio];integers.setSelectedId(std::abs(ratio-std::round(ratio))<1e-6?static_cast<int>(std::round(ratio)):0,juce::dontSendNotification);
    envelope.setEnvelope({patch.values[fmAttack],patch.values[fmDecay],patch.values[fmSustain],patch.values[fmRelease]});
    for(const auto& route:fmRoutes)
    {
        auto* value=values[static_cast<size_t>(route.amount-fmRatio)].get();const auto target=static_cast<int>(patch.values[route.target]);
        value->format=[target](double v){return (v>0?"+":"")+juce::String(v,2)+(target==0?" idx":target==1?" oct":"");};
        value->unit=target==0?UI::ValueUnit::index:target==1?UI::ValueUnit::octaves:UI::ValueUnit::number;
    }
    updating=false;repaint();
}
void FmPanel::selectPage(int selected)
{
    finishEdits();page=selected;
    for(int p=fmRatio;p<parameterCount;++p)
    {
        const auto i=static_cast<size_t>(p-fmRatio);const auto visible=page==0?p<fmAttack:page==1?(p>=fmAttack&&p<=fmEnvelopeAmount):p>=fmRoute1Source;
        if(values[i])values[i]->setVisible(visible);if(choices[i])choices[i]->setVisible(visible);
    }
    integers.setVisible(page==0);envelope.setVisible(page==1);
    source.setToggleState(page==0,juce::dontSendNotification);envelopeTab.setToggleState(page==1,juce::dontSendNotification);modulation.setToggleState(page==2,juce::dontSendNotification);repaint();
}
void FmPanel::paint(juce::Graphics& g)
{
    g.fillAll(UI::chassis());g.setColour(UI::accent().withAlpha(.5f));g.drawRect(getLocalBounds(),1);
    UI::text(g,"FM / NETWORK SOURCE",{20,16,360,20},UI::ink(),12);
    if(page==0)
    {
        UI::text(g,"PRIMARY MODULATOR",{20,92,500,18},UI::muted(),9);
        UI::text(g,"Companion nodes retain their harmonic anchors and adaptive links.",{20,250,540,24},UI::muted(),10);
        UI::text(g,"Carrier / modulator blend changes output, never internal feedback.",{20,280,540,24},UI::muted(),10);
    }
    else if(page==1)UI::text(g,"PER VOICE / SIGNED INDEX OFFSET",{20,92,540,18},UI::muted(),9);
    else
    {
        UI::text(g,"SOURCE",{20,92,190,18},UI::muted(),9);UI::text(g,"TARGET",{230,92,180,18},UI::muted(),9);UI::text(g,"AMOUNT",{422,92,140,18},UI::muted(),9);
        UI::text(g,"Depth: index / Ratio: octaves / Shape and mix: normalized offsets",{20,270,540,24},UI::muted(),10);
        UI::text(g,routingDescription,{20,300,540,24},UI::muted(),10);
    }
}
void FmPanel::resized()
{
    source.setBounds(20,50,114,28);envelopeTab.setBounds(142,50,130,28);modulation.setBounds(280,50,156,28);close.setBounds(getWidth()-100,12,80,28);
    const auto bounds=[this](Parameter p,juce::Rectangle<int> r){const auto i=static_cast<size_t>(p-fmRatio);if(values[i])values[i]->setBounds(r);if(choices[i])choices[i]->setBounds(r);};
    bounds(fmRatio,{20,122,260,30});integers.setBounds(300,122,260,30);bounds(fmShape,{20,166,260,30});bounds(fmShapeAmount,{300,166,260,30});bounds(fmMix,{20,210,540,30});
    envelope.setBounds(20,120,250,120);bounds(fmAttack,{290,120,270,28});bounds(fmDecay,{290,156,270,28});bounds(fmSustain,{290,192,270,28});bounds(fmRelease,{290,228,270,28});bounds(fmEnvelopeAmount,{20,280,540,30});
    for(size_t i=0;i<fmRoutes.size();++i){const auto y=124+static_cast<int>(i)*46;const auto& r=fmRoutes[i];bounds(r.source,{20,y,198,32});bounds(r.target,{230,y,180,32});bounds(r.amount,{422,y,138,32});}
}
bool FmPanel::keyPressed(const juce::KeyPress& key){if(key.getKeyCode()!=juce::KeyPress::escapeKey)return false;close.onClick();return true;}
}
