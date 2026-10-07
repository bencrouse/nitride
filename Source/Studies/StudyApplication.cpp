#include "StudyEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <cmath>
#include <iostream>

namespace
{
juce::Colour paper() { return juce::Colour(0xffdadbd4); }
juce::Colour ink() { return juce::Colour(0xff202321); }
juce::Colour muted() { return juce::Colour(0xff646b63); }
juce::Colour accent() { return juce::Colour(0xfff15d3b); }
juce::Font mono(float size) { return juce::Font(juce::FontOptions("Menlo", size, juce::Font::plain)); }

template <size_t N>
float gainAt(float amount, const std::array<float, N>& knots, const std::array<float, N>& gains)
{
    for (size_t i = 1; i < N; ++i)
        if (amount <= knots[i])
            return gains[i - 1] + (gains[i] - gains[i - 1]) * (amount - knots[i - 1]) / (knots[i] - knots[i - 1]);
    return gains.back();
}

float compensationDb(int treatment, int source, float amount, bool adaptive)
{
    if (treatment == 1) return 2.5f * amount + 0.85f * amount * amount;
    if (treatment == 2) return 1.6f * amount * (1.0f - amount) + 0.025f * amount;
    if (treatment < 3) return 0.0f;
    if (treatment == 7)
    {
        if (amount <= 0.5f) return compensationDb(6, source, amount * 2.0f, adaptive);
        constexpr std::array<std::array<float, 5>, 4> stressGains {{
            { 0.99f, 1.35f, 1.42f, 6.02f, 9.59f }, { 0.85f, 1.60f, 0.65f, 5.46f, 9.47f },
            { -1.32f, -0.91f, -1.37f, 3.23f, 6.99f }, { -1.19f, -1.12f, -2.13f, 3.07f, 6.92f }
        }};
        return gainAt(amount, std::array<float, 5> { 0.5f, 0.6f, 0.75f, 0.9f, 1 },
                      stressGains[static_cast<size_t>(source * 2 + (adaptive ? 0 : 1))]);
    }
    if (treatment == 6)
    {
        constexpr std::array<std::array<float, 5>, 4> gains {{
            { 0, 0.07f, 0.28f, 0.58f, 0.99f }, { 0, 0.03f, 0.16f, 0.44f, 0.85f },
            { 0, 0.34f, 0.87f, 0.13f, -1.32f }, { 0, 0.20f, 0.64f, -0.23f, -1.19f }
        }};
        return gainAt(amount, std::array<float, 5> { 0, 0.3f, 0.5f, 0.7f, 1 },
                      gains[static_cast<size_t>(source * 2 + (adaptive ? 0 : 1))]);
    }
    // Fixed mean matching gains from lead/pad renders; interpolated values are not AGC.
    constexpr std::array<std::array<float, 4>, 2> interactionGains {{
        { 0.0f, 1.22f, 2.79f, 7.38f }, { 0.0f, 1.33f, 2.95f, 5.97f }
    }};
    if (treatment == 3)
        return gainAt(amount, std::array<float, 4> { 0, 0.25f, 0.5f, 1 }, interactionGains[static_cast<size_t>(source)]);
    constexpr std::array<std::array<float, 6>, 4> delayGains {{
        { 0, 1.02f, 1.30f, 2.85f, 5.79f, 10.82f },
        { 0, 1.0667f, 1.28f, 2.01f, 2.37f, 4.04f },
        { 0, 1.06f, 1.44f, 5.06f, 8.21f, 8.24f },
        { 0, 1.225f, 1.47f, 3.11f, 5.09f, 5.35f }
    }};
    return gainAt(amount, std::array<float, 6> { 0, 0.25f, 0.3f, 0.5f, 0.7f, 1 },
                  delayGains[static_cast<size_t>(source * 2 + treatment - 4)]);
}

class StudyComponent final : public juce::AudioAppComponent, private juce::Timer
{
public:
    StudyComponent() : keyboard(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        setLookAndFeel(&lookAndFeel);
        lookAndFeel.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffc5c9bd));
        lookAndFeel.setColour(juce::TextButton::buttonOnColourId, ink());
        lookAndFeel.setColour(juce::TextButton::textColourOffId, ink());
        lookAndFeel.setColour(juce::TextButton::textColourOnId, paper());
        lookAndFeel.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xffe7e8e1));
        lookAndFeel.setColour(juce::ComboBox::textColourId, ink());
        lookAndFeel.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xffa4ab9d));
        lookAndFeel.setColour(juce::ComboBox::arrowColourId, ink());
        lookAndFeel.setColour(juce::Slider::trackColourId, ink());
        lookAndFeel.setColour(juce::Slider::thumbColourId, accent());
        lookAndFeel.setColour(juce::Slider::backgroundColourId, juce::Colour(0xffb6b9af));
        lookAndFeel.setColour(juce::Slider::textBoxTextColourId, ink());
        lookAndFeel.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xffe7e8e1));
        lookAndFeel.setColour(juce::PopupMenu::backgroundColourId, paper());
        lookAndFeel.setColour(juce::PopupMenu::textColourId, ink());
        const juce::StringArray names { "A / FM reference", "G / Current range", "H / Extended range" };
        for (size_t i = 0; i < modeButtons.size(); ++i)
        {
            auto& button = modeButtons[i];
            button.setButtonText(names[static_cast<int>(i)]);
            button.setRadioGroupId(1);
            button.setClickingTogglesState(true);
            button.onClick = [this, i] {
                constexpr std::array<std::array<int, 3>, 5> treatments {{{0, 1, 2}, {0, 3, 4}, {0, 4, 5}, {0, 5, 6}, {0, 6, 7}}};
                const auto selected = treatments[static_cast<size_t>(juce::jlimit(0, 4, studyBox.getSelectedId() - 1))][i];
                mode.store(selected);
                if (selected != 0) intensity.store(retainedAmounts[static_cast<size_t>(selected)]);
                refreshAmountControl();
                repaint();
            };
            addAndMakeVisible(button);
        }
        modeButtons[0].setToggleState(true, juce::dontSendNotification);
        amount.setSliderStyle(juce::Slider::LinearHorizontal);
        amount.setRange(0, 100, 0.1);
        amount.setValue(25, juce::dontSendNotification);
        amount.setTextValueSuffix(" %");
        amount.setName("Treatment amount");
        amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 78, 28);
        amount.setColour(juce::Slider::textBoxTextColourId, ink());
        amount.setDoubleClickReturnValue(true, 25);
        amount.onValueChange = [this] {
            const auto value = static_cast<float>(amount.getValue() / 100);
            intensity.store(value);
            if (mode.load() != 0 && !fixedReference.load()) retainedAmounts[static_cast<size_t>(mode.load())] = value;
        };
        amount.setEnabled(false);
        addAndMakeVisible(amount);
        level.setSliderStyle(juce::Slider::LinearHorizontal);
        level.setRange(-12, 6, 0.1);
        level.setValue(0, juce::dontSendNotification);
        level.setTextValueSuffix(" dB");
        level.setName("Output trim");
        level.setTextBoxStyle(juce::Slider::TextBoxRight, false, 78, 28);
        level.setColour(juce::Slider::textBoxTextColourId, ink());
        level.setDoubleClickReturnValue(true, 0);
        level.onValueChange = [this] { trimDb.store(static_cast<float>(level.getValue())); };
        addAndMakeVisible(level);
        patternBox.addItemList({ "Held pad chord", "Short notes", "Velocity changes", "Amount sweep", "Monophonic lead" }, 1);
        patternBox.setSelectedId(5, juce::dontSendNotification);
        patternBox.setName("Test phrase");
        patternBox.onChange = [this] {
            pattern.store(patternBox.getSelectedId() - 1);
            refreshAmountControl();
            repaint();
        };
        addAndMakeVisible(patternBox);
        sourceBox.addItemList({ "Simple FM / index 1.65", "Rich FM / index 2.70" }, 1);
        sourceBox.setSelectedId(2, juce::dontSendNotification);
        sourceBox.setName("FM source");
        sourceBox.onChange = [this] { sourceProfile.store(sourceBox.getSelectedId() - 1); repaint(); };
        addAndMakeVisible(sourceBox);
        studyBox.addItemList({ "Study 01", "Study 02", "Study 03", "Study 04", "Study 05" }, 1);
        studyBox.setSelectedId(5, juce::dontSendNotification);
        studyBox.setName("Sound study");
        studyBox.onChange = [this] {
            const auto study = studyBox.getSelectedId();
            studyProfile.store(study);
            const auto modern = study > 1;
            const juce::StringArray labels = study == 5
                ? juce::StringArray { "A / FM reference", "G / Current range", "H / Extended range" }
                : study == 4
                ? juce::StringArray { "A / FM reference", "F / 50% reference", "G / Adaptive network" }
                : study == 3
                ? juce::StringArray { "A / FM reference", "E / Original", "F / Note-coupled" }
                : study == 2
                ? juce::StringArray { "A / FM reference", "D / Nonlinear interaction", "E / Phase delay" }
                : juce::StringArray { "A / FM reference", "B / Body coupling", "C / Spectral spread" };
            for (size_t i = 0; i < modeButtons.size(); ++i)
            {
                modeButtons[i].setButtonText(labels[static_cast<int>(i)]);
                modeButtons[i].setToggleState(i == 0, juce::dontSendNotification);
            }
            mode.store(0);
            sourceBox.setSelectedId(modern ? 2 : 1, juce::sendNotificationSync);
            patternBox.setSelectedId(modern ? 5 : 1, juce::sendNotificationSync);
            refreshAmountControl();
            repaint();
        };
        addAndMakeVisible(studyBox);
        adaptiveButton.setButtonText("Adaptive links");
        adaptiveButton.setToggleState(true, juce::dontSendNotification);
        adaptiveButton.setColour(juce::ToggleButton::textColourId, ink());
        adaptiveButton.setColour(juce::ToggleButton::tickColourId, accent());
        adaptiveButton.setEnabled(false);
        adaptiveButton.onClick = [this] { adaptiveLinks.store(adaptiveButton.getToggleState()); };
        addAndMakeVisible(adaptiveButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this] {
            const auto next = !playing.load();
            playing.store(next);
            playButton.setButtonText(next ? "Stop" : "Play");
            playButton.setToggleState(next, juce::dontSendNotification);
            if (!next) keyboardState.allNotesOff(1);
        };
        addAndMakeVisible(playButton);
        restartButton.setButtonText("Restart");
        restartButton.onClick = [this] {
            playing.store(true);
            playButton.setButtonText("Stop");
            playButton.setToggleState(true, juce::dontSendNotification);
            restart.fetch_add(1);
        };
        addAndMakeVisible(restartButton);
        settingsButton.setButtonText("Audio / MIDI");
        settingsButton.onClick = [this] {
            auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager, 0, 0, 0, 2, true, false, true, false);
            selector->setSize(500, 430);
            juce::DialogWindow::LaunchOptions options;
            options.content.setOwned(selector.release());
            options.dialogTitle = "Sound studies / Audio and MIDI";
            options.dialogBackgroundColour = paper();
            options.componentToCentreAround = this;
            options.useNativeTitleBar = true;
            options.resizable = false;
            options.launchAsync();
        };
        addAndMakeVisible(settingsButton);
        keyboard.setAvailableRange(36, 96);
        keyboard.setLowestVisibleKey(48);
        keyboard.setKeyPressBaseOctave(4);
        keyboard.setWantsKeyboardFocus(true);
        keyboard.setKeyWidth(24);
        keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe7e8e1));
        keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, ink());
        keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent().withAlpha(0.6f));
        addAndMakeVisible(keyboard);
        midi.ensureSize(8192);
        for (auto& sample : scope) sample.store(0.0f);
        setSize(880, 550);
        setAudioChannels(0, 2);
        deviceManager.addMidiInputDeviceCallback({}, &collector);
        timerCallback();
        startTimerHz(20);
    }

    ~StudyComponent() override
    {
        stopTimer();
        deviceManager.removeMidiInputDeviceCallback({}, &collector);
        shutdownAudio();
        setLookAndFeel(nullptr);
    }

    void prepareToPlay(int, double sampleRate) override
    {
        engine.setTreatment(static_cast<SoundStudies::Treatment>(mode.load()));
        engine.setSource(static_cast<SoundStudies::Source>(sourceProfile.load()));
        const auto fixed = studyProfile.load() == 4 && mode.load() == 5;
        engine.setAmount(fixed ? 0.5f : intensity.load());
        engine.setAmountSweepEnabled(!fixed);
        engine.setNetworkAdaptive(adaptiveLinks.load());
        engine.prepare(sampleRate);
        collector.reset(sampleRate);
        outputGain.reset(sampleRate, 0.025);
        outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(trimDb.load()));
        callbackPlaying = false;
        callbackPattern = -1;
        callbackRestart = -1;
    }

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        const juce::ScopedNoDenormals noDenormals;
        info.clearActiveBufferRegion();
        if (info.buffer->getNumChannels() == 0) return;
        const auto selectedMode = mode.load();
        engine.setTreatment(static_cast<SoundStudies::Treatment>(selectedMode));
        engine.setSource(static_cast<SoundStudies::Source>(sourceProfile.load()));
        const auto fixed = studyProfile.load() == 4 && selectedMode == 5;
        engine.setAmount(fixed ? 0.5f : intensity.load());
        engine.setAmountSweepEnabled(!fixed);
        const auto adaptive = adaptiveLinks.load();
        engine.setNetworkAdaptive(adaptive);
        const auto nextPlaying = playing.load();
        const auto nextPattern = pattern.load();
        const auto nextRestart = restart.load();
        if (nextPlaying != callbackPlaying || nextPattern != callbackPattern || nextRestart != callbackRestart)
        {
            if (nextPlaying) engine.start(static_cast<SoundStudies::Pattern>(nextPattern));
            else engine.stop();
            callbackPlaying = nextPlaying;
            callbackPattern = nextPattern;
            callbackRestart = nextRestart;
        }
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
            if (message.isNoteOn())
            {
                const auto articulation = nextPattern == 4 ? SoundStudies::Articulation::lead
                    : nextPattern == 0 || nextPattern == 3 ? SoundStudies::Articulation::pad : SoundStudies::Articulation::hit;
                engine.noteOn(message.getNoteNumber(), message.getFloatVelocity(), articulation);
            }
            else if (message.isNoteOff()) engine.noteOff(message.getNoteNumber());
            else if (message.isPitchWheel()) engine.setPitchBend(static_cast<float>(message.getPitchWheelValue() - 8192) * 2.0f / 8192.0f);
            else if (message.isAllNotesOff() || message.isAllSoundOff()) engine.allNotesOff();
            offset = position;
        }
        engine.render(left + offset, right + offset, info.numSamples - offset);
        const auto a = engine.getAmount();
        // Fixed curve measured from the exported pad/hit/velocity studies, not an AGC.
        // Keeping gain independent of measured input preserves velocity and envelope differences.
        const auto compensation = compensationDb(selectedMode, sourceProfile.load(), a, adaptive);
        outputGain.setTargetValue(juce::Decibels::decibelsToGain(trimDb.load() + compensation));
        float blockPeak = 0.0f;
        for (int i = 0; i < info.numSamples; ++i)
        {
            const auto gain = outputGain.getNextValue();
            left[i] = juce::jlimit(-0.95f, 0.95f, left[i] * gain);
            if (right != left) right[i] = juce::jlimit(-0.95f, 0.95f, right[i] * gain);
            blockPeak = std::max(blockPeak, std::abs(left[i]));
            if (i % 8 == 0)
            {
                scope[scopeCursor].store(left[i], std::memory_order_relaxed);
                scopeCursor = (scopeCursor + 1) % scope.size();
            }
        }
        scopeHead.store(scopeCursor, std::memory_order_release);
        peak.store(blockPeak);
        currentAmount.store(engine.getAmount());
        numericalFaults.store(engine.getNumericalFaults());
    }

    void beginAudioCheck(int slot, bool pad, bool adaptive = true)
    {
        adaptiveButton.setToggleState(adaptive, juce::dontSendNotification);
        adaptiveButton.onClick();
        patternBox.setSelectedId(pad ? 1 : 5, juce::sendNotificationSync);
        modeButtons[static_cast<size_t>(slot)].setToggleState(true, juce::dontSendNotification);
        modeButtons[static_cast<size_t>(slot)].onClick();
        amount.setValue(100);
        playing.store(true);
    }

    bool finishAudioCheck()
    {
        const auto activeDevice = deviceManager.getCurrentAudioDevice() != nullptr;
        const auto faults = numericalFaults.load();
        const auto cpu = deviceManager.getCpuUsage();
        const auto outputPeak = peak.load();
        std::cout << "Native audio check (mode " << mode.load() << ", phrase " << pattern.load()
                  << ", adaptive " << adaptiveLinks.load() << "): " << deviceStatus << ", peak " << outputPeak
                  << ", audio CPU " << cpu * 100 << "%, xruns " << deviceManager.getXRunCount()
                  << ", numerical faults " << faults << '\n';
        playing.store(false);
        return activeDevice && outputPeak > 0.001f && faults == 0 && cpu < 0.9 && deviceManager.getXRunCount() == 0;
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(paper());
        g.setColour(ink());
        g.setFont(juce::Font(juce::FontOptions("Helvetica Neue", 27, juce::Font::bold)));
        g.drawText("nitride / sound studies", 28, 22, 540, 40, juce::Justification::left);
        text(g, studyBox.getSelectedId() == 5 ? "05   NETWORK / EXTENDED STRESS RANGE"
             : studyBox.getSelectedId() == 4 ? "04   FM / ADAPTIVE OSCILLATOR COUPLING"
             : studyBox.getSelectedId() == 3 ? "03   ORIGINAL / NOTE-COUPLED PHASE DELAY"
             : studyBox.getSelectedId() == 2 ? "02   NONLINEAR INTERACTION / PHASE DELAY" : "01   FM / BODY / SPECTRAL",
             { 30, 67, 430, 18 }, muted(), 10);
        text(g, "SOURCE", { 477, 67, 80, 18 }, muted(), 10);
        g.setColour(juce::Colour(0xffb6b9af));
        g.drawHorizontalLine(98, 28.0f, 852.0f);
        text(g, "TREATMENT", { 30, 113, 550, 17 });
        text(g, "TEST PHRASE", { 30, 195, 350, 16 });
        const auto selected = mode.load();
        const auto sweep = pattern.load() == 3;
        text(g, fixedReference.load() ? "F REFERENCE / FIXED AT 50%" : selected == 7 ? "EXTENDED INTERACTION"
             : selected == 6 ? "NETWORK INTERACTION"
             : selected == 1 ? "COUPLING" : selected == 2 ? "SPREAD" : selected == 3 ? "INTERACTION"
             : selected >= 4 ? "DEFORMATION" : "TREATMENT AMOUNT", { 30, 265, 350, 18 });
        text(g, selected == 7 ? "0-50: G RANGE / 50-100: STRESS"
             : sweep && !fixedReference.load() ? "SEQUENCE: 0 > 100 > 0" : "",
             { 490, 265, 360, 18 }, muted(), 10, juce::Justification::right);
        text(g, "OUTPUT TRIM", { 600, 195, 250, 16 });
        g.setColour(juce::Colour(0xff141b18));
        g.fillRoundedRectangle({ 30.0f, 340.0f, 820.0f, 78.0f }, 3.0f);
        text(g, "OUTPUT", { 42, 349, 120, 13 }, juce::Colour(0xffb4d7be), 8);
        const auto head = scopeHead.load(std::memory_order_acquire);
        juce::Path trace;
        for (size_t i = 0; i < scope.size(); ++i)
        {
            const auto sample = scope[(head + i) % scope.size()].load(std::memory_order_relaxed);
            const auto x = 42.0f + static_cast<float>(i) / 255.0f * 796.0f;
            const auto y = 388.0f - juce::jlimit(-1.0f, 1.0f, sample * 3.0f) * 22.0f;
            if (i == 0) trace.startNewSubPath(x, y); else trace.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xffb4d7be));
        g.strokePath(trace, juce::PathStrokeType(1));
        text(g, "MIDI / KEYBOARD", { 30, 429, 340, 16 }, muted(), 9);
        text(g, pattern.load() == 0 || pattern.load() == 3 ? "Cmaj9 / 120 BPM / NO FX"
             : pattern.load() == 4 ? "MONO / PITCH BENDS / 120 BPM" : "120 BPM / NO FX",
             { 480, 429, 370, 16 }, muted(), 9, juce::Justification::right);
        text(g, deviceStatus, { 30, 521, 590, 18 }, muted(), 9);
        const auto db = juce::Decibels::gainToDecibels(displayPeak, -100.0f);
        text(g, "PEAK " + juce::String(db, 1) + " dBFS", { 635, 521, 215, 18 }, muted(), 9, juce::Justification::right);
    }

    void resized() override
    {
        settingsButton.setBounds(718, 30, 132, 34);
        studyBox.setBounds(555, 30, 143, 34);
        sourceBox.setBounds(565, 65, 285, 28);
        adaptiveButton.setBounds(650, 109, 200, 25);
        for (size_t i = 0; i < modeButtons.size(); ++i) modeButtons[i].setBounds(30 + static_cast<int>(i) * 279, 141, 262, 36);
        patternBox.setBounds(30, 220, 302, 32);
        playButton.setBounds(347, 220, 95, 32);
        restartButton.setBounds(453, 220, 95, 32);
        level.setBounds(594, 218, 256, 36);
        amount.setBounds(25, 290, 825, 36);
        keyboard.setBounds(30, 453, 820, 55);
    }

private:
    void refreshAmountControl()
    {
        const auto fixed = studyBox.getSelectedId() == 4 && mode.load() == 5;
        fixedReference.store(fixed);
        if (fixed || patternBox.getSelectedId() != 4)
            amount.setValue(fixed ? 50 : intensity.load() * 100, juce::dontSendNotification);
        amount.setEnabled(mode.load() != 0 && !fixed && patternBox.getSelectedId() != 4);
        amount.setDoubleClickReturnValue(true, mode.load() == 7 ? 25 : 50);
        adaptiveButton.setEnabled(mode.load() >= 6);
    }

    static void text(juce::Graphics& g, const juce::String& caption, juce::Rectangle<int> bounds,
                     juce::Colour colour = ink(), float size = 10,
                     juce::Justification align = juce::Justification::left)
    {
        g.setColour(colour);
        g.setFont(mono(size));
        g.drawText(caption, bounds, align, false);
    }

    void timerCallback() override
    {
        displayPeak = std::max(peak.load(), displayPeak * 0.82f);
        if (auto* device = deviceManager.getCurrentAudioDevice())
            deviceStatus = device->getName() + " / " + juce::String(device->getCurrentSampleRate(), 0) + " Hz";
        else deviceStatus = "NO AUDIO DEVICE / OPEN AUDIO SETTINGS";
        if (pattern.load() == 3 && !fixedReference.load()) amount.setValue(currentAmount.load() * 100, juce::dontSendNotification);
        repaint();
    }

    juce::LookAndFeel_V4 lookAndFeel;
    SoundStudies::Engine engine;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard;
    juce::MidiMessageCollector collector;
    juce::MidiBuffer midi;
    juce::SmoothedValue<float> outputGain;
    std::array<juce::TextButton, 3> modeButtons;
    juce::TextButton playButton, restartButton, settingsButton;
    juce::ToggleButton adaptiveButton;
    juce::Slider amount, level;
    juce::ComboBox patternBox, studyBox, sourceBox;
    std::array<float, 8> retainedAmounts { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.25f };
    std::atomic<int> mode { 0 }, pattern { 4 }, sourceProfile { 1 }, studyProfile { 5 }, restart { 0 };
    std::atomic<bool> playing { false };
    std::atomic<bool> fixedReference { false }, adaptiveLinks { true };
    std::atomic<float> intensity { 0.25f }, trimDb { 0 }, peak { 0 }, currentAmount { 0.25f };
    std::atomic<std::uint64_t> numericalFaults { 0 };
    std::array<std::atomic<float>, 256> scope;
    std::atomic<size_t> scopeHead { 0 };
    size_t scopeCursor = 0;
    bool callbackPlaying = false;
    int callbackPattern = -1, callbackRestart = -1;
    float displayPeak = 0;
    juce::String deviceStatus;
};

class StudyApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Nitride Sound Studies"; }
    const juce::String getApplicationVersion() override { return "0.5"; }
    void initialise(const juce::String& commandLine) override
    {
        window = std::make_unique<Window>();
        if (commandLine == "--audio-check")
            runAudioCheck(0, true);
        if (commandLine.startsWith("--snapshot "))
        {
            const auto path = commandLine.fromFirstOccurrenceOf("--snapshot ", false, false).trim().unquoted();
            juce::MessageManager::callAsync([this, path] {
                auto* content = window->getContentComponent();
                const auto image = content->createComponentSnapshot(content->getLocalBounds(), true, 2.0f);
                if (auto stream = juce::File(path).createOutputStream())
                {
                    stream->setPosition(0);
                    stream->truncate();
                    juce::PNGImageFormat().writeImageToStream(image, *stream);
                }
                quit();
            });
        }
    }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    void runAudioCheck(int index, bool passed)
    {
        if (index == 6)
        {
            setApplicationReturnValue(passed ? 0 : 1);
            quit();
            return;
        }
        auto* content = static_cast<StudyComponent*>(window->getContentComponent());
        content->beginAudioCheck(index >= 4 ? 2 : index % 2 + 1,
                                 index == 2 || index == 3 || index == 5, index < 4);
        juce::Timer::callAfterDelay(2200, [this, content, index, passed] {
            runAudioCheck(index + 1, content->finishAudioCheck() && passed);
        });
    }

    class Window final : public juce::DocumentWindow
    {
    public:
        Window() : DocumentWindow("Nitride / sound studies", paper(), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new StudyComponent(), true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };
    std::unique_ptr<Window> window;
};
}

START_JUCE_APPLICATION(StudyApplication)
