#pragma once
#ifndef NITRIDE_HOST_VIEW
#define NITRIDE_HOST_VIEW 0
#endif
#ifndef NITRIDE_REFERENCE_CAPTURE
#define NITRIDE_REFERENCE_CAPTURE 0
#endif

// Private review view, included in GestureApplication.cpp after its shared native controls.
// The separate build target keeps the approved gesture/deformation studies intact.

class ReviewValue final : public juce::Component
{
public:
    ReviewValue(const juce::String& label, double low, double high, double initialValue, bool logarithmic = false)
        : caption(label), minimum(low), maximum(high), value(initialValue), defaultValue(initialValue), logScale(logarithmic)
    {
        setName(label); setTitle(label); setDescription("Drag vertically to change " + label + ". Arrows adjust. Double-click resets.");
        setWantsKeyboardFocus(true); setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    std::function<void()> onBegin, onEnd;
    std::function<void(double)> onChange;
    std::function<juce::String(double)> format;
    void setValue(double v) { value = juce::jlimit(minimum, maximum, v); repaint(); }
    double getValue() const { return value; }
    double position(double v) const { return logScale ? std::log(v / minimum) / std::log(maximum / minimum) : (v - minimum) / (maximum - minimum); }
    double fromPosition(double p) const { p = juce::jlimit(0.0, 1.0, p); return logScale ? minimum * std::pow(maximum / minimum, p) : minimum + p * (maximum - minimum); }
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff252b27)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 3);
        text(g, caption.toUpperCase(), { 10, 0, getWidth() / 2, getHeight() }, muted(), 9);
        text(g, format ? format(value) : juce::String(value, 2), { getWidth() / 2, 0, getWidth() / 2 - 10, getHeight() }, ink(), 12, juce::Justification::right);
        if (isMouseOverOrDragging() || hasKeyboardFocus(true)) { g.setColour(accent()); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 3, 1); }
    }
    void mouseDown(const juce::MouseEvent& e) override { grabKeyboardFocus(); origin = e.position; initial = position(value); if (onBegin) onBegin(); }
    void mouseDrag(const juce::MouseEvent& e) override { change(fromPosition(initial + (origin.y - e.position.y) / (e.mods.isShiftDown() ? 900 : 170))); }
    void mouseUp(const juce::MouseEvent&) override { if (onEnd) onEnd(); }
    void mouseDoubleClick(const juce::MouseEvent&) override { if (onBegin) onBegin(); change(defaultValue); if (onEnd) onEnd(); }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType) override { repaint(); if (onEnd) onEnd(); }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto code = key.getKeyCode();
        if (code != juce::KeyPress::upKey && code != juce::KeyPress::downKey && code != juce::KeyPress::leftKey && code != juce::KeyPress::rightKey) return false;
        if (onBegin) onBegin();
        change(fromPosition(position(value) + ((code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey) ? 1 : -1) * (key.getModifiers().isShiftDown() ? .002 : .02)));
        if (onEnd) onEnd(); return true;
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Value final : public juce::AccessibilityValueInterface
        {
        public:
            explicit Value(ReviewValue& v) : view(v) {}
            bool isReadOnly() const override { return false; }
            double getCurrentValue() const override { return view.value; }
            juce::String getCurrentValueAsString() const override { return view.format ? view.format(view.value) : juce::String(view.value); }
            void setValue(double v) override { if (view.onBegin) view.onBegin(); view.change(v); if (view.onEnd) view.onEnd(); }
            void setValueAsString(const juce::String& v) override { setValue(v.getDoubleValue()); }
            AccessibleValueRange getRange() const override { return {{view.minimum, view.maximum}, (view.maximum - view.minimum) / 100}; }
        private: ReviewValue& view;
        };
        return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::slider,
            juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus, [this] { grabKeyboardFocus(); }),
            juce::AccessibilityHandler::Interfaces(std::make_unique<Value>(*this)));
    }
private:
    void change(double v) { setValue(v); if (onChange) onChange(value); }
    juce::String caption;
    double minimum, maximum, value, defaultValue, initial = 0;
    bool logScale;
    juce::Point<float> origin;
};

class ReviewEnvelope final : public juce::Component
{
public:
    ReviewEnvelope() { setName("Amplitude envelope"); setTitle(getName()); setDescription("Drag attack, decay/sustain, and release points."); setWantsKeyboardFocus(true); }
    std::function<void()> onBegin, onEnd;
    std::function<void(std::array<double, 4>)> onChange;
    void setEnvelope(std::array<double, 4> v) { values = v; repaint(); }
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced(12, 15); }
    static double norm(double v, double low, double high) { return std::log(v / low) / std::log(high / low); }
    std::array<juce::Point<float>, 3> handles() const
    {
        const auto p = plot();
        const auto a = p.getX() + p.getWidth() * static_cast<float>(.08 + .22 * norm(values[0], .001, 4));
        return {{{a, p.getY() + 6}, {a + p.getWidth() * static_cast<float>(.1 + .13 * norm(values[1], .01, 4)), p.getBottom() - static_cast<float>(values[2]) * (p.getHeight() - 12)},
                 {p.getX() + p.getWidth() * static_cast<float>(.75 + .23 * norm(values[3], .02, 8)), p.getBottom()}}};
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(glass()); g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        const auto p = plot(); const auto points = handles();
        juce::Path path; path.startNewSubPath(p.getX(), p.getBottom()); path.lineTo(points[0]);
        path.cubicTo(points[0].x + 10, points[0].y, points[1].x - 10, points[1].y, points[1].x, points[1].y);
        path.lineTo(p.getX() + p.getWidth() * .67f, points[1].y);
        path.cubicTo(p.getX() + p.getWidth() * .72f, points[1].y, points[2].x - 12, p.getBottom(), points[2].x, p.getBottom());
        auto fill = path; fill.lineTo(p.getX(), p.getBottom()); fill.closeSubPath();
        g.setColour(accent().withAlpha(.065f)); g.fillPath(fill);
        g.setColour(signal().withAlpha(.75f)); g.strokePath(path, juce::PathStrokeType(1.2f));
        for (const auto point : points) { g.setColour(accent()); g.fillEllipse(point.x - 3, point.y - 3, 6, 6); }
        if (hasKeyboardFocus(true)) { g.setColour(accent()); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 3, 1); }
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        grabKeyboardFocus(); origin = e.position; initial = values;
        const auto points = handles(); float distance = 1.0e9f;
        for (int i = 0; i < 3; ++i) if (auto d = points[static_cast<size_t>(i)].getDistanceFrom(e.position); d < distance) { selected = i; distance = d; }
        if (onBegin) onBegin();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        const auto dx = static_cast<double>(e.position.x - origin.x) / plot().getWidth();
        if (selected == 0) values[0] = .001 * std::pow(4000.0, juce::jlimit(0.0, 1.0, norm(initial[0], .001, 4) + dx * 3));
        else if (selected == 1)
        {
            values[1] = .01 * std::pow(400.0, juce::jlimit(0.0, 1.0, norm(initial[1], .01, 4) + dx * 3));
            values[2] = juce::jlimit(0.0, 1.0, initial[2] + static_cast<double>(origin.y - e.position.y) / plot().getHeight());
        }
        else values[3] = .02 * std::pow(400.0, juce::jlimit(0.0, 1.0, norm(initial[3], .02, 8) + dx * 3));
        if (onChange) onChange(values); repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { if (onEnd) onEnd(); }
private:
    std::array<double, 4> values { .65, .7, .72, 1.4 }, initial = values;
    juce::Point<float> origin;
    int selected = 0;
};

class ReviewXY final : public juce::Component
{
public:
    enum class Kind { filter, motion, space };
    explicit ReviewXY(Kind k) : kind(k)
    {
        setName(k == Kind::filter ? "Filter cutoff and resonance" : k == Kind::motion ? "Motion rate and depth" : "Space mix and size");
        setTitle(getName()); setDescription("Drag to shape " + getName() + ". Arrow keys adjust."); setWantsKeyboardFocus(true);
    }
    std::function<void()> onBegin, onEnd;
    std::function<void(float, float)> onChange;
    void setPosition(float a, float b) { x = a; y = b; repaint(); }
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced(15, 12); }
    void paint(juce::Graphics& g) override
    {
        g.setColour(glass()); g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        const auto p = plot();
        const auto marker = juce::Point<float>(p.getX() + x * p.getWidth(), p.getBottom() - y * p.getHeight());
        g.setColour(signal().withAlpha(.13f)); g.drawLine(p.getX(), p.getCentreY(), p.getRight(), p.getCentreY(), .5f);
        juce::Path path;
        if (kind == Kind::filter)
        {
            path.startNewSubPath(p.getX(), p.getY() + p.getHeight() * .35f); path.lineTo(marker.x - 18, p.getY() + p.getHeight() * .35f);
            path.quadraticTo(marker.x, p.getY() - y * 12, marker.x + 10, p.getCentreY()); path.lineTo(std::min(p.getRight(), marker.x + 55), p.getBottom());
        }
        else if (kind == Kind::motion)
        {
            for (int i = 0; i <= 150; ++i)
            {
                const auto t = static_cast<float>(i) / 150;
                const auto point = juce::Point<float>(p.getX() + t * p.getWidth(), p.getCentreY() - std::sin(t * pi * (2 + x * 5)) * (5 + y * p.getHeight() * .38f));
                if (i == 0) path.startNewSubPath(point); else path.lineTo(point);
            }
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto r = (12 + static_cast<float>(i) * (9 + y * 12));
                g.setColour(signal().withAlpha(.1f + x * .13f));
                g.drawEllipse(p.getCentreX() - r * 1.8f, p.getCentreY() - r * .45f, r * 3.6f, r * .9f, .8f);
            }
        }
        g.setColour(signal().withAlpha(.65f)); g.strokePath(path, juce::PathStrokeType(1.2f));
        g.setColour(accent()); g.fillEllipse(marker.x - 3, marker.y - 3, 6, 6);
        if (hasKeyboardFocus(true)) { g.setColour(accent()); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 3, 1); }
    }
    void mouseDown(const juce::MouseEvent& e) override { grabKeyboardFocus(); if (onBegin) onBegin(); move(e.position); }
    void mouseDrag(const juce::MouseEvent& e) override { move(e.position); }
    void mouseUp(const juce::MouseEvent&) override { if (onEnd) onEnd(); }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto code = key.getKeyCode();
        if (code != juce::KeyPress::leftKey && code != juce::KeyPress::rightKey && code != juce::KeyPress::upKey && code != juce::KeyPress::downKey) return false;
        if (onBegin) onBegin();
        const auto step = key.getModifiers().isShiftDown() ? .002f : .02f;
        if (code == juce::KeyPress::leftKey) x -= step; if (code == juce::KeyPress::rightKey) x += step;
        if (code == juce::KeyPress::upKey) y += step; if (code == juce::KeyPress::downKey) y -= step;
        setPosition(juce::jlimit(0.0f, 1.0f, x), juce::jlimit(0.0f, 1.0f, y)); if (onChange) onChange(x, y);
        if (onEnd) onEnd(); return true;
    }
private:
    void move(juce::Point<float> point)
    {
        const auto p = plot(); setPosition(juce::jlimit(0.0f, 1.0f, (point.x - p.getX()) / p.getWidth()), juce::jlimit(0.0f, 1.0f, (p.getBottom() - point.y) / p.getHeight()));
        if (onChange) onChange(x, y);
    }
    Kind kind;
    float x = .5f, y = .2f;
};

class InstrumentReview final : public
#if NITRIDE_HOST_VIEW
    juce::Component,
#else
    juce::AudioAppComponent,
#endif
    private juce::Timer
{
public:
    enum Parameter { coupling, stress, response, fm, pitch, attack, decay, sustain, release, cutoff, resonance, motionRate, motionDepth, spaceMix, spaceSize, output, count };
    struct Patch
    {
        juce::String name;
        std::array<double, count> values;
        bool mono = false, chord = false;
    };

#if NITRIDE_HOST_VIEW
    InstrumentReview(Nitride::InstrumentSession& state, std::function<void()> audioSettings = {})
        : session(state), live(state.live), keyboardState(state.keyboard), showAudioSettings(std::move(audioSettings)),
#else
    InstrumentReview() :
#endif
        field(live), ribbon(live), keyboard(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
        fmValue("FM depth", 0, 4, 1.65), pitchValue("Pitch", -12, 12, 0),
        attackValue("Attack", .001, 4, .65, true), decayValue("Decay", .01, 4, .7, true),
        sustainValue("Sustain", 0, 1, .72), releaseValue("Release", .02, 8, 1.4, true),
        outputValue("Output", -24, 0, -6), filter(ReviewXY::Kind::filter), motion(ReviewXY::Kind::motion), space(ReviewXY::Kind::space)
    {
        setLookAndFeel(&lookAndFeel);
        lookAndFeel.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252b27));
        lookAndFeel.setColour(juce::TextButton::buttonOnColourId, accent());
        lookAndFeel.setColour(juce::TextButton::textColourOffId, ink());
        lookAndFeel.setColour(juce::TextButton::textColourOnId, glass());
        lookAndFeel.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff252b27));
        lookAndFeel.setColour(juce::ComboBox::textColourId, ink());
        lookAndFeel.setColour(juce::ComboBox::arrowColourId, muted());
        lookAndFeel.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff353d36));
        lookAndFeel.setColour(juce::PopupMenu::backgroundColourId, chassis());
        lookAndFeel.setColour(juce::PopupMenu::textColourId, ink());
        patches = {
            { "01 / Soft pad", {.55,.08,.35,1.65,0,.65,.7,.72,1.4,5500,.18,.25,.08,.25,.7,-6}, false, true },
            { "02 / Glass keys", {.60,.20,.035,2.7,0,.004,.65,.2,.45,12000,.1,.5,0,.12,.4,-6}, false, false },
            { "03 / Contact lead", {.75,.4,.025,2.7,0,.006,.16,.62,.16,15000,.2,.75,0,.08,.3,-6}, true, false },
            { "04 / Hard hit", {.90,.70,.012,2.7,0,.003,.16,0,.22,15000,.15,.5,0,.05,.3,-6}, false, false }
        };
        for (size_t i = 0; i < patches.size(); ++i) patchBox.addItem(patches[i].name, static_cast<int>(i) + 1);
        patchBox.setName("Starting patch");
        patchBox.onChange = [this] { if (patchBox.getSelectedId() > 0) {
            remember(); compare.setToggleState(false,juce::dontSendNotification); endAudition();
#if NITRIDE_HOST_VIEW
            if(patchBox.getSelectedId()<=4)session.program.store(patchBox.getSelectedId()-1);
#endif
            apply(patches[static_cast<size_t>(patchBox.getSelectedId() - 1)]); reference = current; if (hold.getToggleState()) startAudition(); } };
        addAndMakeVisible(patchBox);
        undo.setButtonText("UNDO"); undo.onClick = [this] { if (!history.empty()) { auto p = history.back(); history.pop_back(); compare.setToggleState(false,juce::dontSendNotification); apply(p); undo.setEnabled(!history.empty()); } };
        compare.setButtonText("COMPARE"); compare.setClickingTogglesState(true);
        compare.onClick = [this] { if (compare.getToggleState()) { edited = current; apply(reference); } else apply(edited); };
        keep.setButtonText("KEEP"); keep.onClick = [this] {
            auto p = current; p.name = "Kept " + juce::String(++keptCount); patches.push_back(p); patchBox.addItem(p.name, static_cast<int>(patches.size()));
            current.name = p.name; reference = current; patchBox.setSelectedId(static_cast<int>(patches.size()), juce::dontSendNotification);
            compare.setToggleState(false, juce::dontSendNotification); repaint();
        };
        hold.setButtonText("HOLD"); hold.setClickingTogglesState(true); hold.onClick = [this] { if (hold.getToggleState()) startAudition(true); else if (!touching) endAudition(); };
        settings.setButtonText("..."); settings.onClick = [this] { showSettings(); };
        for (auto* b : { &undo, &compare, &keep, &hold, &settings }) addAndMakeVisible(*b);
        keep.setTooltip("Keep the current patch in this review session");
        field.onShape = [this](float c, float s) { if (!touching) remember(); set(coupling, c); set(stress, s); };
        field.onBegin = [this] { touching = true; if(current.values[sustain]<.01&&!notes.isEmpty())endAudition(); startAudition(); };
        field.onEnd = [this] { touching = false; if (!hold.getToggleState()) endAudition(); };
        ribbon.onResponse = [this](double seconds) { set(response, seconds); };
        ribbon.onBegin = [this] { remember(); touching = true; startAudition(); };
        ribbon.onEnd = field.onEnd;
        addAndMakeVisible(field); addAndMakeVisible(ribbon);
        for (auto pair : { std::pair{&fmValue, fm}, {&pitchValue, pitch}, {&attackValue, attack}, {&decayValue, decay},
                           {&sustainValue, sustain}, {&releaseValue, release}, {&outputValue, output} })
        {
            auto* view = pair.first; const auto parameter = pair.second;
            view->onBegin = [this] { remember(); };
            view->onChange = [this, parameter](double value) { set(parameter, value); if (parameter >= attack && parameter <= release) syncEnvelope(); };
            addAndMakeVisible(*view);
        }
        fmValue.format = [](double v) { return juce::String(v, 2); };
        pitchValue.format = [](double v) { return (v > 0 ? "+" : "") + juce::String(v, 1) + " st"; };
        for (auto* value : { &attackValue, &decayValue, &releaseValue }) value->format = [](double v) { return v >= 1 ? juce::String(v, 2) + " s" : juce::String(v * 1000, 0) + " ms"; };
        sustainValue.format = [](double v) { return juce::String(v * 100, 0) + "%"; };
        outputValue.format = [](double v) { return juce::String(v, 1) + " dB"; };
        poly.setButtonText("POLY"); monoButton.setButtonText("MONO");
        poly.onClick = [this] { remember(); current.mono = false; publish(); syncModes(); };
        monoButton.onClick = [this] { remember(); current.mono = true; publish(); syncModes(); };
        addAndMakeVisible(poly); addAndMakeVisible(monoButton);
        envelope.onBegin = [this] { remember(); };
        envelope.onChange = [this](std::array<double, 4> v) { for (size_t i = 0; i < v.size(); ++i) set(static_cast<Parameter>(attack + i), v[i]); syncEnvelope(); };
        addAndMakeVisible(envelope);
        for (auto* panel : { &filter, &motion, &space }) { panel->onBegin = [this] { remember(); }; addAndMakeVisible(*panel); }
        filter.onChange = [this](float x, float y) { set(cutoff, 40 * std::pow(450.0, x)); set(resonance, y * .85); };
        motion.onChange = [this](float x, float y) { set(motionRate, .05 * std::pow(160.0, x)); set(motionDepth, y); };
        space.onChange = [this](float x, float y) { set(spaceMix, x * .65); set(spaceSize, y); };
        keyboard.setAvailableRange(48, 84); keyboard.setKeyPressBaseOctave(4); keyboard.setOctaveForMiddleC(4);
#if NITRIDE_HOST_VIEW
        keyboard.setMidiChannel(15);
#endif
        keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xff2b312b));
        keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, glass());
        keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, muted());
        keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent().withAlpha(.65f));
        addAndMakeVisible(keyboard);
#if NITRIDE_HOST_VIEW
        const auto initial = session.read();
        apply({initial.name,initial.values,initial.mono,initial.chord}); reference = current;
        patchBox.setSelectedId(session.program.load()+1,juce::dontSendNotification);
        undo.setEnabled(false); setSize(1200,840); startTimerHz(30);
#else
        midi.ensureSize(8192); apply(patches[0]); reference = current; patchBox.setSelectedId(1, juce::dontSendNotification);
        undo.setEnabled(false); setSize(1200, 840);
#if !NITRIDE_REFERENCE_CAPTURE
        setAudioChannels(0, 2); deviceManager.addMidiInputDeviceCallback({}, &collector); startTimerHz(30);
#endif
#endif
    }
    ~InstrumentReview() override {
        stopTimer(); endAudition();
#if !NITRIDE_HOST_VIEW
        deviceManager.removeMidiInputDeviceCallback({}, &collector); shutdownAudio();
#endif
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(chassis());
        g.setColour(juce::Colour(0xff1b201d)); g.fillRect(0, 92, 286, 424); g.fillRect(918, 92, 282, 424);
        g.setColour(juce::Colour(0xff30372f));
        g.drawHorizontalLine(92, 24, 1176); g.drawHorizontalLine(530, 24, 1176); g.drawHorizontalLine(726, 24, 1176);
        g.drawVerticalLine(286, 110, 510); g.drawVerticalLine(918, 110, 510);
        g.drawVerticalLine(392, 553, 710); g.drawVerticalLine(800, 553, 710);
        g.setColour(ink()); g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 36, juce::Font::bold)));
        g.drawText("nitride", 25, 23, 200, 43, juce::Justification::left);
        text(g, "PATCH", {300, 13, 120, 14}, muted(), 8);
        section(g, "SOUND", {24,112,238,18}); section(g, "NETWORK", {310,112,584,18}); section(g, "AMPLITUDE", {942,112,234,18});
        text(g, "COUPLING " + juce::String(current.values[coupling] * 100, 0) + "%   /   STRESS " + juce::String(current.values[stress] * 100, 0) + "%",
             {310,428,584,17}, muted(), 8);
        sourceWave(g);
        text(g, "SOURCE / SINE FM", {36,269,225,18}, muted(), 8);
        text(g, "PLAY MODE", {36,406,225,18}, muted(), 8);
        text(g, "DRAG VALUES TO SHAPE SOURCE", {36,487,225,14}, muted(), 7);
        section(g, "TONE", {24,550,344,18}); text(g, "LP / 12 dB", {205,550,163,18}, muted(), 8, juce::Justification::right);
        section(g, "MOTION", {416,550,360,18}); text(g, "SINE > COUPLING", {580,550,196,18}, muted(), 8, juce::Justification::right);
        section(g, "SPACE", {824,550,352,18}); text(g, "DIFFUSION", {1000,550,176,18}, muted(), 8, juce::Justification::right);
        text(g, formatCutoff(current.values[cutoff]) + "  /  RES " + juce::String(current.values[resonance] * 100, 0) + "%", {36,692,332,17}, muted(), 9);
        text(g, juce::String(current.values[motionRate],2) + " Hz  /  DEPTH " + juce::String(current.values[motionDepth] * 100,0) + "%", {428,692,348,17}, muted(), 9);
        text(g, "MIX " + juce::String(current.values[spaceMix]*100,0) + "%  /  SIZE " + juce::String(current.values[spaceSize]*100,0) + "%", {836,692,340,17}, muted(), 9);
        text(g,
#if NITRIDE_HOST_VIEW
            "NATIVE INSTRUMENT",
#else
            "INSTRUMENT STUDY 08",
#endif
            {26,808,260,17}, muted(), 8);
        text(g, juce::String(live.voices.load()) + " VOICES", {600,808,146,17}, muted(), 8);
        text(g, deviceStatus, {250,808,340,17}, muted(), 8);
        const auto peak = live.peak.load();
        for (int i = 0; i < 14; ++i) { g.setColour(peak > static_cast<float>(i) / 20 ? accent().withAlpha(.8f) : juce::Colour(0xff30372f)); g.fillRect(790+i*8,813,5,10); }
    }
    void resized() override
    {
        patchBox.setBounds(300,33,330,34); undo.setBounds(668,33,70,34); compare.setBounds(746,33,94,34);
        keep.setBounds(848,33,70,34); hold.setBounds(968,33,80,34); settings.setBounds(1102,33,74,34);
        field.setBounds(310,145,584,274); ribbon.setBounds(310,448,584,82);
        fmValue.setBounds(36,302,226,34); pitchValue.setBounds(36,352,226,34);
        poly.setBounds(36,435,109,30); monoButton.setBounds(153,435,109,30);
        envelope.setBounds(942,145,234,147);
        attackValue.setBounds(942,309,234,34); decayValue.setBounds(942,351,234,34);
        sustainValue.setBounds(942,393,234,34); releaseValue.setBounds(942,435,234,34);
        filter.setBounds(36,586,332,96); motion.setBounds(428,586,348,96); space.setBounds(836,586,340,96);
        keyboard.setBounds(26,741,1148,53); keyboard.setKeyWidth(1148.0f/22);
        outputValue.setBounds(914,803,262,28);
    }

#if !NITRIDE_HOST_VIEW
    void prepareToPlay(int, double sampleRate) override
    {
        rate = sampleRate; engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);
        setEngine(); engine.prepare(sampleRate); collector.reset(sampleRate); reverb.setSampleRate(sampleRate); reverb.reset();
        tone = {}; lfoPhase = 0; cutoffSmooth.reset(sampleRate,.025); resSmooth.reset(sampleRate,.025); gainSmooth.reset(sampleRate,.025);
        cutoffSmooth.setCurrentAndTargetValue(static_cast<float>(audio[cutoff].load())); resSmooth.setCurrentAndTargetValue(static_cast<float>(audio[resonance].load()));
        gainSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(audio[output].load())));
        std::array<bool,128> restored{};
        for(size_t i=0;i<held.size();++i)if(held[i]&&!restored[i%128]){restored[i%128]=true;engine.noteOn(static_cast<int>(i%128),.74f,monoAudio.load()?SoundStudies::Articulation::lead:SoundStudies::Articulation::pad);}
    }
    void releaseResources() override {}
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        const juce::ScopedNoDenormals noDenormals; info.clearActiveBufferRegion(); if (info.buffer->getNumChannels()==0) return;
        setEngine(); midi.clear(); collector.removeNextBlockOfMessages(midi,info.numSamples); keyboardState.processNextMidiBuffer(midi,0,info.numSamples,true);
        auto* left=info.buffer->getWritePointer(0,info.startSample); auto* right=info.buffer->getWritePointer(info.buffer->getNumChannels()>1?1:0,info.startSample);
        int offset=0;
        for (const auto metadata:midi)
        {
            const auto pos=juce::jlimit(offset,info.numSamples,metadata.samplePosition); renderRange(left+offset,right+offset,pos-offset);
            const auto message=metadata.getMessage();
            if (message.isNoteOn()||message.isNoteOff())
            {
                const auto n=message.getNoteNumber(), ch=message.getChannel(); held[static_cast<size_t>((ch-1)*128+n)]=message.isNoteOn();
                if(message.isNoteOn()) { bool capture=false; if(ch==16) for(int c=0;c<15;++c) capture|=held[static_cast<size_t>(c*128+n)]; if(!capture) engine.noteOn(n,message.getFloatVelocity(),monoAudio.load()?SoundStudies::Articulation::lead:SoundStudies::Articulation::pad); }
                else { bool other=false; for(int c=0;c<16;++c) other|=held[static_cast<size_t>(c*128+n)]; if(!other) engine.noteOff(n); }
            }
            else if(message.isPitchWheel()) { wheel=static_cast<double>(message.getPitchWheelValue()-8192)*2/8192; engine.setPitchBend(static_cast<float>(audio[pitch].load()+wheel)); }
            else if(message.isAllNotesOff()||message.isAllSoundOff()) {held={};engine.allNotesOff();}
            offset=pos;
        }
        renderRange(left+offset,right+offset,info.numSamples-offset);
        cutoffSmooth.setTargetValue(static_cast<float>(audio[cutoff].load()));resSmooth.setTargetValue(static_cast<float>(audio[resonance].load()));
        for(int i=0;i<info.numSamples;++i)
        {
            const auto frequency=std::min(static_cast<double>(cutoffSmooth.getNextValue()),rate*.42);
            const auto g=std::tan(juce::MathConstants<double>::pi*frequency/rate),k=2.0-1.85*resSmooth.getNextValue(),a=1.0/(1.0+g*(g+k));
            const auto sample=[&](double input,int channel){auto& t=tone[static_cast<size_t>(channel)];const auto v1=a*(t[0]+g*(input-t[1]));const auto v2=t[1]+g*v1;t[0]=2*v1-t[0];t[1]=2*v2-t[1];return static_cast<float>(v2);};
            left[i]=sample(left[i],0);if(right!=left)right[i]=sample(right[i],1);
        }
        juce::Reverb::Parameters rp;rp.roomSize=static_cast<float>(audio[spaceSize].load());rp.damping=.45f;rp.width=1;
        rp.wetLevel=static_cast<float>(audio[spaceMix].load())*.45f;rp.dryLevel=1-static_cast<float>(audio[spaceMix].load())*.35f;reverb.setParameters(rp);
        if(right!=left)reverb.processStereo(left,right,info.numSamples);else reverb.processMono(left,info.numSamples);
        gainSmooth.setTargetValue(juce::Decibels::decibelsToGain(static_cast<float>(audio[output].load())));float peak=0;
        for(int i=0;i<info.numSamples;++i){const auto gain=gainSmooth.getNextValue();left[i]=juce::jlimit(-.95f,.95f,left[i]*gain);if(right!=left)right[i]=juce::jlimit(-.95f,.95f,right[i]*gain);peak=std::max(peak,std::abs(left[i]));}
        int external=0;for(size_t i=0;i<15*128;++i)external+=held[i]?1:0;live.externalKeys.store(external);live.peak.store(peak);live.faults.store(engine.getNumericalFaults());live.publish(engine.getNetworkSnapshot());
    }
#endif

    void beginPreview() { hold.setToggleState(true,juce::dontSendNotification);startAudition(); }
#if !NITRIDE_HOST_VIEW
    void beginAudioCheck() { apply(patches[0]);beginPreview(); }
    void changeAudioCheck() { set(coupling,.95);set(stress,.8);set(response,.035);set(cutoff,12000);set(motionDepth,.25);field.setPosition(.95f,.8f);ribbon.setResponse(.035); }
    bool finishAudioCheck()
    {
        const auto good=live.peak.load()>.0001f&&live.faults.load()==0&&deviceManager.getXRunCount()==0&&deviceManager.getCpuUsage()<.9;
        std::cout<<"Instrument review audio: peak "<<live.peak.load()<<", CPU "<<deviceManager.getCpuUsage()*100<<"%, xruns "<<deviceManager.getXRunCount()<<", faults "<<live.faults.load()<<'\n';
        hold.setToggleState(false,juce::dontSendNotification);endAudition();return good;
    }
    bool checkReleased() {const auto released=live.voices.load()==0;std::cout<<"Instrument voice release: "<<released<<'\n';return released;}
#endif
    bool checkInteractions()
    {
        apply(patches[0]);reference=current;const auto initial=current.values[coupling];
        field.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));const auto changed=current.values[coupling]>initial;
        undo.onClick();const auto undone=std::abs(current.values[coupling]-initial)<.001;
        fmValue.getAccessibilityHandler()->getValueInterface()->setValue(3.2);const auto source=std::abs(current.values[fm]-3.2)<.001;
        envelope.setEnvelope({.1,.2,.5,.4});envelope.onChange({.1,.2,.5,.4});const auto env=std::abs(current.values[attack]-.1)<.001;
        filter.onChange(.7f,.4f);motion.onChange(.4f,.2f);space.onChange(.3f,.7f);const auto support=current.values[cutoff]>1000&&current.values[motionDepth]>.1&&current.values[spaceMix]>.1;
        keep.onClick();const auto kept=patches.size()==5&&patchBox.getSelectedId()==5;
        const auto saved=current;set(stress,.8);compare.setToggleState(true,juce::dontSendNotification);compare.onClick();const auto compared=std::abs(current.values[stress]-saved.values[stress])<.001;
        compare.setToggleState(false,juce::dontSendNotification);compare.onClick();const auto returned=std::abs(current.values[stress]-.8)<.001;
        field.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey));const auto played=!notes.isEmpty();field.keyStateChanged(false);const auto stopped=notes.isEmpty();
        std::cout<<"Review interaction checks: field "<<changed<<", undo "<<undone<<", source "<<source<<", envelope "<<env<<", support "<<support<<", keep "<<kept<<", compare "<<compared<<'/'<<returned<<", play "<<played<<'/'<<stopped<<'\n';
        return changed&&undone&&source&&env&&support&&kept&&compared&&returned&&played&&stopped;
    }

#if NITRIDE_REFERENCE_CAPTURE
    std::array<std::vector<float>,2> renderReference(int program, double sampleRate, int samples, int blockSize)
    {
        apply(patches[static_cast<size_t>(program)]);
        prepareToPlay(blockSize,sampleRate);
        const auto shape=current.mono?SoundStudies::Articulation::lead:SoundStudies::Articulation::pad;
        const auto velocity=juce::MidiMessage::noteOn(1,60,.74f).getFloatVelocity();
        if(current.chord&&!current.mono)for(const auto note:{48,55,59,62})engine.noteOn(note,velocity,shape);
        else engine.noteOn(60,velocity,shape);
        std::array<std::vector<float>,2> output{std::vector<float>(static_cast<size_t>(samples)),std::vector<float>(static_cast<size_t>(samples))};
        for(int offset=0;offset<samples;offset+=blockSize)
        {
            const auto size=std::min(blockSize,samples-offset);
            float* pointers[]{output[0].data()+offset,output[1].data()+offset};
            juce::AudioBuffer<float> block(pointers,2,size);
            getNextAudioBlock({&block,0,size});
        }
        return output;
    }
#endif

private:
    static void section(juce::Graphics& g,const juce::String& label,juce::Rectangle<int> r){g.setColour(ink());g.setFont(juce::Font(juce::FontOptions("Helvetica Neue",11,juce::Font::bold)));g.drawText(label,r,juce::Justification::left);}
    static juce::String formatCutoff(double hz){return hz>=1000?juce::String(hz/1000,2)+" kHz":juce::String(hz,0)+" Hz";}
    void sourceWave(juce::Graphics& g)
    {
        const juce::Rectangle<float> p{36,151,226,103};g.setColour(glass());g.fillRoundedRectangle(p,4);juce::Path wave;
        for(int i=0;i<=140;++i){const auto t=static_cast<float>(i)/140;const auto phase=t*pi*4;const auto sample=std::sin(phase+static_cast<float>(current.values[fm])*std::sin(phase*2));const auto x=p.getX()+12+t*(p.getWidth()-24),y=p.getCentreY()-sample*29;if(i==0)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}
        g.setColour(signal().withAlpha(.7f));g.strokePath(wave,juce::PathStrokeType(1.1f));
    }
    void publish(){
#if NITRIDE_HOST_VIEW
        session.apply({current.name,current.values,current.mono,current.chord}); lastRevision=session.revision.load();
#else
        for(size_t i=0;i<count;++i)audio[i].store(current.values[i]);monoAudio.store(current.mono);
#endif
        repaint();
    }
    void set(Parameter p,double value){current.values[static_cast<size_t>(p)]=value;
#if NITRIDE_HOST_VIEW
        session.set(static_cast<Nitride::Parameter>(p),value);lastRevision=session.revision.load();
#else
        audio[static_cast<size_t>(p)].store(value);
#endif
        repaint();
    }
    void remember(){if(!applying){history.push_back(current);if(history.size()>40)history.erase(history.begin());undo.setEnabled(true);}}
    void apply(const Patch& patch)
    {
        applying=true;current=patch;publish();field.setPosition(static_cast<float>(current.values[coupling]),static_cast<float>(current.values[stress]));ribbon.setResponse(current.values[response]);
        fmValue.setValue(current.values[fm]);pitchValue.setValue(current.values[pitch]);outputValue.setValue(current.values[output]);syncEnvelope();syncModes();
        filter.setPosition(static_cast<float>(std::log(current.values[cutoff]/40)/std::log(450.0)),static_cast<float>(current.values[resonance]/.85));
        motion.setPosition(static_cast<float>(std::log(current.values[motionRate]/.05)/std::log(160.0)),static_cast<float>(current.values[motionDepth]));
        space.setPosition(static_cast<float>(current.values[spaceMix]/.65),static_cast<float>(current.values[spaceSize]));applying=false;
        for(size_t i=0;i<patches.size();++i)if(patches[i].name==current.name){patchBox.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);break;}
    }
    void syncEnvelope(){envelope.setEnvelope({current.values[attack],current.values[decay],current.values[sustain],current.values[release]});attackValue.setValue(current.values[attack]);decayValue.setValue(current.values[decay]);sustainValue.setValue(current.values[sustain]);releaseValue.setValue(current.values[release]);}
    void syncModes(){poly.setToggleState(!current.mono,juce::dontSendNotification);monoButton.setToggleState(current.mono,juce::dontSendNotification);}
#if !NITRIDE_HOST_VIEW
    void setEngine(){engine.setFmIndex(audio[fm].load());engine.setAmplitudeEnvelope(audio[attack].load(),audio[decay].load(),static_cast<float>(audio[sustain].load()),audio[release].load());engine.setNetworkDimensions(static_cast<float>(audio[coupling].load()),static_cast<float>(audio[stress].load()),audio[response].load());engine.setPitchBend(static_cast<float>(audio[pitch].load()+wheel));}
    void renderRange(float* left,float* right,int samples)
    {
        int offset=0;while(offset<samples){const auto size=std::min(32,samples-offset);const auto mod=std::sin(lfoPhase)*audio[motionDepth].load()*.18;engine.setNetworkDimensions(static_cast<float>(juce::jlimit(0.0,1.0,audio[coupling].load()+mod)),static_cast<float>(audio[stress].load()),audio[response].load());engine.render(left+offset,right+offset,size);lfoPhase+=juce::MathConstants<double>::twoPi*audio[motionRate].load()*size/rate;lfoPhase=std::fmod(lfoPhase,juce::MathConstants<double>::twoPi);offset+=size;}
    }
#endif
    void startAudition(bool capture=false)
    {
        if(!notes.isEmpty())return;
        if(capture)for(int n=0;n<128;++n)for(int c=1;c<16;++c)if(keyboardState.isNoteOn(c,n)){notes.add(n);break;}
        if(notes.isEmpty()){if(live.externalKeys.load()!=0)return;notes=current.chord&&!current.mono?juce::Array<int>{48,55,59,62}:juce::Array<int>{60};}
        for(const auto n:notes)keyboardState.noteOn(16,n,.74f);
    }
    void endAudition(){for(const auto n:notes)keyboardState.noteOff(16,n,0);notes.clear();}
    void showSettings()
    {
#if NITRIDE_HOST_VIEW
        juce::PopupMenu menu;
        if(showAudioSettings)menu.addItem(1,"Audio / MIDI...");
        menu.addItem(2,"Reduced motion",true,field.reducedMotion);
        menu.addItem(3,"About Nitride");
        const juce::Component::SafePointer<InstrumentReview> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),[safe](int choice){
            if(safe==nullptr)return;
            if(choice==1&&safe->showAudioSettings)safe->showAudioSettings();
            else if(choice==2){safe->field.reducedMotion=!safe->field.reducedMotion;safe->ribbon.reducedMotion=safe->field.reducedMotion;}
            else if(choice==3)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Nitride / first playable instrument",
                "The native field, source, envelope, tone, motion and space are connected to the instrument's audio processor. "
                "Keep/Compare/Undo are local to the current editor. Host automation and complete session/preset workflow are the next milestone.");
        });
#else
        juce::PopupMenu menu;menu.addItem(1,"Audio / MIDI...");menu.addItem(2,"Reduced motion",true,field.reducedMotion);menu.addItem(3,"About this review");
        const juce::Component::SafePointer<InstrumentReview> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),[safe](int choice){if(safe==nullptr)return;if(choice==1){auto selector=std::make_unique<juce::AudioDeviceSelectorComponent>(safe->deviceManager,0,0,0,2,true,false,true,false);selector->setSize(500,430);juce::DialogWindow::LaunchOptions dialog;dialog.content.setOwned(selector.release());dialog.dialogTitle="Nitride / Audio and MIDI";dialog.dialogBackgroundColour=chassis();dialog.componentToCentreAround=safe.getComponent();dialog.useNativeTitleBar=true;dialog.launchAsync();}else if(choice==2){safe->field.reducedMotion=!safe->field.reducedMotion;safe->ribbon.reducedMotion=safe->field.reducedMotion;}else if(choice==3)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Instrument review 08","Playable native layout review. The approved field and Response gesture are retained. Source depth, pitch, amplitude envelope, tone, motion and space are audible preview controls. Keep stores patches for this session; Compare returns to the last starting/kept patch. The product AU is a separate milestone.");});
#endif
    }
    void timerCallback() override{
#if NITRIDE_HOST_VIEW
        const auto revision=session.revision.load(std::memory_order_acquire);
        if(revision!=lastRevision){const auto patch=session.read();apply({patch.name,patch.values,patch.mono,patch.chord});}
        deviceStatus="AUDIO PROCESSOR";
#else
        if(auto* device=deviceManager.getCurrentAudioDevice())deviceStatus=device->getName()+" / "+juce::String(device->getCurrentSampleRate(),0)+" Hz";else deviceStatus="NO AUDIO DEVICE";
#endif
        field.repaint();ribbon.repaint();repaint(24,803,1152,34);
    }

    juce::LookAndFeel_V4 lookAndFeel;
#if NITRIDE_HOST_VIEW
    Nitride::InstrumentSession& session;
    LiveState& live;
    juce::MidiKeyboardState& keyboardState;
    std::function<void()> showAudioSettings;
    std::uint64_t lastRevision=0;
#else
    SoundStudies::Engine engine;
    LiveState live;
#endif
    PlayingField field;
    ResponseRibbon ribbon;
#if !NITRIDE_HOST_VIEW
    juce::MidiKeyboardState keyboardState;
#endif
    juce::MidiKeyboardComponent keyboard;
#if !NITRIDE_HOST_VIEW
    juce::MidiMessageCollector collector;
    juce::MidiBuffer midi;
#endif
    ReviewValue fmValue,pitchValue,attackValue,decayValue,sustainValue,releaseValue,outputValue;
    ReviewEnvelope envelope;
    ReviewXY filter,motion,space;
    juce::ComboBox patchBox;
    juce::TextButton undo,compare,keep,hold,settings,poly,monoButton;
    std::vector<Patch> patches,history;
    Patch current{},reference{},edited{};
#if !NITRIDE_HOST_VIEW
    std::array<std::atomic<double>,count> audio {};
    std::atomic<bool> monoAudio {false};
    std::array<bool,16*128> held {};
#endif
    juce::Array<int> notes;
#if !NITRIDE_HOST_VIEW
    juce::Reverb reverb;
    std::array<std::array<double,2>,2> tone {};
    juce::SmoothedValue<float> cutoffSmooth,resSmooth,gainSmooth;
    double rate=48000,lfoPhase=0,wheel=0;
#endif
    bool touching=false,applying=false;
    int keptCount=0;
    juce::String deviceStatus;
};
