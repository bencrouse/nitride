#pragma once
#include "NativeGestureControls.h"

namespace Nitride::UI
{
class ValueControl final : public juce::Component
{
public:
    ValueControl(const juce::String& label,double low,double high,double initialValue,bool logarithmic=false)
        :caption(label),minimum(low),maximum(high),value(initialValue),defaultValue(initialValue),logScale(logarithmic)
    {setName(label);setTitle(label);setDescription("Drag vertically to change "+label+". Arrows adjust. Double-click resets.");setWantsKeyboardFocus(true);setMouseCursor(juce::MouseCursor::UpDownResizeCursor);}
    std::function<void()> onBegin,onEnd;
    std::function<void(double)> onChange;
    std::function<juce::String(double)> format;
    void setValue(double v){value=juce::jlimit(minimum,maximum,v);repaint();}
    double getValue() const{return value;}
    void cancelGesture(){dragging=false;}
    double position(double v) const{return logScale?std::log(v/minimum)/std::log(maximum/minimum):(v-minimum)/(maximum-minimum);}
    double fromPosition(double p) const{p=juce::jlimit(0.0,1.0,p);return logScale?minimum*std::pow(maximum/minimum,p):minimum+p*(maximum-minimum);}
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff252b27));g.fillRoundedRectangle(getLocalBounds().toFloat(),3);
        text(g,caption.toUpperCase(),{10,0,getWidth()/2,getHeight()},muted(),9);
        text(g,format?format(value):juce::String(value,2),{getWidth()/2,0,getWidth()/2-10,getHeight()},ink(),12,juce::Justification::right);
        if(isMouseOverOrDragging()||hasKeyboardFocus(true)){g.setColour(accent());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),3,1);}
    }
    void mouseDown(const juce::MouseEvent& e) override{if(!e.mods.isLeftButtonDown())return;grabKeyboardFocus();if(onBegin)onBegin();dragging=true;origin=e.position;initial=position(value);}
    void mouseDrag(const juce::MouseEvent& e) override{if(dragging)change(fromPosition(initial+(origin.y-e.position.y)/(e.mods.isShiftDown()?900:170)));}
    void mouseUp(const juce::MouseEvent&) override{finish();}
    void mouseDoubleClick(const juce::MouseEvent&) override{if(onBegin)onBegin();change(defaultValue);if(onEnd)onEnd();}
    void mouseEnter(const juce::MouseEvent&) override{repaint();}void mouseExit(const juce::MouseEvent&) override{repaint();}
    void focusGained(FocusChangeType) override{repaint();}void focusLost(FocusChangeType) override{finish();repaint();}
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto c=key.getKeyCode();if(c!=juce::KeyPress::upKey&&c!=juce::KeyPress::downKey&&c!=juce::KeyPress::leftKey&&c!=juce::KeyPress::rightKey)return false;
        if(onBegin)onBegin();change(fromPosition(position(value)+((c==juce::KeyPress::upKey||c==juce::KeyPress::rightKey)?1:-1)*(key.getModifiers().isShiftDown()?.002:.02)));if(onEnd)onEnd();return true;
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Value final : public juce::AccessibilityValueInterface
        {
        public:
            explicit Value(ValueControl& v):view(v){}bool isReadOnly() const override{return false;}
            double getCurrentValue() const override{return view.value;}
            juce::String getCurrentValueAsString() const override{return view.format?view.format(view.value):juce::String(view.value);}
            void setValue(double v) override{if(view.onBegin)view.onBegin();view.change(v);if(view.onEnd)view.onEnd();}
            void setValueAsString(const juce::String& v) override{setValue(v.getDoubleValue());}
            AccessibleValueRange getRange() const override{return{{view.minimum,view.maximum},(view.maximum-view.minimum)/100};}
        private:ValueControl& view;
        };
        return std::make_unique<juce::AccessibilityHandler>(*this,juce::AccessibilityRole::slider,juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus,[this]{grabKeyboardFocus();}),juce::AccessibilityHandler::Interfaces(std::make_unique<Value>(*this)));
    }
private:
    void finish(){if(dragging){dragging=false;if(onEnd)onEnd();}}
    void change(double v){setValue(v);if(onChange)onChange(value);}
    juce::String caption;double minimum,maximum,value,defaultValue,initial=0;bool logScale,dragging=false;juce::Point<float> origin;
};

class EnvelopeControl final : public juce::Component
{
public:
    EnvelopeControl(){setName("Amplitude envelope");setTitle(getName());setDescription("Drag attack, decay/sustain, and release points.");setWantsKeyboardFocus(true);}
    std::function<void()> onBegin,onEnd;std::function<void(std::array<double,4>)> onChange;
    void setEnvelope(std::array<double,4> v){values=v;repaint();}void cancelGesture(){dragging=false;}
    juce::Rectangle<float> plot() const{return getLocalBounds().toFloat().reduced(12,15);}
    static double norm(double v,double low,double high){return std::log(v/low)/std::log(high/low);}
    std::array<juce::Point<float>,3> handles() const
    {
        const auto p=plot();const auto a=p.getX()+p.getWidth()*static_cast<float>(.08+.22*norm(values[0],.001,4));
        return{{{a,p.getY()+6},{a+p.getWidth()*static_cast<float>(.1+.13*norm(values[1],.01,4)),p.getBottom()-static_cast<float>(values[2])*(p.getHeight()-12)},{p.getX()+p.getWidth()*static_cast<float>(.75+.23*norm(values[3],.02,8)),p.getBottom()}}};
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(glass());g.fillRoundedRectangle(getLocalBounds().toFloat(),4);const auto p=plot();const auto points=handles();
        juce::Path path;path.startNewSubPath(p.getX(),p.getBottom());path.lineTo(points[0]);path.cubicTo(points[0].x+10,points[0].y,points[1].x-10,points[1].y,points[1].x,points[1].y);
        path.lineTo(p.getX()+p.getWidth()*.67f,points[1].y);path.cubicTo(p.getX()+p.getWidth()*.72f,points[1].y,points[2].x-12,p.getBottom(),points[2].x,p.getBottom());
        auto fill=path;fill.lineTo(p.getX(),p.getBottom());fill.closeSubPath();g.setColour(accent().withAlpha(.065f));g.fillPath(fill);g.setColour(signal().withAlpha(.75f));g.strokePath(path,juce::PathStrokeType(1.2f));
        for(const auto point:points){g.setColour(accent());g.fillEllipse(point.x-3,point.y-3,6,6);}if(hasKeyboardFocus(true)){g.setColour(accent());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2),3,1);}
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if(!e.mods.isLeftButtonDown())return;grabKeyboardFocus();if(onBegin)onBegin();dragging=true;origin=e.position;initial=values;
        const auto points=handles();float distance=1e9f;for(int i=0;i<3;++i)if(const auto d=points[static_cast<size_t>(i)].getDistanceFrom(e.position);d<distance){selected=i;distance=d;}
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if(!dragging)return;const auto dx=static_cast<double>(e.position.x-origin.x)/plot().getWidth();
        if(selected==0)values[0]=.001*std::pow(4000.0,juce::jlimit(0.0,1.0,norm(initial[0],.001,4)+dx*3));
        else if(selected==1){values[1]=.01*std::pow(400.0,juce::jlimit(0.0,1.0,norm(initial[1],.01,4)+dx*3));values[2]=juce::jlimit(0.0,1.0,initial[2]+static_cast<double>(origin.y-e.position.y)/plot().getHeight());}
        else values[3]=.02*std::pow(400.0,juce::jlimit(0.0,1.0,norm(initial[3],.02,8)+dx*3));if(onChange)onChange(values);repaint();
    }
    void mouseUp(const juce::MouseEvent&) override{finish();}void focusLost(FocusChangeType) override{finish();repaint();}
private:
    void finish(){if(dragging){dragging=false;if(onEnd)onEnd();}}
    std::array<double,4> values{.65,.7,.72,1.4},initial=values;juce::Point<float> origin;int selected=0;bool dragging=false;
};

class XYControl final : public juce::Component
{
public:
    enum class Kind{filter,motion,space};
    explicit XYControl(Kind k):kind(k){setName(k==Kind::filter?"Filter cutoff and resonance":k==Kind::motion?"Motion rate and depth":"Space mix and size");setTitle(getName());setDescription("Drag to shape "+getName()+". Arrow keys adjust.");setWantsKeyboardFocus(true);}
    std::function<void()> onBegin,onEnd;std::function<void(float,float)> onChange;
    void setPosition(float a,float b){x=a;y=b;repaint();}void cancelGesture(){dragging=false;}
    juce::Rectangle<float> plot() const{return getLocalBounds().toFloat().reduced(15,12);}
    void paint(juce::Graphics& g) override
    {
        g.setColour(glass());g.fillRoundedRectangle(getLocalBounds().toFloat(),4);const auto p=plot();const auto marker=juce::Point<float>(p.getX()+x*p.getWidth(),p.getBottom()-y*p.getHeight());
        g.setColour(signal().withAlpha(.13f));g.drawLine(p.getX(),p.getCentreY(),p.getRight(),p.getCentreY(),.5f);juce::Path path;
        if(kind==Kind::filter){path.startNewSubPath(p.getX(),p.getY()+p.getHeight()*.35f);path.lineTo(marker.x-18,p.getY()+p.getHeight()*.35f);path.quadraticTo(marker.x,p.getY()-y*12,marker.x+10,p.getCentreY());path.lineTo(std::min(p.getRight(),marker.x+55),p.getBottom());}
        else if(kind==Kind::motion){for(int i=0;i<=150;++i){const auto t=static_cast<float>(i)/150;const auto point=juce::Point<float>(p.getX()+t*p.getWidth(),p.getCentreY()-std::sin(t*pi*(2+x*5))*(5+y*p.getHeight()*.38f));if(i==0)path.startNewSubPath(point);else path.lineTo(point);}}
        else for(int i=0;i<4;++i){const auto r=12+static_cast<float>(i)*(9+y*12);g.setColour(signal().withAlpha(.1f+x*.13f));g.drawEllipse(p.getCentreX()-r*1.8f,p.getCentreY()-r*.45f,r*3.6f,r*.9f,.8f);}
        g.setColour(signal().withAlpha(.65f));g.strokePath(path,juce::PathStrokeType(1.2f));g.setColour(accent());g.fillEllipse(marker.x-3,marker.y-3,6,6);if(hasKeyboardFocus(true)){g.setColour(accent());g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2),3,1);}
    }
    void mouseDown(const juce::MouseEvent& e) override{if(!e.mods.isLeftButtonDown())return;grabKeyboardFocus();if(onBegin)onBegin();dragging=true;move(e.position);}
    void mouseDrag(const juce::MouseEvent& e) override{if(dragging)move(e.position);}void mouseUp(const juce::MouseEvent&) override{finish();}
    void focusLost(FocusChangeType) override{finish();repaint();}
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto c=key.getKeyCode();if(c!=juce::KeyPress::leftKey&&c!=juce::KeyPress::rightKey&&c!=juce::KeyPress::upKey&&c!=juce::KeyPress::downKey)return false;if(onBegin)onBegin();
        const auto step=key.getModifiers().isShiftDown()?.002f:.02f;if(c==juce::KeyPress::leftKey)x-=step;if(c==juce::KeyPress::rightKey)x+=step;if(c==juce::KeyPress::upKey)y+=step;if(c==juce::KeyPress::downKey)y-=step;
        setPosition(juce::jlimit(0.0f,1.0f,x),juce::jlimit(0.0f,1.0f,y));if(onChange)onChange(x,y);if(onEnd)onEnd();return true;
    }
private:
    void finish(){if(dragging){dragging=false;if(onEnd)onEnd();}}void move(juce::Point<float> p){const auto a=plot();setPosition(juce::jlimit(0.0f,1.0f,(p.x-a.getX())/a.getWidth()),juce::jlimit(0.0f,1.0f,1-(p.y-a.getY())/a.getHeight()));if(onChange)onChange(x,y);}
    Kind kind;float x=.5f,y=.2f;bool dragging=false;
};
}
