#include "StudyEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <cmath>
#include <iostream>

#ifndef NITRIDE_DEFORMATION_STUDY
#define NITRIDE_DEFORMATION_STUDY 0
#endif
#ifndef NITRIDE_INSTRUMENT_REVIEW
#define NITRIDE_INSTRUMENT_REVIEW 0
#endif

namespace
{
constexpr bool deformationStudy = NITRIDE_DEFORMATION_STUDY != 0;
constexpr bool instrumentReview = NITRIDE_INSTRUMENT_REVIEW != 0;
constexpr const char* applicationName = instrumentReview ? "Nitride Instrument Review"
    : deformationStudy ? "Nitride Deformation Study" : "Nitride Gesture Study";
constexpr auto pi = juce::MathConstants<float>::pi;
juce::Colour chassis() { return juce::Colour(instrumentReview ? 0xff151817 : 0xffdadbd4); }
juce::Colour paper() { return juce::Colour(0xffe7e8e1); }
juce::Colour ink() { return juce::Colour(instrumentReview ? 0xffe5dfd2 : 0xff202321); }
juce::Colour muted() { return juce::Colour(instrumentReview ? 0xff969b8f : 0xff646b63); }
juce::Colour glass() { return juce::Colour(instrumentReview ? 0xff0d100f : 0xff141b18); }
juce::Colour signal() { return juce::Colour(instrumentReview ? 0xffd9d8ca : 0xffb4d7be); }
juce::Colour accent() { return juce::Colour(instrumentReview ? 0xfff87946 : 0xfff15d3b); }
juce::Font mono(float size = 10) { return juce::Font(juce::FontOptions("Menlo", size, juce::Font::plain)); }

void text(juce::Graphics& g, const juce::String& caption, juce::Rectangle<int> bounds,
          juce::Colour colour = ink(), float size = 10,
          juce::Justification alignment = juce::Justification::left)
{
    g.setColour(colour);
    g.setFont(mono(size));
    g.drawText(caption, bounds, alignment, false);
}

double responseFromPosition(double position)
{
    return 0.01 * std::pow(160.0, juce::jlimit(0.0, 1.0, position));
}

double responsePosition(double seconds)
{
    return std::log(juce::jlimit(0.01, 1.6, seconds) / 0.01) / std::log(160.0);
}

struct LiveState
{
    std::array<std::atomic<float>, 6> offsets {};
    std::array<std::atomic<float>, 9> links {};
    std::atomic<float> level { 0 }, coherence { 1 }, peak { 0 };
    std::atomic<int> voices { 0 }, externalKeys { 0 };
    std::atomic<std::uint64_t> faults { 0 };

    void publish(const SoundStudies::Engine::NetworkSnapshot& snapshot)
    {
        for (size_t i = 0; i < offsets.size(); ++i) offsets[i].store(snapshot.offsets[i], std::memory_order_relaxed);
        for (size_t i = 0; i < links.size(); ++i) links[i].store(snapshot.links[i], std::memory_order_relaxed);
        level.store(snapshot.level);
        coherence.store(snapshot.coherence);
        voices.store(snapshot.activeVoices);
    }
};

class PlayingField final : public juce::Component
{
public:
    explicit PlayingField(LiveState& state) : live(state)
    {
        setName("Coupling and Stress playing field");
        setTitle(getName());
        setDescription("Touch to play. Drag horizontally for Coupling and vertically for Stress. Arrow keys shape; Space plays.");
        setWantsKeyboardFocus(true);
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    std::function<void(float, float)> onShape;
    std::function<void()> onBegin, onEnd;
    bool reducedMotion = false;

    void setPosition(float c, float s)
    {
        coupling = juce::jlimit(0.0f, 1.0f, c);
        stress = juce::jlimit(0.0f, 1.0f, s);
        repaint();
    }

    juce::Rectangle<float> fieldBounds() const
    {
        return getLocalBounds().toFloat().reduced(30, 35);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(glass());
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 7);
        const auto area = fieldBounds();
        const auto point = juce::Point<float>(area.getX() + coupling * area.getWidth(), area.getBottom() - stress * area.getHeight());
        const auto level = live.level.load();
        const auto coherence = reducedMotion ? 1.0f : live.coherence.load();
        // Six curves report oscillator-offset and link state, rather than a simulated FFT.
        const juce::Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(area.toNearestInt());
        for (size_t band = 0; band < 6; ++band)
        {
            const auto phase = reducedMotion ? 0.0f : live.offsets[band].load(std::memory_order_relaxed);
            const auto link = reducedMotion ? 0.55f : live.links[band].load(std::memory_order_relaxed);
            const auto spread = (static_cast<float>(band) - 2.5f) * (7.0f + 13.0f * stress);
            juce::Path path;
            for (int sample = 0; sample <= 160; ++sample)
            {
                const auto t = static_cast<float>(sample) / 160;
                const auto envelope = std::sin(t * pi);
                const auto distance = (t - coupling) * 6.0f;
                const auto focus = 1.0f - std::exp(-distance * distance);
                const auto x = area.getX() + area.getWidth() * t;
                const auto activity = reducedMotion ? 0.0f : level * (5.0f + 11.0f * stress);
                const auto fold = std::sin(t * pi * (2.0f + coupling * 3.0f) + phase)
                    * (8.0f + stress * 34.0f + activity) * (0.6f + std::abs(link) * 0.4f);
                const auto y = point.y + envelope * focus * (spread + fold * (1.0f - coherence * 0.25f));
                if (sample == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
            }
            g.setColour((band == 2 || band == 3 ? accent() : signal()).withAlpha(0.42f + level * 0.35f));
            g.strokePath(path, juce::PathStrokeType(0.8f + level * 0.45f));
        }
        g.setColour(accent().withAlpha(0.16f + level * 0.15f));
        g.fillEllipse(point.x - 14, point.y - 14, 28, 28);
        g.setColour(accent());
        g.fillEllipse(point.x - 4, point.y - 4, 8, 8);
        g.setColour(paper());
        g.drawEllipse(point.x - 8, point.y - 8, 16, 16, 0.7f);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        text(g, "STRESS", { 16, 11, 140, 16 }, signal().withAlpha(0.7f), 9);
        text(g, "NETWORK STATE", { getWidth() - 174, 11, 156, 16 }, signal().withAlpha(0.45f), 8, juce::Justification::right);
        text(g, "COUPLING", { 16, getHeight() - 25, 160, 16 }, signal().withAlpha(0.7f), 9);
        text(g, "TOUCH TO PLAY / DRAG TO SHAPE", { getWidth() - 340, getHeight() - 25, 322, 16 }, signal().withAlpha(0.5f), 8, juce::Justification::right);
        if (hasKeyboardFocus(true))
        {
            g.setColour(accent());
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3), 5, 1.3f);
        }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (!event.mods.isLeftButtonDown()) return;
        grabKeyboardFocus();
        pointerDown = true;
        shapeAt(event.position);
        if (!keyDown && onBegin) onBegin();
    }
    void mouseDrag(const juce::MouseEvent& event) override { if (pointerDown) shapeAt(event.position); }
    void mouseUp(const juce::MouseEvent&) override { endPointer(); }
    void focusLost(FocusChangeType) override { endPointer(); endKey(); repaint(); }
    void focusGained(FocusChangeType) override { repaint(); }

    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto step = key.getModifiers().isShiftDown() ? 0.002f : 0.02f;
        if (key.getKeyCode() == juce::KeyPress::leftKey) coupling -= step;
        else if (key.getKeyCode() == juce::KeyPress::rightKey) coupling += step;
        else if (key.getKeyCode() == juce::KeyPress::upKey) stress += step;
        else if (key.getKeyCode() == juce::KeyPress::downKey) stress -= step;
        else if (key.getKeyCode() == juce::KeyPress::spaceKey)
        {
            if (!keyDown) { keyDown = true; if (!pointerDown && onBegin) onBegin(); }
            return true;
        }
        else return false;
        setPosition(coupling, stress);
        if (onShape) onShape(coupling, stress);
        return true;
    }
    bool keyStateChanged(bool) override
    {
        if (keyDown && !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey)) { endKey(); return true; }
        return false;
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Handler final : public juce::AccessibilityHandler
        {
        public:
            explicit Handler(PlayingField& owner)
                : AccessibilityHandler(owner, juce::AccessibilityRole::group,
                    juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus, [&owner] { owner.grabKeyboardFocus(); })
                        .addAction(juce::AccessibilityActionType::press, [&owner] {
                            if (owner.keyDown) owner.endKey();
                            else { owner.keyDown = true; if (!owner.pointerDown && owner.onBegin) owner.onBegin(); }
                        })), field(owner) {}
            juce::String getDescription() const override
            {
                return "Coupling " + juce::String(field.coupling * 100, 0) + " percent, Stress " + juce::String(field.stress * 100, 0) + " percent";
            }
            juce::String getHelp() const override { return field.getDescription(); }
        private:
            PlayingField& field;
        };
        return std::make_unique<Handler>(*this);
    }

private:
    void shapeAt(juce::Point<float> point)
    {
        const auto area = fieldBounds();
        setPosition((point.x - area.getX()) / area.getWidth(), 1.0f - (point.y - area.getY()) / area.getHeight());
        if (onShape) onShape(coupling, stress);
    }
    void endPointer() { if (pointerDown) { pointerDown = false; if (!keyDown && onEnd) onEnd(); } }
    void endKey() { if (keyDown) { keyDown = false; if (!pointerDown && onEnd) onEnd(); } }
    LiveState& live;
    float coupling = 0.65f, stress = 0.2f;
    bool pointerDown = false, keyDown = false;
};

#if NITRIDE_DEFORMATION_STUDY
class DeformingBody final : public juce::Component
{
public:
    enum class Grip { none, body, left, right, top, bottom };

    explicit DeformingBody(LiveState& state) : live(state)
    {
        setName("Deformable sound object"); setTitle(getName());
        setDescription("Pinch or stretch the ends for Coupling. Pull the top or bottom rim for Stress. Arrows shape, Space plays, Escape cancels a drag.");
        setWantsKeyboardFocus(true);
    }
    std::function<void(float, float)> onShape;
    std::function<void()> onBegin, onEnd;
    bool reducedMotion = false;

    void setPosition(float c, float s) { coupling = juce::jlimit(0.0f, 1.0f, c); stress = juce::jlimit(0.0f, 1.0f, s); repaint(); }
    juce::Rectangle<float> fieldBounds() const { return getLocalBounds().toFloat().reduced(30, 35); }
    juce::Point<float> centre() const { return fieldBounds().getCentre(); }
    float radiusX() const { return 140.0f + (1.0f - coupling) * 160.0f; }
    float radiusY() const { return 34.0f + stress * 90.0f; }

    juce::Point<float> gripPosition(Grip grip) const
    {
        const auto c = centre();
        if (grip == Grip::left) return { c.x - radiusX(), c.y };
        if (grip == Grip::right) return { c.x + radiusX(), c.y };
        if (grip == Grip::top) return { c.x, c.y - radiusY() };
        if (grip == Grip::bottom) return { c.x, c.y + radiusY() };
        return c;
    }

    juce::Path silhouette() const
    {
        const auto c = centre();
        juce::Path path;
        for (int i = 0; i <= 128; ++i)
        {
            const auto angle = static_cast<float>(i) / 128 * pi * 2;
            const auto fold = std::sin(angle * 2);
            const auto x = c.x + radiusX() * std::cos(angle) * (1 + stress * 0.12f * fold * fold);
            const auto y = c.y + radiusY() * std::sin(angle) + stress * 17 * fold * std::cos(angle);
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        path.closeSubPath();
        return path;
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(glass()); g.fillRoundedRectangle(getLocalBounds().toFloat(), 7);
        const auto c = centre();
        const auto level = live.level.load();
        const auto coherence = reducedMotion ? 1.0f : live.coherence.load();
        const auto body = silhouette();
        g.setGradientFill(juce::ColourGradient(signal().withAlpha(0.23f + level * 0.12f),
            c.x - radiusX() * 0.25f, c.y - radiusY(), signal().withAlpha(0.045f),
            c.x + radiusX() * 0.3f, c.y + radiusY(), false));
        g.fillPath(body);
        g.setColour(signal().withAlpha(0.5f + level * 0.3f));
        g.strokePath(body, juce::PathStrokeType(1));

        {
            const juce::Graphics::ScopedSaveState saved(g);
            g.reduceClipRegion(body);
            for (size_t band = 0; band < 6; ++band)
            {
                const auto phase = reducedMotion ? 0.0f : live.offsets[band].load(std::memory_order_relaxed);
                const auto link = reducedMotion ? 0.55f : live.links[band].load(std::memory_order_relaxed);
                const auto offset = (static_cast<float>(band) - 2.5f) / 3.0f;
                juce::Path line;
                for (int i = 0; i <= 140; ++i)
                {
                    const auto t = static_cast<float>(i) / 140;
                    const auto envelope = std::sin(t * pi);
                    const auto x = c.x - radiusX() + radiusX() * 2 * t;
                    const auto activity = reducedMotion ? 0.0f : level;
                    const auto ripple = std::sin(t * pi * (2 + stress * 4) + phase)
                        * (4 + stress * 21 + activity * 5) * (0.45f + std::abs(link) * 0.55f);
                    const auto y = c.y + envelope * (offset * radiusY() * 0.8f
                        + ripple * (1 - coherence * 0.35f) + stress * 12 * std::sin(t * pi * 2));
                    if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
                }
                g.setColour((band == 2 || band == 3 ? accent() : signal()).withAlpha(0.36f + level * 0.4f));
                g.strokePath(line, juce::PathStrokeType(0.85f + level * 0.35f));
            }
        }
        for (const auto grip : { Grip::left, Grip::right, Grip::top, Grip::bottom })
        {
            const auto point = gripPosition(grip);
            const auto active = grip == dragGrip || grip == hovered;
            g.setColour(accent().withAlpha(active ? 0.22f : 0.09f));
            g.fillEllipse(point.x - 12, point.y - 12, 24, 24);
            g.setColour(active ? accent() : signal().withAlpha(0.7f));
            g.drawEllipse(point.x - 4, point.y - 4, 8, 8, 1);
            if (active) g.fillEllipse(point.x - 2, point.y - 2, 4, 4);
        }
        text(g, "COUPLING / PINCH OR STRETCH ENDS", { 16, 11, 460, 16 }, signal().withAlpha(0.7f), 9);
        text(g, "STRESS / PULL THE RIM", { 16, getHeight() - 25, 400, 16 }, signal().withAlpha(0.7f), 9);
        text(g, "GRAB TO PLAY / SHAPE IS RETAINED", { getWidth() - 340, getHeight() - 25, 322, 16 }, signal().withAlpha(0.5f), 8, juce::Justification::right);
        if (hasKeyboardFocus(true)) { g.setColour(accent()); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3), 5, 1.3f); }
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        hovered = hitGrip(event.position);
        setMouseCursor(hovered == Grip::none ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);
        repaint();
    }
    void mouseExit(const juce::MouseEvent&) override { hovered = Grip::none; repaint(); }
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (!event.mods.isLeftButtonDown()) return;
        const auto hit = hitGrip(event.position);
        if (hit == Grip::none) return;
        grabKeyboardFocus();
        dragGrip = hit;
        pointerDown = true;
        startPoint = event.position; startCoupling = coupling; startStress = stress;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        if (!keyDown && onBegin) onBegin();
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!pointerDown) return;
        const auto delta = event.position - startPoint;
        if (dragGrip == Grip::body)
        {
            if (delta.getDistanceFromOrigin() < 4) return;
            if (std::abs(delta.x) > std::abs(delta.y))
                dragGrip = std::abs(startPoint.x - centre().x) < 12
                    ? delta.x >= 0 ? Grip::right : Grip::left : startPoint.x >= centre().x ? Grip::right : Grip::left;
            else dragGrip = std::abs(startPoint.y - centre().y) < 12
                ? delta.y <= 0 ? Grip::top : Grip::bottom : startPoint.y <= centre().y ? Grip::top : Grip::bottom;
        }
        if (dragGrip == Grip::left || dragGrip == Grip::right)
            setPosition(startCoupling - delta.x * (dragGrip == Grip::right ? 1.0f : -1.0f) / 160, startStress);
        else setPosition(startCoupling, startStress + delta.y * (dragGrip == Grip::bottom ? 1.0f : -1.0f) / 90);
        if (onShape) onShape(coupling, stress);
    }
    void mouseUp(const juce::MouseEvent& event) override { endPointer(); mouseMove(event); }
    void focusLost(FocusChangeType) override { endPointer(); endKey(); repaint(); }
    void focusGained(FocusChangeType) override { repaint(); }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto step = key.getModifiers().isShiftDown() ? 0.002f : 0.02f;
        if (key.getKeyCode() == juce::KeyPress::escapeKey && pointerDown)
        {
            setPosition(startCoupling, startStress); if (onShape) onShape(coupling, stress); endPointer(); return true;
        }
        if (key.getKeyCode() == juce::KeyPress::leftKey) coupling -= step;
        else if (key.getKeyCode() == juce::KeyPress::rightKey) coupling += step;
        else if (key.getKeyCode() == juce::KeyPress::upKey) stress += step;
        else if (key.getKeyCode() == juce::KeyPress::downKey) stress -= step;
        else if (key.getKeyCode() == juce::KeyPress::spaceKey)
        {
            if (!keyDown) { keyDown = true; if (!pointerDown && onBegin) onBegin(); }
            return true;
        }
        else return false;
        setPosition(coupling, stress); if (onShape) onShape(coupling, stress); return true;
    }
    bool keyStateChanged(bool) override
    {
        if (keyDown && !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey)) { endKey(); return true; }
        return false;
    }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Handler final : public juce::AccessibilityHandler
        {
        public:
            explicit Handler(DeformingBody& owner)
                : AccessibilityHandler(owner, juce::AccessibilityRole::group,
                    juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus, [&owner] { owner.grabKeyboardFocus(); })
                        .addAction(juce::AccessibilityActionType::press, [&owner] {
                            if (owner.keyDown) owner.endKey();
                            else { owner.keyDown = true; if (!owner.pointerDown && owner.onBegin) owner.onBegin(); }
                        })), body(owner) {}
            juce::String getDescription() const override
            {
                return "Coupling " + juce::String(body.coupling * 100, 0) + " percent, Stress " + juce::String(body.stress * 100, 0) + " percent";
            }
            juce::String getHelp() const override { return body.getDescription(); }
        private:
            DeformingBody& body;
        };
        return std::make_unique<Handler>(*this);
    }

private:
    Grip hitGrip(juce::Point<float> point) const
    {
        auto nearest = Grip::none;
        float distance = 24;
        for (const auto grip : { Grip::left, Grip::right, Grip::top, Grip::bottom })
            if (const auto d = point.getDistanceFrom(gripPosition(grip)); d < distance) { nearest = grip; distance = d; }
        if (nearest != Grip::none) return nearest;
        return silhouette().contains(point.x, point.y) ? Grip::body : Grip::none;
    }
    void endPointer()
    {
        if (!pointerDown) return;
        pointerDown = false; dragGrip = Grip::none; setMouseCursor(juce::MouseCursor::NormalCursor);
        if (!keyDown && onEnd) onEnd(); repaint();
    }
    void endKey() { if (keyDown) { keyDown = false; if (!pointerDown && onEnd) onEnd(); } }
    LiveState& live;
    float coupling = 0.65f, stress = 0.2f, startCoupling = 0.65f, startStress = 0.2f;
    juce::Point<float> startPoint;
    Grip dragGrip = Grip::none, hovered = Grip::none;
    bool pointerDown = false, keyDown = false;
};
using ControlSurface = DeformingBody;
#else
using ControlSurface = PlayingField;
#endif

class ResponseRibbon final : public juce::Component, private juce::Timer
{
public:
    explicit ResponseRibbon(LiveState& state) : live(state)
    {
        setName("Response timing gesture");
        setTitle(getName());
        setDescription("Hold to set a response duration, or drag to stretch it. Arrow keys make finer adjustments.");
        setWantsKeyboardFocus(true);
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    std::function<void(double)> onResponse;
    std::function<void()> onBegin, onEnd;
    bool reducedMotion = false;
    void setResponse(double seconds) { response = juce::jlimit(0.01, 1.6, seconds); repaint(); }

    void paint(juce::Graphics& g) override
    {
        text(g, "RESPONSE", { 0, 0, 180, 18 }, ink(), 9);
        text(g, response < 1.0 ? juce::String(response * 1000, 0) + " ms" : juce::String(response, 2) + " s",
             { getWidth() - 140, 0, 140, 18 }, ink(), 11, juce::Justification::right);
        const auto extent = 20.0f + static_cast<float>(responsePosition(response)) * static_cast<float>(getWidth() - 40);
        const auto activity = reducedMotion ? 0.0f : live.level.load();
        for (int band = 0; band < 4; ++band)
        {
            juce::Path tail;
            tail.startNewSubPath(20, 46);
            tail.cubicTo(extent * 0.35f, 46 - static_cast<float>(band + 1) * (4.5f + activity * 3),
                         extent * 0.72f, 46 + static_cast<float>(band + 1) * (4.5f + activity * 3), extent, 46);
            g.setColour(ink().withAlpha(0.25f + static_cast<float>(band) * 0.1f));
            g.strokePath(tail, juce::PathStrokeType(0.8f));
        }
        g.setColour(accent());
        g.drawLine(extent - 4, 40, extent + 2, 46, 1.5f);
        g.drawLine(extent + 2, 46, extent - 4, 52, 1.5f);
        text(g, "FAST", { 0, 65, 100, 16 }, muted(), 8);
        text(g, "HOLD TO SET TIME / DRAG TO STRETCH", { 110, 65, getWidth() - 220, 16 }, muted(), 8, juce::Justification::centred);
        text(g, "SLOW", { getWidth() - 100, 65, 100, 16 }, muted(), 8, juce::Justification::right);
        if (hasKeyboardFocus(true)) { g.setColour(accent()); g.drawRect(getLocalBounds().reduced(1), 1); }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (!event.mods.isLeftButtonDown()) return;
        grabKeyboardFocus();
        started = juce::Time::getMillisecondCounterHiRes();
        holding = true;
        dragging = false;
        if (onBegin) onBegin();
        startTimerHz(30);
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!holding || event.getDistanceFromDragStart() < 4) return;
        dragging = true;
        stopTimer();
        change(responseFromPosition(static_cast<double>(event.position.x - 20) / static_cast<double>(getWidth() - 40)));
    }
    void mouseUp(const juce::MouseEvent&) override { finish(); }
    void focusLost(FocusChangeType) override { finish(); repaint(); }
    void focusGained(FocusChangeType) override { repaint(); }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto step = key.getModifiers().isShiftDown() ? 0.005 : 0.035;
        if (key.getKeyCode() == juce::KeyPress::leftKey || key.getKeyCode() == juce::KeyPress::downKey)
            change(responseFromPosition(responsePosition(response) - step));
        else if (key.getKeyCode() == juce::KeyPress::rightKey || key.getKeyCode() == juce::KeyPress::upKey)
            change(responseFromPosition(responsePosition(response) + step));
        else return false;
        return true;
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        class Value final : public juce::AccessibilityValueInterface
        {
        public:
            explicit Value(ResponseRibbon& owner) : ribbon(owner) {}
            bool isReadOnly() const override { return false; }
            double getCurrentValue() const override { return ribbon.response; }
            juce::String getCurrentValueAsString() const override { return juce::String(ribbon.response * 1000, 0) + " milliseconds"; }
            void setValue(double value) override { ribbon.change(value); }
            void setValueAsString(const juce::String& value) override { ribbon.change(value.getDoubleValue() / 1000); }
            AccessibleValueRange getRange() const override { return { { 0.01, 1.6 }, 0.01 }; }
        private:
            ResponseRibbon& ribbon;
        };
        return std::make_unique<juce::AccessibilityHandler>(*this, juce::AccessibilityRole::slider,
            juce::AccessibilityActions().addAction(juce::AccessibilityActionType::focus, [this] { grabKeyboardFocus(); }),
            juce::AccessibilityHandler::Interfaces(std::make_unique<Value>(*this)));
    }

private:
    void change(double seconds) { setResponse(seconds); if (onResponse) onResponse(response); }
    void finish()
    {
        if (!holding) return;
        if (!dragging) change((juce::Time::getMillisecondCounterHiRes() - started) / 1000);
        holding = false;
        stopTimer();
        if (onEnd) onEnd();
    }
    void timerCallback() override { if (holding && !dragging) change((juce::Time::getMillisecondCounterHiRes() - started) / 1000); }
    LiveState& live;
    double response = 0.075, started = 0;
    bool holding = false, dragging = false;
};

class GestureComponent final : public juce::AudioAppComponent, private juce::Timer
{
public:
    GestureComponent() : field(live), ribbon(live), keyboard(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        setLookAndFeel(&lookAndFeel);
        lookAndFeel.setColour(juce::TextButton::buttonColourId, chassis());
        lookAndFeel.setColour(juce::TextButton::buttonOnColourId, ink());
        lookAndFeel.setColour(juce::TextButton::textColourOffId, ink());
        lookAndFeel.setColour(juce::TextButton::textColourOnId, paper());
        lookAndFeel.setColour(juce::PopupMenu::backgroundColourId, paper());
        lookAndFeel.setColour(juce::PopupMenu::textColourId, ink());
        field.onShape = [this](float c, float s) { coupling.store(c); stress.store(s); repaint(); };
        field.onBegin = [this] { touching = true; startAudition(); };
        field.onEnd = [this] { touching = false; if (!hold.getToggleState()) endAudition(); };
        ribbon.onResponse = [this](double seconds) { response.store(seconds); };
        ribbon.onBegin = field.onBegin;
        ribbon.onEnd = field.onEnd;
        addAndMakeVisible(field);
        addAndMakeVisible(ribbon);
        const juce::StringArray labels { "PAD", "VOICE", "HIT" };
        for (size_t i = 0; i < familyButtons.size(); ++i)
        {
            auto& button = familyButtons[i];
            button.setButtonText(labels[static_cast<int>(i)]);
            button.setRadioGroupId(1);
            button.setClickingTogglesState(true);
            button.setName("Starting patch " + labels[static_cast<int>(i)].toLowerCase());
            button.onClick = [this, i] { chooseFamily(static_cast<int>(i)); };
            addAndMakeVisible(button);
        }
        hold.setButtonText("HOLD");
        hold.setClickingTogglesState(true);
        hold.setName("Hold audition notes");
        hold.onClick = [this] { if (hold.getToggleState()) startAudition(true); else if (!touching) endAudition(); };
        addAndMakeVisible(hold);
        options.setButtonText("...");
        options.setName("Audio, MIDI and display options");
        options.onClick = [this] { showOptions(); };
        addAndMakeVisible(options);
        help.setButtonText("?");
        help.setName("Gesture study help");
        help.onClick = [] {
            const juce::String surfaceHelp = deformationStudy
                ? "Grab the object to play. Pinch its ends together to increase Coupling, or pull them apart to loosen it. "
                  "Pull the top or bottom rim outward for more Stress; push it inward for less. Grabbing does not jump the sound. "
                  "Release retains the shape; Escape cancels the current drag.\n\n"
                : "Touch the field to play the current starting patch. Drag horizontally for Coupling and vertically for Stress. "
                  "The position is retained when you release; releasing ends the audition note.\n\n";
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                deformationStudy ? "Nitride / control study 07" : "Nitride / control study 06", surfaceHelp +
                "Hold sustains audition notes so you can shape the field and Response independently. MIDI and the keyboard also play the voice.\n\n"
                "Press and hold the Response ribbon to set its duration, or drag to stretch it. Response changes the pace of phase/link evolution, "
                "not just the visual motion.\n\n"
                "Field keyboard: arrows shape, Shift makes finer changes, Space plays. Response: arrows adjust timing. "
                "Options includes reduced motion, output level, and Audio/MIDI.\n\n"
                "The traces show the most recent active voice's network state, not a measured audio spectrum."
                + juce::String(deformationStudy ? " The object is a control representation, not a calibrated physical simulation." : ""));
        };
        addAndMakeVisible(help);
        keyboard.setAvailableRange(48, 84);
        keyboard.setLowestVisibleKey(48);
        keyboard.setKeyWidth(28);
        keyboard.setKeyPressBaseOctave(4);
        keyboard.setOctaveForMiddleC(4);
        keyboard.setWantsKeyboardFocus(true);
        keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, paper());
        keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, ink());
        keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent().withAlpha(0.6f));
        addAndMakeVisible(keyboard);
        midi.ensureSize(8192);
        chooseFamily(1);
        setSize(1000, 750);
        setAudioChannels(0, 2);
        deviceManager.addMidiInputDeviceCallback({}, &collector);
        timerCallback();
        startTimerHz(30);
    }

    ~GestureComponent() override
    {
        stopTimer();
        deviceManager.removeMidiInputDeviceCallback({}, &collector);
        shutdownAudio();
        setLookAndFeel(nullptr);
    }

    void prepareToPlay(int, double sampleRate) override
    {
        engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);
        engine.setSource(family.load() == 0 ? SoundStudies::Source::simple : SoundStudies::Source::rich);
        engine.setNetworkDimensions(coupling.load(), stress.load(), response.load());
        engine.prepare(sampleRate);
        collector.reset(sampleRate);
        outputGain.reset(sampleRate, 0.025);
        outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputDb.load()));
        // Restore notes that were already sounding if the audio device is reconfigured.
        std::array<bool, 128> restored {};
        for (size_t i = 0; i < heldNotes.size(); ++i)
            if (heldNotes[i] && !restored[i % 128])
            {
                restored[i % 128] = true;
                engine.noteOn(static_cast<int>(i % 128), heldVelocities[i], articulation());
            }
    }
    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        const juce::ScopedNoDenormals noDenormals;
        info.clearActiveBufferRegion();
        if (info.buffer->getNumChannels() == 0) return;
        engine.setSource(family.load() == 0 ? SoundStudies::Source::simple : SoundStudies::Source::rich);
        engine.setNetworkDimensions(coupling.load(), stress.load(), response.load());
        midi.clear();
        collector.removeNextBlockOfMessages(midi, info.numSamples);
        keyboardState.processNextMidiBuffer(midi, 0, info.numSamples, true);
        auto* left = info.buffer->getWritePointer(0, info.startSample);
        auto* right = info.buffer->getWritePointer(info.buffer->getNumChannels() > 1 ? 1 : 0, info.startSample);
        int offset = 0;
        for (const auto metadata : midi)
        {
            const auto position = juce::jlimit(offset, info.numSamples, metadata.samplePosition);
            engine.render(left + offset, right + offset, position - offset);
            const auto message = metadata.getMessage();
            if (message.isNoteOn() || message.isNoteOff())
            {
                const auto note = message.getNoteNumber(), channel = message.getChannel();
                heldNotes[static_cast<size_t>((channel - 1) * 128 + note)] = message.isNoteOn();
                if (message.isNoteOn())
                {
                    heldVelocities[static_cast<size_t>((channel - 1) * 128 + note)] = message.getFloatVelocity();
                    bool capturedExternal = false;
                    if (channel == 16)
                        for (int c = 0; c < 15; ++c) capturedExternal |= heldNotes[static_cast<size_t>(c * 128 + note)];
                    // Latching an already held note takes ownership of its release; it
                    // should not double the sound or replace the original velocity.
                    if (!capturedExternal) engine.noteOn(note, message.getFloatVelocity(), articulation());
                }
                else
                {
                    bool anotherHeld = false;
                    for (int c = 0; c < 16; ++c) anotherHeld |= heldNotes[static_cast<size_t>(c * 128 + note)];
                    if (!anotherHeld) engine.noteOff(note);
                }
            }
            else if (message.isPitchWheel()) engine.setPitchBend(static_cast<float>(message.getPitchWheelValue() - 8192) * 2 / 8192);
            else if (message.isAllNotesOff() || message.isAllSoundOff()) { heldNotes = {}; engine.allNotesOff(); }
            offset = position;
        }
        engine.render(left + offset, right + offset, info.numSamples - offset);
        outputGain.setTargetValue(juce::Decibels::decibelsToGain(outputDb.load()));
        float peak = 0;
        for (int i = 0; i < info.numSamples; ++i)
        {
            const auto gain = outputGain.getNextValue();
            left[i] = juce::jlimit(-0.95f, 0.95f, left[i] * gain);
            if (right != left) right[i] = juce::jlimit(-0.95f, 0.95f, right[i] * gain);
            peak = std::max(peak, std::abs(left[i]));
        }
        int external = 0;
        for (size_t i = 0; i < 15 * 128; ++i) external += heldNotes[i] ? 1 : 0;
        live.externalKeys.store(external);
        live.peak.store(peak);
        live.faults.store(engine.getNumericalFaults());
        live.publish(engine.getNetworkSnapshot());
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(chassis());
        g.setColour(ink());
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 36, juce::Font::bold)));
        g.drawText("nitride", 28, 19, 200, 48, juce::Justification::left);
        text(g, deformationStudy ? "CONTROL STUDY 07" : "CONTROL STUDY 06", { 29, 67, 250, 16 }, muted(), 8);
        g.setColour(juce::Colour(0xffb6b9af));
        g.drawHorizontalLine(96, 28, 972);
        text(g, "COUPLING " + juce::String(coupling.load() * 100, 0) + "%", { 32, 520, 250, 18 }, muted(), 9);
        text(g, "STRESS " + juce::String(stress.load() * 100, 0) + "%", { 280, 520, 250, 18 }, muted(), 9);
        text(g, "ARROWS: SHAPE / SPACE: PLAY", { 570, 520, 398, 18 }, muted(), 8, juce::Justification::right);
        g.setColour(juce::Colour(0xffb6b9af));
        g.drawHorizontalLine(658, 28, 972);
        text(g, deviceStatus, { 30, 726, 670, 17 }, muted(), 8);
        text(g, juce::String(live.voices.load()) + " VOICES / " + juce::String(outputDb.load(), 0) + " dB",
             { 710, 726, 258, 17 }, muted(), 8, juce::Justification::right);
    }

    void resized() override
    {
        for (size_t i = 0; i < familyButtons.size(); ++i) familyButtons[i].setBounds(410 + static_cast<int>(i) * 90, 30, 82, 33);
        hold.setBounds(715, 30, 90, 33);
        help.setBounds(825, 30, 42, 33);
        options.setBounds(884, 30, 84, 33);
        field.setBounds(28, 117, 944, 392);
        ribbon.setBounds(32, 560, 936, 86);
        keyboard.setBounds(30, 672, 940, 43);
        keyboard.setKeyWidth(940.0f / 22.0f);
    }

    void beginAudioCheck()
    {
        chooseFamily(0);
        for (const auto note : {48, 55, 59, 62}) keyboardState.noteOn(1, note, 0.66f);
        hold.setToggleState(true, juce::dontSendNotification); hold.onClick();
        for (const auto note : {48, 55, 59, 62}) keyboardState.noteOff(1, note, 0);
    }
    void beginPreview()
    {
        hold.setToggleState(true, juce::dontSendNotification);
        startAudition();
    }
    void changeAudioCheck()
    {
        coupling.store(0.95f); stress.store(0.9f); response.store(0.035);
        field.setPosition(0.95f, 0.9f); ribbon.setResponse(0.035);
    }
    bool finishAudioCheck()
    {
        std::cout << "Gesture audio check: " << deviceStatus << ", peak " << live.peak.load()
                  << ", CPU " << deviceManager.getCpuUsage() * 100 << "%, xruns " << deviceManager.getXRunCount()
                  << ", numerical faults " << live.faults.load() << ", latched voices " << live.voices.load() << '\n';
        const auto passed = deviceManager.getCurrentAudioDevice() != nullptr && live.peak.load() > 0.0001f
            && live.faults.load() == 0 && live.voices.load() == 4 && deviceManager.getXRunCount() == 0 && deviceManager.getCpuUsage() < 0.9;
        hold.setToggleState(false, juce::dontSendNotification);
        endAudition();
        return passed;
    }
    bool checkReleased()
    {
        const auto released = live.voices.load() == 0 && live.peak.load() < 0.000001f;
        std::cout << "Gesture release check: " << released << '\n';
        return released;
    }

    bool checkInteractions()
    {
        chooseFamily(1);
        const auto oldCoupling = coupling.load(), oldStress = stress.load();
        const auto oldResponse = response.load();
        field.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
        field.keyPressed(juce::KeyPress(juce::KeyPress::upKey));
        ribbon.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
        const auto independent = coupling.load() > oldCoupling && stress.load() > oldStress && response.load() > oldResponse;
        field.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey));
        const auto played = !auditionNotes.isEmpty();
        field.keyStateChanged(false);
        const auto released = auditionNotes.isEmpty();
        hold.setToggleState(true, juce::dontSendNotification); hold.onClick();
        field.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)); field.keyStateChanged(false);
        const auto retained = !auditionNotes.isEmpty();
        hold.setToggleState(false, juce::dontSendNotification); hold.onClick();
        const auto stopped = auditionNotes.isEmpty();
        const auto now = juce::Time::getCurrentTime();
#if NITRIDE_DEFORMATION_STUDY
        const auto point = field.gripPosition(DeformingBody::Grip::right);
#else
        const auto area = field.fieldBounds();
        const auto point = juce::Point<float>(area.getX() + area.getWidth() * 0.22f, area.getBottom() - area.getHeight() * 0.73f);
#endif
        const juce::MouseEvent down(juce::Desktop::getInstance().getMainMouseSource(), point,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, point, now, 1, false);
        field.mouseDown(down);
#if NITRIDE_DEFORMATION_STUDY
        const auto noJump = std::abs(coupling.load() - (oldCoupling + 0.02f)) < 0.001f;
        const auto pulledPoint = point + juce::Point<float>((coupling.load() - 0.22f) * 160, 0);
        const juce::MouseEvent pull(juce::Desktop::getInstance().getMainMouseSource(), pulledPoint,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, point, now, 1, true);
        field.mouseDrag(pull);
        const auto mapped = noJump && std::abs(coupling.load() - 0.22f) < 0.001f && std::abs(stress.load() - (oldStress + 0.02f)) < 0.001f;
#else
        const auto mapped = std::abs(coupling.load() - 0.22f) < 0.001f && std::abs(stress.load() - 0.73f) < 0.001f;
#endif
        field.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey)); field.keyStateChanged(false);
        const auto stillHeld = !auditionNotes.isEmpty();
        field.mouseUp(down);
        const auto poseRetained = auditionNotes.isEmpty() && std::abs(coupling.load() - 0.22f) < 0.001f;
#if NITRIDE_DEFORMATION_STUDY
        const auto rim = field.gripPosition(DeformingBody::Grip::top);
        const auto stressedPoint = rim - juce::Point<float>(0, (0.73f - stress.load()) * 90);
        const juce::MouseEvent rimDown(juce::Desktop::getInstance().getMainMouseSource(), rim,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, rim, now, 1, false);
        const juce::MouseEvent rimPull(juce::Desktop::getInstance().getMainMouseSource(), stressedPoint,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, rim, now, 1, true);
        field.mouseDown(rimDown); field.mouseDrag(rimPull); field.mouseUp(rimPull);
        const auto rimMapped = std::abs(stress.load() - 0.73f) < 0.001f && std::abs(coupling.load() - 0.22f) < 0.001f;
        const auto cancelPoint = field.gripPosition(DeformingBody::Grip::left);
        const juce::MouseEvent cancelDown(juce::Desktop::getInstance().getMainMouseSource(), cancelPoint,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, cancelPoint, now, 1, false);
        const juce::MouseEvent cancelDrag(juce::Desktop::getInstance().getMainMouseSource(), cancelPoint - juce::Point<float>(30, 0),
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, cancelPoint, now, 1, true);
        field.mouseDown(cancelDown); field.mouseDrag(cancelDrag); field.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
        const auto cancelled = std::abs(coupling.load() - 0.22f) < 0.001f && auditionNotes.isEmpty();
        const auto blank = juce::Point<float>(10, 45);
        const juce::MouseEvent emptyDown(juce::Desktop::getInstance().getMainMouseSource(), blank,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, blank, now, 1, false);
        field.mouseDown(emptyDown);
        const auto emptyIgnored = auditionNotes.isEmpty();
        const auto centrePoint = field.centre();
        const juce::MouseEvent bodyDown(juce::Desktop::getInstance().getMainMouseSource(), centrePoint,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, centrePoint, now, 1, false);
        const juce::MouseEvent bodyDrag(juce::Desktop::getInstance().getMainMouseSource(), centrePoint - juce::Point<float>(0, 9),
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, centrePoint, now, 1, true);
        field.mouseDown(bodyDown); field.mouseDrag(bodyDrag); field.mouseUp(bodyDrag);
        const auto bodyMapped = std::abs(stress.load() - 0.83f) < 0.001f && std::abs(coupling.load() - 0.22f) < 0.001f;
        const auto endPoint = field.gripPosition(DeformingBody::Grip::right);
        const juce::MouseEvent boundDown(juce::Desktop::getInstance().getMainMouseSource(), endPoint,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, endPoint, now, 1, false);
        const juce::MouseEvent boundDrag(juce::Desktop::getInstance().getMainMouseSource(), endPoint + juce::Point<float>(1000, 0),
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &field, &field, now, endPoint, now, 1, true);
        field.mouseDown(boundDown); field.mouseDrag(boundDrag); field.mouseUp(boundDrag);
        const auto bounded = std::abs(coupling.load()) < 0.001f && std::abs(stress.load() - 0.83f) < 0.001f;
        const auto objectPassed = rimMapped && cancelled && emptyIgnored && bodyMapped && bounded;
        std::cout << "Object checks: no-jump " << noJump << ", rim " << rimMapped << ", cancel " << cancelled
                  << ", blank " << emptyIgnored << ", body " << bodyMapped << ", bounds " << bounded << '\n';
#else
        const auto objectPassed = true;
#endif
        const auto accessible = field.getAccessibilityHandler() != nullptr && ribbon.getAccessibilityHandler()->getValueInterface() != nullptr;
        const auto ribbonStart = juce::Point<float>(30, 46);
        const auto ribbonEnd = juce::Point<float>(static_cast<float>(ribbon.getWidth()) * 0.75f, 46);
        const juce::MouseEvent timeDown(juce::Desktop::getInstance().getMainMouseSource(), ribbonStart,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &ribbon, &ribbon, now, ribbonStart, now, 1, false);
        const juce::MouseEvent timeDrag(juce::Desktop::getInstance().getMainMouseSource(), ribbonEnd,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &ribbon, &ribbon, now, ribbonStart, now, 1, true);
        ribbon.mouseDown(timeDown); ribbon.mouseDrag(timeDrag);
        const auto timePlayed = !auditionNotes.isEmpty() && response.load() > oldResponse;
        ribbon.mouseUp(timeDrag);
        const auto timeReleased = auditionNotes.isEmpty();
        keyboardState.noteOn(1, 64, 0.6f); keyboardState.noteOn(1, 67, 0.6f);
        hold.setToggleState(true, juce::dontSendNotification); hold.onClick();
        keyboardState.noteOff(1, 64, 0); keyboardState.noteOff(1, 67, 0);
        const auto capturedKeys = auditionNotes.contains(64) && auditionNotes.contains(67) && keyboardState.isNoteOn(16, 64);
        hold.setToggleState(false, juce::dontSendNotification); hold.onClick();
        const auto capturedReleased = auditionNotes.isEmpty() && !keyboardState.isNoteOn(16, 64);
        std::cout << "Gesture interaction check: independent " << independent << ", play/release " << played << '/' << released
                  << ", hold/release " << retained << '/' << stopped << ", pointer/pose " << mapped << '/' << poseRetained
                  << ", mixed input " << stillHeld << ", accessible " << accessible
                  << ", time gesture " << timePlayed << '/' << timeReleased << ", captured keys " << capturedKeys << '/' << capturedReleased << '\n';
        return independent && played && released && retained && stopped && mapped && stillHeld && poseRetained && accessible
            && timePlayed && timeReleased && capturedKeys && capturedReleased && objectPassed;
    }

private:
    void chooseFamily(int selected)
    {
        endAudition();
        family.store(selected);
        if (selected == 2) hold.setToggleState(false, juce::dontSendNotification);
        hold.setEnabled(selected != 2);
        constexpr std::array<std::array<double, 3>, 3> starts {{{0.55, 0.05, 0.45}, {0.65, 0.2, 0.075}, {0.8, 0.4, 0.018}}};
        const auto& position = starts[static_cast<size_t>(selected)];
        coupling.store(static_cast<float>(position[0])); stress.store(static_cast<float>(position[1])); response.store(position[2]);
        field.setPosition(static_cast<float>(position[0]), static_cast<float>(position[1])); ribbon.setResponse(position[2]);
        for (size_t i = 0; i < familyButtons.size(); ++i) familyButtons[i].setToggleState(static_cast<int>(i) == selected, juce::dontSendNotification);
        if (hold.getToggleState() || touching) startAudition();
        repaint();
    }

    void startAudition(bool captureExternal = false)
    {
        if (!auditionNotes.isEmpty()) return;
        if (captureExternal)
            for (int note = 0; note < 128; ++note)
                for (int channel = 1; channel < 16; ++channel)
                    if (keyboardState.isNoteOn(channel, note)) { auditionNotes.add(note); break; }
        if (auditionNotes.isEmpty())
        {
            if (live.externalKeys.load() != 0) return;
            auditionNotes = family.load() == 0 ? juce::Array<int> {48, 55, 59, 62} : juce::Array<int> {60};
        }
        for (const auto note : auditionNotes) keyboardState.noteOn(16, note, 0.74f);
    }
    void endAudition()
    {
        for (const auto note : auditionNotes) keyboardState.noteOff(16, note, 0);
        auditionNotes.clear();
    }

    void showOptions()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Audio / MIDI...");
        menu.addItem(2, "Reduced motion", true, field.reducedMotion);
        menu.addSeparator();
        for (int i = 0; i < 3; ++i) menu.addItem(10 + i, "Output " + juce::String(-12 + i * 6) + " dB", true,
                                               std::abs(outputDb.load() - static_cast<float>(-12 + i * 6)) < 0.01f);
        const juce::Component::SafePointer<GestureComponent> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&options), [safe](int choice) {
            if (safe == nullptr) return;
            if (choice == 1)
            {
                auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(safe->deviceManager, 0, 0, 0, 2, true, false, true, false);
                selector->setSize(500, 430);
                juce::DialogWindow::LaunchOptions dialog;
                dialog.content.setOwned(selector.release()); dialog.dialogTitle = "Nitride / Audio and MIDI";
                dialog.dialogBackgroundColour = chassis(); dialog.componentToCentreAround = safe.getComponent();
                dialog.useNativeTitleBar = true; dialog.resizable = false; dialog.launchAsync();
            }
            else if (choice == 2)
            {
                safe->field.reducedMotion = !safe->field.reducedMotion;
                safe->ribbon.reducedMotion = safe->field.reducedMotion;
                safe->field.repaint(); safe->ribbon.repaint();
            }
            else if (choice >= 10 && choice <= 12) safe->outputDb.store(static_cast<float>(-12 + (choice - 10) * 6));
        });
    }

    void timerCallback() override
    {
        if (auto* device = deviceManager.getCurrentAudioDevice()) deviceStatus = device->getName() + " / " + juce::String(device->getCurrentSampleRate(), 0) + " Hz";
        else deviceStatus = "NO AUDIO DEVICE / OPEN OPTIONS";
        field.repaint(); ribbon.repaint(); repaint(28, 721, 944, 28);
    }

    SoundStudies::Articulation articulation() const
    {
        return family.load() == 0 ? SoundStudies::Articulation::pad
            : family.load() == 2 ? SoundStudies::Articulation::hit : SoundStudies::Articulation::lead;
    }

    juce::LookAndFeel_V4 lookAndFeel;
    SoundStudies::Engine engine;
    LiveState live;
    ControlSurface field;
    ResponseRibbon ribbon;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard;
    juce::MidiMessageCollector collector;
    juce::MidiBuffer midi;
    juce::SmoothedValue<float> outputGain;
    std::array<juce::TextButton, 3> familyButtons;
    juce::TextButton hold, help, options;
    std::atomic<int> family { 1 };
    std::atomic<float> coupling { 0.65f }, stress { 0.2f }, outputDb { -6 };
    std::atomic<double> response { 0.075 };
    std::array<bool, 16 * 128> heldNotes {};
    std::array<float, 16 * 128> heldVelocities {};
    juce::Array<int> auditionNotes;
    bool touching = false;
    juce::String deviceStatus;
};

#if NITRIDE_INSTRUMENT_REVIEW
// The review surface reuses the approved field, time ribbon and snapshot boundary.
#include "InstrumentReview.inc"
using ApplicationComponent = InstrumentReview;
#else
using ApplicationComponent = GestureComponent;
#endif

class GestureApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return applicationName; }
    const juce::String getApplicationVersion() override { return instrumentReview ? "0.8" : deformationStudy ? "0.7" : "0.6"; }
    void initialise(const juce::String& commandLine) override
    {
        window = std::make_unique<Window>();
        if (commandLine == "--interaction-check")
        {
            auto* component = static_cast<ApplicationComponent*>(window->getContentComponent());
            setApplicationReturnValue(component->checkInteractions() ? 0 : 1); quit();
        }
        if (commandLine == "--audio-check")
        {
            auto* component = static_cast<ApplicationComponent*>(window->getContentComponent());
            component->beginAudioCheck();
            juce::Timer::callAfterDelay(1800, [this, component] {
                component->changeAudioCheck();
                juce::Timer::callAfterDelay(1800, [this, component] {
                    const auto passed = component->finishAudioCheck();
                    juce::Timer::callAfterDelay(1800, [this, component, passed] {
                        setApplicationReturnValue(component->checkReleased() && passed ? 0 : 1); quit();
                    });
                });
            });
        }
        if (commandLine.startsWith("--snapshot ") || commandLine.startsWith("--snapshot-active "))
        {
            const auto active = commandLine.startsWith("--snapshot-active ");
            const auto path = commandLine.fromFirstOccurrenceOf(active ? "--snapshot-active " : "--snapshot ", false, false).trim().unquoted();
            if (active) static_cast<ApplicationComponent*>(window->getContentComponent())->beginPreview();
            juce::Timer::callAfterDelay(active ? 900 : 10, [this, path] {
                auto* content = window->getContentComponent();
                const auto image = content->createComponentSnapshot(content->getLocalBounds(), true, 2);
                if (auto stream = juce::File(path).createOutputStream())
                {
                    stream->setPosition(0); stream->truncate(); juce::PNGImageFormat().writeImageToStream(image, *stream);
                }
                quit();
            });
        }
    }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    class Window final : public juce::DocumentWindow
    {
    public:
        Window() : DocumentWindow(instrumentReview ? "Nitride / instrument review"
            : deformationStudy ? "Nitride / deformation study" : "Nitride / control study", chassis(), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true); setContentOwned(new ApplicationComponent(), true);
            centreWithSize(getWidth(), getHeight()); setVisible(true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };
    std::unique_ptr<Window> window;
};
}

#if !NITRIDE_REFERENCE_CAPTURE
START_JUCE_APPLICATION(GestureApplication)
#endif
