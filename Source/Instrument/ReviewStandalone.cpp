#include "InstrumentRenderer.h"
#include "InstrumentView.h"
#include <iostream>

namespace
{
class ReviewStandalone final : public juce::AudioAppComponent
{
public:
    explicit ReviewStandalone(juce::File presetDirectory = {}) : session(std::move(presetDirectory)),renderer(session), view(session, [this] { showAudioSettings(); })
    {
        addAndMakeVisible(view);
        midi.ensureSize(8192);
        setSize(1200, 840);
        setAudioChannels(0, 2);
        deviceManager.addMidiInputDeviceCallback({}, &collector);
    }
    ~ReviewStandalone() override
    {
        deviceManager.removeMidiInputDeviceCallback({}, &collector);
        shutdownAudio();
    }
    void resized() override { view.setBounds(getLocalBounds()); }
    void prepareToPlay(int, double rate) override { renderer.prepare(rate); collector.reset(rate); }
    void releaseResources() override { renderer.reset(); }
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        const juce::ScopedNoDenormals noDenormals;
        if (info.buffer->getNumChannels() == 0) return;
        midi.clear(); collector.removeNextBlockOfMessages(midi, info.numSamples);
        float* pointers[] {info.buffer->getWritePointer(0,info.startSample),
            info.buffer->getWritePointer(info.buffer->getNumChannels()>1?1:0,info.startSample)};
        juce::AudioBuffer<float> block(pointers, info.buffer->getNumChannels()>1?2:1, info.numSamples);
        renderer.process(block, midi);
    }
    void beginPreview() { view.beginPreview(); }
    bool checkInteractions() { return view.checkInteractions(); }
    void beginAudioCheck() { view.beginPreview(); }
    void changeAudioCheck() { session.set(Nitride::coupling,.95); session.set(Nitride::stress,.8); session.set(Nitride::response,.035); }
    bool finishAudioCheck()
    {
        const auto passed=session.live.peak.load()>.0001f&&session.live.faults.load()==0
            &&deviceManager.getXRunCount()==0&&deviceManager.getCpuUsage()<.9;
        std::cout<<"Shared instrument playback: peak "<<session.live.peak.load()<<", CPU "<<deviceManager.getCpuUsage()*100
            <<"%, xruns "<<deviceManager.getXRunCount()<<", faults "<<session.live.faults.load()<<'\n';
        for(int note=0;note<128;++note)session.keyboard.noteOff(16,note,0);
        return passed;
    }
    bool checkReleased() { return session.live.voices.load()==0; }
    void snapshot(const juce::String& path)
    {
        const auto image=view.createComponentSnapshot(view.getLocalBounds(),true,2);
        if(auto stream=juce::File(path).createOutputStream())
        {
            stream->setPosition(0);stream->truncate();juce::PNGImageFormat().writeImageToStream(image,*stream);
        }
    }
private:
    void showAudioSettings()
    {
        auto selector=std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager,0,0,0,2,true,false,true,false);
        selector->setSize(500,430);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(selector.release());options.dialogTitle="Nitride / Audio and MIDI";
        options.dialogBackgroundColour=juce::Colour(0xff151817);options.componentToCentreAround=this;
        options.useNativeTitleBar=true;options.launchAsync();
    }
    Nitride::InstrumentSession session;
    Nitride::InstrumentRenderer renderer;
    Nitride::InstrumentView view;
    juce::MidiMessageCollector collector;
    juce::MidiBuffer midi;
};

class ReviewApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Nitride Instrument Review"; }
    const juce::String getApplicationVersion() override { return "0.9"; }
    void initialise(const juce::String& commandLine) override
    {
        if(commandLine=="--interaction-check")testPresets=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("nitride-review-tests","",false);
        window=std::make_unique<Window>(testPresets);
        auto* component=static_cast<ReviewStandalone*>(window->getContentComponent());
        if(commandLine=="--interaction-check") {setApplicationReturnValue(component->checkInteractions()?0:1);quit();}
        if(commandLine=="--audio-check")
        {
            component->beginAudioCheck();
            juce::Timer::callAfterDelay(1800,[this,component]{component->changeAudioCheck();
                juce::Timer::callAfterDelay(1800,[this,component]{const auto passed=component->finishAudioCheck();
                    juce::Timer::callAfterDelay(1800,[this,component,passed]{setApplicationReturnValue(component->checkReleased()&&passed?0:1);quit();});});});
        }
        if(commandLine.startsWith("--snapshot ")||commandLine.startsWith("--snapshot-active "))
        {
            const auto active=commandLine.startsWith("--snapshot-active ");
            const auto path=commandLine.fromFirstOccurrenceOf(active?"--snapshot-active ":"--snapshot ",false,false).trim().unquoted();
            if(active)component->beginPreview();
            juce::Timer::callAfterDelay(active?900:10,[component,path]{component->snapshot(path);quit();});
        }
    }
    void shutdown() override {window.reset();if(testPresets!=juce::File{})testPresets.deleteRecursively();}
    void systemRequestedQuit() override {quit();}
private:
    class Window final : public juce::DocumentWindow
    {
    public:
        explicit Window(juce::File presetDirectory):DocumentWindow("Nitride / instrument review",juce::Colour(0xff151817),DocumentWindow::allButtons)
        {setUsingNativeTitleBar(true);setContentOwned(new ReviewStandalone(std::move(presetDirectory)),true);centreWithSize(getWidth(),getHeight());setVisible(true);}
        void closeButtonPressed() override {juce::JUCEApplication::getInstance()->systemRequestedQuit();}
    };
    std::unique_ptr<Window> window;
    juce::File testPresets;
};
}

START_JUCE_APPLICATION(ReviewApplication)
