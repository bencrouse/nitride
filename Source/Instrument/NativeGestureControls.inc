// Product copy of the approved native gestures. The older study remains a frozen
// comparison; both the AU and review app compile these controls through InstrumentView.cpp.
constexpr auto pi = juce::MathConstants<float>::pi;
juce::Colour chassis() { return juce::Colour(0xff151817); }
juce::Colour paper() { return juce::Colour(0xffe7e8e1); }
juce::Colour ink() { return juce::Colour(0xffe5dfd2); }
juce::Colour muted() { return juce::Colour(0xff969b8f); }
juce::Colour glass() { return juce::Colour(0xff0d100f); }
juce::Colour signal() { return juce::Colour(0xffd9d8ca); }
juce::Colour accent() { return juce::Colour(0xfff87946); }
juce::Font mono(float size = 10) { return juce::Font(juce::FontOptions("Menlo", size, juce::Font::plain)); }
void text(juce::Graphics& g, const juce::String& caption, juce::Rectangle<int> bounds,
          juce::Colour colour = ink(), float size = 10, juce::Justification alignment = juce::Justification::left)
{
    g.setColour(colour); g.setFont(mono(size)); g.drawText(caption, bounds, alignment, false);
}
double responseFromPosition(double position) { return .01 * std::pow(160.0, juce::jlimit(0.0, 1.0, position)); }
double responsePosition(double seconds) { return std::log(juce::jlimit(.01, 1.6, seconds) / .01) / std::log(160.0); }
using LiveState = Nitride::LiveState;

class PlayingField final : public juce::Component
{
public:
    explicit PlayingField(LiveState& state) : live(state)
    {
        setName("Coupling and Stress playing field"); setTitle(getName());
        setDescription("Touch to play. Drag horizontally for Coupling and vertically for Stress. Arrow keys shape; Space plays.");
        setWantsKeyboardFocus(true); setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
    std::function<void(float, float)> onShape;
    std::function<void()> onBegin, onEnd;
    std::function<void()> onGestureBegin, onGestureEnd;
    bool reducedMotion = false;
    void cancelGesture() { pointerDown=false;keyDown=false; }
    void setPosition(float c, float s) { coupling = juce::jlimit(0.0f, 1.0f, c); stress = juce::jlimit(0.0f, 1.0f, s); repaint(); }
    juce::Rectangle<float> fieldBounds() const { return getLocalBounds().toFloat().reduced(30, 35); }
    void paint(juce::Graphics& g) override
    {
        g.setColour(glass()); g.fillRoundedRectangle(getLocalBounds().toFloat(), 7);
        const auto area = fieldBounds();
        const auto point = juce::Point<float>(area.getX() + coupling * area.getWidth(), area.getBottom() - stress * area.getHeight());
        const auto level = live.level.load();
        const auto coherence = reducedMotion ? 1.0f : live.coherence.load();
        const juce::Graphics::ScopedSaveState saved(g); g.reduceClipRegion(area.toNearestInt());
        for (size_t band = 0; band < 6; ++band)
        {
            const auto phase = reducedMotion ? 0.0f : live.offsets[band].load(std::memory_order_relaxed);
            const auto link = reducedMotion ? .55f : live.links[band].load(std::memory_order_relaxed);
            const auto spread = (static_cast<float>(band) - 2.5f) * (7 + 13 * stress);
            juce::Path path;
            for (int sample = 0; sample <= 160; ++sample)
            {
                const auto t = static_cast<float>(sample) / 160;
                const auto envelope = std::sin(t * pi), distance = (t - coupling) * 6;
                const auto focus = 1 - std::exp(-distance * distance);
                const auto activity = reducedMotion ? 0.0f : level * (5 + 11 * stress);
                const auto fold = std::sin(t * pi * (2 + coupling * 3) + phase) * (8 + stress * 34 + activity) * (.6f + std::abs(link) * .4f);
                const auto x = area.getX() + area.getWidth() * t;
                const auto y = point.y + envelope * focus * (spread + fold * (1 - coherence * .25f));
                if (sample == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
            }
            g.setColour((band == 2 || band == 3 ? accent() : signal()).withAlpha(.42f + level * .35f));
            g.strokePath(path, juce::PathStrokeType(.8f + level * .45f));
        }
        g.setColour(accent().withAlpha(.16f + level * .15f)); g.fillEllipse(point.x - 14, point.y - 14, 28, 28);
        g.setColour(accent()); g.fillEllipse(point.x - 4, point.y - 4, 8, 8);
        g.setColour(paper()); g.drawEllipse(point.x - 8, point.y - 8, 16, 16, .7f);
    }
    void paintOverChildren(juce::Graphics& g) override
    {
        text(g, "STRESS", {16,11,140,16}, signal().withAlpha(.7f), 9);
        text(g, "NETWORK STATE", {getWidth()-174,11,156,16}, signal().withAlpha(.45f), 8, juce::Justification::right);
        text(g, "COUPLING", {16,getHeight()-25,160,16}, signal().withAlpha(.7f), 9);
        text(g, "TOUCH TO PLAY / DRAG TO SHAPE", {getWidth()-340,getHeight()-25,322,16}, signal().withAlpha(.5f), 8, juce::Justification::right);
        if (hasKeyboardFocus(true)) { g.setColour(accent()); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3), 5, 1.3f); }
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!e.mods.isLeftButtonDown()) return;
        grabKeyboardFocus(); if(onGestureBegin)onGestureBegin(); pointerDown = true; shapeAt(e.position); if (!keyDown && onBegin) onBegin();
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (pointerDown) shapeAt(e.position); }
    void mouseUp(const juce::MouseEvent&) override { endPointer(); }
    void focusLost(FocusChangeType) override { endPointer(); endKey(); repaint(); }
    void focusGained(FocusChangeType) override { repaint(); }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto code=key.getKeyCode();
        const auto changesShape=code==juce::KeyPress::leftKey||code==juce::KeyPress::rightKey||code==juce::KeyPress::upKey||code==juce::KeyPress::downKey;
        if(changesShape&&onGestureBegin)onGestureBegin();
        const auto step = key.getModifiers().isShiftDown() ? .002f : .02f;
        if (key.getKeyCode() == juce::KeyPress::leftKey) coupling -= step;
        else if (key.getKeyCode() == juce::KeyPress::rightKey) coupling += step;
        else if (key.getKeyCode() == juce::KeyPress::upKey) stress += step;
        else if (key.getKeyCode() == juce::KeyPress::downKey) stress -= step;
        else if (key.getKeyCode() == juce::KeyPress::spaceKey)
        {
            if (!keyDown) { keyDown = true; if (!pointerDown && onBegin) onBegin(); } return true;
        }
        else return false;
        setPosition(coupling, stress); if (onShape) onShape(coupling, stress); if(onGestureEnd)onGestureEnd(); return true;
    }
    bool keyStateChanged(bool) override
    {
        if (keyDown && !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey)) { endKey(); return true; } return false;
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Handler final : public juce::AccessibilityHandler
        {
        public:
            explicit Handler(PlayingField& owner) : AccessibilityHandler(owner, juce::AccessibilityRole::group,
                juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus, [&owner] { owner.grabKeyboardFocus(); })
                    .addAction(juce::AccessibilityActionType::press, [&owner] {
                        if (owner.keyDown) owner.endKey(); else { owner.keyDown = true; if (!owner.pointerDown && owner.onBegin) owner.onBegin(); }
                    })), field(owner) {}
            juce::String getDescription() const override { return "Coupling " + juce::String(field.coupling * 100,0) + " percent, Stress " + juce::String(field.stress * 100,0) + " percent"; }
            juce::String getHelp() const override { return field.getDescription(); }
        private: PlayingField& field;
        };
        return std::make_unique<Handler>(*this);
    }
private:
    void shapeAt(juce::Point<float> p) { const auto area = fieldBounds(); setPosition((p.x-area.getX())/area.getWidth(),1-(p.y-area.getY())/area.getHeight()); if(onShape)onShape(coupling,stress); }
    void endPointer() { if(pointerDown){pointerDown=false;if(!keyDown&&onEnd)onEnd();if(onGestureEnd)onGestureEnd();} }
    void endKey() { if(keyDown){keyDown=false;if(!pointerDown&&onEnd)onEnd();} }
    LiveState& live;
    float coupling=.65f, stress=.2f;
    bool pointerDown=false, keyDown=false;
};

class ResponseRibbon final : public juce::Component, private juce::Timer
{
public:
    explicit ResponseRibbon(LiveState& state) : live(state)
    {
        setName("Response timing gesture"); setTitle(getName());
        setDescription("Hold to set a response duration, or drag to stretch it. Arrow keys make finer adjustments.");
        setWantsKeyboardFocus(true); setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    std::function<void(double)> onResponse;
    std::function<void()> onBegin, onEnd;
    std::function<void()> onGestureBegin, onGestureEnd;
    bool reducedMotion=false;
    void cancelGesture() { holding=false;dragging=false;stopTimer(); }
    void setResponse(double seconds) { response=juce::jlimit(.01,1.6,seconds);repaint(); }
    void paint(juce::Graphics& g) override
    {
        text(g,"RESPONSE",{0,0,180,18},ink(),9);
        text(g,response<1?juce::String(response*1000,0)+" ms":juce::String(response,2)+" s",{getWidth()-140,0,140,18},ink(),11,juce::Justification::right);
        const auto extent=20+static_cast<float>(responsePosition(response))*static_cast<float>(getWidth()-40);
        const auto activity=reducedMotion?0.0f:live.level.load();
        for(int band=0;band<4;++band)
        {
            juce::Path tail;tail.startNewSubPath(20,46);
            tail.cubicTo(extent*.35f,46-static_cast<float>(band+1)*(4.5f+activity*3),extent*.72f,46+static_cast<float>(band+1)*(4.5f+activity*3),extent,46);
            g.setColour(ink().withAlpha(.25f+static_cast<float>(band)*.1f));g.strokePath(tail,juce::PathStrokeType(.8f));
        }
        g.setColour(accent());g.drawLine(extent-4,40,extent+2,46,1.5f);g.drawLine(extent+2,46,extent-4,52,1.5f);
        text(g,"FAST",{0,65,100,16},muted(),8);text(g,"HOLD TO SET TIME / DRAG TO STRETCH",{110,65,getWidth()-220,16},muted(),8,juce::Justification::centred);
        text(g,"SLOW",{getWidth()-100,65,100,16},muted(),8,juce::Justification::right);
        if(hasKeyboardFocus(true)){g.setColour(accent());g.drawRect(getLocalBounds().reduced(1),1);}
    }
    void mouseDown(const juce::MouseEvent& e) override {if(!e.mods.isLeftButtonDown())return;grabKeyboardFocus();if(onGestureBegin)onGestureBegin();started=juce::Time::getMillisecondCounterHiRes();holding=true;dragging=false;if(onBegin)onBegin();startTimerHz(30);}
    void mouseDrag(const juce::MouseEvent& e) override {if(!holding||e.getDistanceFromDragStart()<4)return;dragging=true;stopTimer();change(responseFromPosition(static_cast<double>(e.position.x-20)/static_cast<double>(getWidth()-40)));}
    void mouseUp(const juce::MouseEvent&) override {finish();}
    void focusLost(FocusChangeType) override {finish();repaint();}
    void focusGained(FocusChangeType) override {repaint();}
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto code=key.getKeyCode();if(code!=juce::KeyPress::leftKey&&code!=juce::KeyPress::rightKey&&code!=juce::KeyPress::upKey&&code!=juce::KeyPress::downKey)return false;
        if(onGestureBegin)onGestureBegin();const auto step=key.getModifiers().isShiftDown()?.005:.035;
        if(key.getKeyCode()==juce::KeyPress::leftKey||key.getKeyCode()==juce::KeyPress::downKey)change(responseFromPosition(responsePosition(response)-step));
        else if(key.getKeyCode()==juce::KeyPress::rightKey||key.getKeyCode()==juce::KeyPress::upKey)change(responseFromPosition(responsePosition(response)+step));if(onGestureEnd)onGestureEnd();return true;
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Value final : public juce::AccessibilityValueInterface
        {
        public:
            explicit Value(ResponseRibbon& owner):ribbon(owner){}
            bool isReadOnly()const override{return false;}
            double getCurrentValue()const override{return ribbon.response;}
            juce::String getCurrentValueAsString()const override{return juce::String(ribbon.response*1000,0)+" milliseconds";}
            void setValue(double v)override{if(ribbon.onGestureBegin)ribbon.onGestureBegin();ribbon.change(v);if(ribbon.onGestureEnd)ribbon.onGestureEnd();}
            void setValueAsString(const juce::String& v)override{setValue(v.getDoubleValue()/1000);}
            AccessibleValueRange getRange()const override{return{{.01,1.6},.01};}
        private:ResponseRibbon& ribbon;
        };
        return std::make_unique<juce::AccessibilityHandler>(*this,juce::AccessibilityRole::slider,
            juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus,[this]{grabKeyboardFocus();}),juce::AccessibilityHandler::Interfaces(std::make_unique<Value>(*this)));
    }
private:
    void change(double seconds){setResponse(seconds);if(onResponse)onResponse(response);}
    void finish(){if(!holding)return;if(!dragging)change((juce::Time::getMillisecondCounterHiRes()-started)/1000);holding=false;stopTimer();if(onEnd)onEnd();if(onGestureEnd)onGestureEnd();}
    void timerCallback()override{if(holding&&!dragging)change((juce::Time::getMillisecondCounterHiRes()-started)/1000);}
    LiveState& live;
    double response=.075,started=0;
    bool holding=false,dragging=false;
};
