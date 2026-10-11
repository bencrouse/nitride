#include "InstrumentPanel.h"
#include "SupportingControls.h"
#include "PitchMotionPanel.h"
#include "InstrumentKeyboard.h"
#include <iostream>

namespace Nitride
{
using namespace UI;
class InstrumentPanel::Content final : public juce::Component,private juce::Timer
{
public:
    Content(InstrumentSession& s,std::function<void()> settingsCallback)
        :session(s),live(s.live),audioSettings(std::move(settingsCallback)),glidePanel(s,false),autobendPanel(s,true),field(live),ribbon(live),keyboard(s),
        fmValue("FM depth",0,4,1.65),pitchValue("Pitch",-12,12,0),attackValue("Attack",.001,4,.65,true),decayValue("Decay",.01,4,.7,true),
        sustainValue("Sustain",0,1,.72),releaseValue("Release",.02,8,1.4,true),outputValue("Output",-24,0,-6),
        filter(XYControl::Kind::filter),motion(XYControl::Kind::motion),space(XYControl::Kind::space)
    {
        setLookAndFeel(&look);
        look.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff252b27));look.setColour(juce::TextButton::buttonOnColourId,accent());
        look.setColour(juce::TextButton::textColourOffId,ink());look.setColour(juce::TextButton::textColourOnId,glass());
        look.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff252b27));look.setColour(juce::ComboBox::textColourId,ink());
        look.setColour(juce::ComboBox::arrowColourId,muted());look.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff353d36));
        look.setColour(juce::PopupMenu::backgroundColourId,chassis());look.setColour(juce::PopupMenu::textColourId,ink());
        session.attachEditor();editorGeneration=session.editorMidi.epoch();rebuildPresets();
        patchBox.setName("Starting patch");patchBox.onChange=[this]{const auto i=patchBox.getSelectedId()-1;if(i>=0&&i<static_cast<int>(presetIds.size())){endAudition();hold.setToggleState(false,juce::dontSendNotification);session.selectPreset(presetIds[static_cast<size_t>(i)]);sync();}};
        undo.setButtonText("UNDO");undo.onClick=[this]{session.undo();sync();};
        compare.setButtonText("COMPARE");compare.setClickingTogglesState(true);compare.onClick=[this]{session.setComparing(compare.getToggleState());sync();};
        keep.setButtonText("KEEP");keep.setTooltip("Name and save this sound to your Nitride preset library");keep.onClick=[this]{promptKeep();};
        hold.setButtonText("HOLD");hold.setClickingTogglesState(true);hold.onClick=[this]{if(hold.getToggleState())startAudition(true);else if(!touching)endAudition();};
        settings.setButtonText("...");settings.onClick=[this]{showSettings();};
        for(auto* button:{&undo,&compare,&keep,&hold,&settings})addAndMakeVisible(button);addAndMakeVisible(patchBox);
        field.onGestureBegin=[this]{begin({coupling,stress});};field.onGestureEnd=[this]{end({coupling,stress});};
        ribbon.onGestureBegin=[this]{begin({response});};ribbon.onGestureEnd=[this]{end({response});};
        field.onShape=[this](float c,float t){group({{coupling,c},{stress,t}});};
        field.onBegin=[this]{touching=true;if(current.values[sustain]<.01&&!notes.isEmpty())endAudition();startAudition();};
        field.onEnd=[this]{touching=false;if(!hold.getToggleState())endAudition();};
        ribbon.onResponse=[this](double v){set(response,v);};ribbon.onBegin=[this]{touching=true;startAudition();};ribbon.onEnd=field.onEnd;
        addAndMakeVisible(field);addAndMakeVisible(ribbon);
        for(auto pair:{std::pair{&fmValue,fm},{&pitchValue,pitch},{&attackValue,attack},{&decayValue,decay},{&sustainValue,sustain},{&releaseValue,release},{&outputValue,output}})
        {
            const auto parameter=pair.second;auto* value=pair.first;value->onBegin=[this,parameter]{begin({hostIndex(parameter)});};value->onEnd=[this,parameter]{end({hostIndex(parameter)});};
            value->onChange=[this,parameter](double v){set(parameter,v);};addAndMakeVisible(*value);
        }
        fmValue.format=[](double v){return juce::String(v,2);};pitchValue.format=[](double v){return(v>0?"+":"")+juce::String(v,1)+" st";};
        for(auto* value:{&attackValue,&decayValue,&releaseValue})value->format=[](double v){return v>=1?juce::String(v,2)+" s":juce::String(v*1000,0)+" ms";};
        sustainValue.format=[](double v){return juce::String(v*100,0)+"%";};outputValue.format=[](double v){return juce::String(v,1)+" dB";};
        poly.setButtonText("POLY");monoButton.setButtonText("MONO");legatoButton.setButtonText("LEGATO");legatoButton.setName("Legato play mode");
        poly.onClick=[this]{playMode(0);};monoButton.onClick=[this]{playMode(1);};legatoButton.onClick=[this]{playMode(2);};
        for(auto* b:{&poly,&monoButton,&legatoButton})addAndMakeVisible(*b);addAndMakeVisible(glidePanel);addAndMakeVisible(autobendPanel);
        envelope.onBegin=[this]{begin({attack,decay,sustain,release});};envelope.onEnd=[this]{end({attack,decay,sustain,release});};
        envelope.onChange=[this](std::array<double,4> v){group({{attack,v[0]},{decay,v[1]},{sustain,v[2]},{release,v[3]}});};addAndMakeVisible(envelope);
        filter.onBegin=[this]{begin({cutoff,resonance});};filter.onEnd=[this]{end({cutoff,resonance});};
        motion.onBegin=[this]{begin({motionRate,motionDepth});};motion.onEnd=[this]{end({motionRate,motionDepth});};
        space.onBegin=[this]{begin({spaceMix,spaceSize});};space.onEnd=[this]{end({spaceMix,spaceSize});};
        filter.onChange=[this](float x,float y){group({{cutoff,40*std::pow(450.0,x)},{resonance,y*.85}});};
        motion.onChange=[this](float x,float y){group({{motionRate,.05*std::pow(160.0,x)},{motionDepth,y}});};space.onChange=[this](float x,float y){group({{spaceMix,x*.65},{spaceSize,y}});};
        for(auto* p:{&filter,&motion,&space})addAndMakeVisible(*p);
        keyboard.setAvailableRange(48,84);keyboard.setKeyPressBaseOctave(4);keyboard.setOctaveForMiddleC(4);keyboard.setMidiChannel(15);
        keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xff2b312b));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,glass());
        keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId,muted());keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,accent().withAlpha(.65f));addAndMakeVisible(keyboard);
        sync();setSize(1200,840);startTimerHz(30);
    }
    ~Content() override{stopTimer();endAudition();session.endAllEdits();session.detachEditor();setLookAndFeel(nullptr);}
    void beginPreview(){hold.setToggleState(true,juce::dontSendNotification);startAudition();}
    bool checkInteractions()
    {
        session.apply(factoryPatches()[0]);session.setCompareReference(session.read());sync();const auto initial=current.values[coupling];
        field.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));const auto changed=current.values[coupling]>initial;undo.onClick();const auto undone=std::abs(current.values[coupling]-initial)<.001;
        fmValue.getAccessibilityHandler()->getValueInterface()->setValue(3.2);const auto source=std::abs(current.values[fm]-3.2)<.001;
        envelope.onChange({.1,.2,.5,.4});const auto env=std::abs(current.values[attack]-.1)<.001;
        filter.onChange(.7f,.4f);motion.onChange(.4f,.2f);space.onChange(.3f,.7f);const auto support=current.values[cutoff]>1000&&current.values[motionDepth]>.1&&current.values[spaceMix]>.1;
        const auto count=session.presets().size();const auto kept=session.keep("Interaction test").wasOk()&&session.presets().size()==count+1;rebuildPresets();sync();
        const auto saved=current;set(stress,.8);session.setComparing(true);sync();const auto compared=std::abs(current.values[stress]-saved.values[stress])<.001;
        session.setComparing(false);sync();const auto returned=std::abs(current.values[stress]-.8)<.001;
        field.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey));const auto played=!notes.isEmpty();field.keyStateChanged(false);const auto stopped=notes.isEmpty();
        std::cout<<"Native interaction checks: "<<changed<<undone<<source<<env<<support<<kept<<compared<<returned<<played<<stopped<<'\n';
        return changed&&undone&&source&&env&&support&&kept&&compared&&returned&&played&&stopped;
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(chassis());g.setColour(juce::Colour(0xff1b201d));g.fillRect(0,92,286,424);g.fillRect(918,92,282,424);g.setColour(juce::Colour(0xff30372f));
        for(int y:{92,530,726})g.drawHorizontalLine(y,24,1176);g.drawVerticalLine(286,110,510);g.drawVerticalLine(918,110,510);g.drawVerticalLine(392,553,710);g.drawVerticalLine(800,553,710);
        g.setColour(ink());g.setFont(juce::Font(juce::FontOptions("Helvetica Neue",36,juce::Font::bold)));g.drawText("nitride",25,23,200,43,juce::Justification::left);
        text(g,"PATCH",{300,13,120,14},muted(),8);section(g,"SOUND",{24,112,238,18});section(g,"NETWORK",{310,112,584,18});section(g,"AMPLITUDE",{942,112,234,18});
        text(g,"COUPLING "+juce::String(current.values[coupling]*100,0)+"%   /   STRESS "+juce::String(current.values[stress]*100,0)+"%",{310,428,584,17},muted(),8);
        sourceWave(g);text(g,"SOURCE / SINE FM",{36,190,225,14},muted(),8);
        section(g,"TONE",{24,550,344,18});text(g,"LP / 12 dB",{205,550,163,18},muted(),8,juce::Justification::right);
        section(g,"MOTION",{416,550,360,18});text(g,"SINE > COUPLING",{580,550,196,18},muted(),8,juce::Justification::right);
        section(g,"SPACE",{824,550,352,18});text(g,"DIFFUSION",{1000,550,176,18},muted(),8,juce::Justification::right);
        text(g,formatCutoff(current.values[cutoff])+"  /  RES "+juce::String(current.values[resonance]*100,0)+"%",{36,692,332,17},muted(),9);
        text(g,juce::String(current.values[motionRate],2)+" Hz  /  DEPTH "+juce::String(current.values[motionDepth]*100,0)+"%",{428,692,348,17},muted(),9);
        text(g,"MIX "+juce::String(current.values[spaceMix]*100,0)+"%  /  SIZE "+juce::String(current.values[spaceSize]*100,0)+"%",{836,692,340,17},muted(),9);
        text(g,"NATIVE INSTRUMENT",{26,808,260,17},muted(),8);text(g,juce::String(live.voices.load())+" VOICES",{600,808,146,17},muted(),8);text(g,"AUDIO PROCESSOR",{250,808,340,17},muted(),8);
        const auto peak=live.peak.load();for(int i=0;i<14;++i){g.setColour(peak>static_cast<float>(i)/20?accent().withAlpha(.8f):juce::Colour(0xff30372f));g.fillRect(790+i*8,813,5,10);}
    }
    void resized() override
    {
        patchBox.setBounds(300,33,330,34);undo.setBounds(668,33,70,34);compare.setBounds(746,33,94,34);keep.setBounds(848,33,70,34);hold.setBounds(968,33,80,34);settings.setBounds(1102,33,74,34);
        field.setBounds(310,145,584,274);ribbon.setBounds(310,448,584,82);fmValue.setBounds(36,208,226,28);pitchValue.setBounds(36,242,226,28);
        poly.setBounds(36,280,70,24);monoButton.setBounds(112,280,70,24);legatoButton.setBounds(188,280,74,24);glidePanel.setBounds(36,317,226,85);autobendPanel.setBounds(36,418,226,85);
        envelope.setBounds(942,145,234,147);attackValue.setBounds(942,309,234,34);decayValue.setBounds(942,351,234,34);sustainValue.setBounds(942,393,234,34);releaseValue.setBounds(942,435,234,34);
        filter.setBounds(36,586,332,96);motion.setBounds(428,586,348,96);space.setBounds(836,586,340,96);keyboard.setBounds(26,741,1148,53);keyboard.setKeyWidth(1148.0f/22);outputValue.setBounds(914,803,262,28);
    }
private:
    void set(Parameter p,double value){session.setIfCurrent(p,value,lastEpoch);sync();}
    void group(std::initializer_list<std::pair<Parameter,double>> updates){session.setGroupIfCurrent(updates,lastEpoch);sync();}
    void begin(std::initializer_list<int> parameters)
    {
        if(lastEpoch!=session.restoreEpoch.load())sync();for(const auto i:parameters)if(edits[static_cast<size_t>(i)])return;
        session.beginEdit(parameters);for(const auto i:parameters)edits[static_cast<size_t>(i)]=true;sync();
    }
    void end(std::initializer_list<int> parameters)
    {bool active=false;for(const auto i:parameters)active|=edits[static_cast<size_t>(i)];if(!active)return;for(const auto i:parameters)edits[static_cast<size_t>(i)]=false;session.endEdit(parameters);sync();}
    void playMode(int mode)
    {session.beginEdit({monoHostIndex,hostIndex(legato)});session.setMono(mode!=0);session.set(legato,mode==2?1:0);session.endEdit({monoHostIndex,hostIndex(legato)});sync();}
    void rebuildPresets()
    {
        const auto revision=session.libraryRevision.load();presetIds.clear();patchBox.clear(juce::dontSendNotification);const auto presets=session.presets();
        for(size_t i=0;i<presets.size();++i){if(i==0)patchBox.addSectionHeading("Starting patches");if(i==4)patchBox.addSectionHeading("Kept sounds");patchBox.addItem(presets[i].patch.name,static_cast<int>(i)+1);presetIds.push_back(presets[i].id);}lastLibrary=revision;
    }
    void sync()
    {
        const auto revision=session.revision.load(std::memory_order_acquire),epoch=session.restoreEpoch.load();
        if(lastEpoch!=epoch)
        {
            endAudition();hold.setToggleState(false,juce::dontSendNotification);touching=false;edits={};field.cancelGesture();ribbon.cancelGesture();envelope.cancelGesture();
            for(auto* v:{&fmValue,&pitchValue,&attackValue,&decayValue,&sustainValue,&releaseValue,&outputValue})v->cancelGesture();for(auto* p:{&filter,&motion,&space})p->cancelGesture();lastEpoch=epoch;
        }
        if(lastLibrary!=session.libraryRevision.load())rebuildPresets();current=session.readVisible();glidePanel.sync(current,epoch);autobendPanel.sync(current,epoch);
        field.setPosition(static_cast<float>(current.values[coupling]),static_cast<float>(current.values[stress]));ribbon.setResponse(current.values[response]);
        fmValue.setValue(current.values[fm]);pitchValue.setValue(current.values[pitch]);outputValue.setValue(current.values[output]);
        attackValue.setValue(current.values[attack]);decayValue.setValue(current.values[decay]);sustainValue.setValue(current.values[sustain]);releaseValue.setValue(current.values[release]);envelope.setEnvelope({current.values[attack],current.values[decay],current.values[sustain],current.values[release]});
        filter.setPosition(static_cast<float>(std::log(current.values[cutoff]/40)/std::log(450.0)),static_cast<float>(current.values[resonance]/.85));
        motion.setPosition(static_cast<float>(std::log(current.values[motionRate]/.05)/std::log(160.0)),static_cast<float>(current.values[motionDepth]));space.setPosition(static_cast<float>(current.values[spaceMix]/.65),static_cast<float>(current.values[spaceSize]));
        poly.setToggleState(!current.mono,juce::dontSendNotification);monoButton.setToggleState(current.mono&&current.values[legato]<.5,juce::dontSendNotification);legatoButton.setToggleState(current.mono&&current.values[legato]>=.5,juce::dontSendNotification);
        compare.setToggleState(session.isComparing(),juce::dontSendNotification);undo.setEnabled(session.canUndo());const auto id=session.selectedPresetId();bool found=false;
        for(size_t i=0;i<presetIds.size();++i)if(presetIds[i]==id){patchBox.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);found=true;break;}if(!found){patchBox.setSelectedId(0,juce::dontSendNotification);patchBox.setText(current.name,juce::dontSendNotification);}
        lastRevision=revision;repaint();
    }
    void startAudition(bool capture=false)
    {
        if(editorGeneration!=session.editorMidi.epoch()){editorGeneration=session.editorMidi.epoch();session.synchronizeEditorInput();notes.clear();}
        if(!notes.isEmpty())return;if(capture)for(int n=0;n<128;++n)if(session.auditionNote(n))notes.add(n);
        if(notes.isEmpty()){if(live.externalKeys.load()!=0)return;notes=current.chord&&!current.mono?juce::Array<int>{48,55,59,62}:juce::Array<int>{60};}
        for(const auto n:notes)session.keyboard.noteOn(16,n,.74f);
    }
    void endAudition(){for(const auto n:notes)session.keyboard.noteOff(16,n,0);notes.clear();}
    void showResult(const juce::Result& result)
    {if(result.failed())juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Nitride preset",result.getErrorMessage());else{rebuildPresets();sync();}}
    void promptKeep()
    {
        const auto patch=session.read();auto* dialog=new juce::AlertWindow("Keep sound","Save these sound conditions to your Nitride library.",juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor("name",patch.name,"Name");dialog->addButton("Keep",1,juce::KeyPress(juce::KeyPress::returnKey));dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
        const juce::Component::SafePointer<Content> safe(this);dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe,dialog,patch](int r){if(safe&&r==1)safe->showResult(safe->session.keepPatch(dialog->getTextEditorContents("name"),patch));}),true);
    }
    void filePreset(bool importing)
    {
        chooser=std::make_unique<juce::FileChooser>(importing?"Import Nitride preset":"Export Nitride preset",importing?session.presetDirectory():session.presetDirectory().getChildFile("Sound.nitridepreset"),"*.nitridepreset");
        const juce::Component::SafePointer<Content> safe(this);const auto flags=importing?(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles):(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting);
        chooser->launchAsync(flags,[safe,importing](const juce::FileChooser& c){if(!safe)return;const auto f=c.getResult();if(importing&&f.existsAsFile())safe->showResult(safe->session.importPreset(f));else if(!importing&&f!=juce::File{})safe->showResult(safe->session.exportPreset(f.withFileExtension(".nitridepreset")));});
    }
    void showSettings()
    {
        juce::PopupMenu menu;if(audioSettings)menu.addItem(1,"Audio / MIDI...");menu.addItem(2,"Reduced motion",true,field.reducedMotion);menu.addItem(3,"About Nitride");menu.addSeparator();menu.addItem(4,"Redo",session.canRedo());menu.addItem(5,"Import preset...");menu.addItem(6,"Export preset...");menu.addItem(7,"Refresh kept sounds");
        const juce::Component::SafePointer<Content> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),[safe](int choice){if(!safe)return;if(choice==1&&safe->audioSettings)safe->audioSettings();else if(choice==2){safe->field.reducedMotion=!safe->field.reducedMotion;safe->ribbon.reducedMotion=safe->field.reducedMotion;}else if(choice==4){safe->session.redo();safe->sync();}else if(choice==5)safe->filePreset(true);else if(choice==6)safe->filePreset(false);else if(choice==7){safe->session.refreshPresets();safe->rebuildPresets();safe->sync();}else if(choice==3)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Nitride","The native field, pitch motion, source, envelope, tone, motion and space are connected to the audio processor. Keep saves presets; Compare retains edits.");});
    }
    static void section(juce::Graphics& g,const juce::String& label,juce::Rectangle<int> r){g.setColour(ink());g.setFont(juce::Font(juce::FontOptions("Helvetica Neue",11,juce::Font::bold)));g.drawText(label,r,juce::Justification::left);}
    static juce::String formatCutoff(double hz){return hz>=1000?juce::String(hz/1000,2)+" kHz":juce::String(hz,0)+" Hz";}
    void sourceWave(juce::Graphics& g)
    {
        const juce::Rectangle<float> p{36,145,226,40};g.setColour(glass());g.fillRoundedRectangle(p,4);juce::Path wave;
        for(int i=0;i<=140;++i){const auto t=static_cast<float>(i)/140,phase=t*pi*4;const auto sample=std::sin(phase+static_cast<float>(current.values[fm])*std::sin(phase*2));const auto x=p.getX()+12+t*(p.getWidth()-24),y=p.getCentreY()-sample*12;if(i==0)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}g.setColour(signal().withAlpha(.7f));g.strokePath(wave,juce::PathStrokeType(1.1f));
    }
    void timerCallback() override
    {
        if(editorGeneration!=session.editorMidi.epoch()){editorGeneration=session.editorMidi.epoch();session.synchronizeEditorInput();notes.clear();hold.setToggleState(false,juce::dontSendNotification);touching=false;}
        if(lastRevision!=session.revision.load(std::memory_order_acquire)||lastLibrary!=session.libraryRevision.load())sync();keyboard.repaint();field.repaint();ribbon.repaint();repaint(24,803,1152,34);
    }
    juce::LookAndFeel_V4 look;InstrumentSession& session;LiveState& live;std::function<void()> audioSettings;
    PitchMotionPanel glidePanel,autobendPanel;PlayingField field;ResponseRibbon ribbon;InstrumentKeyboard keyboard;
    ValueControl fmValue,pitchValue,attackValue,decayValue,sustainValue,releaseValue,outputValue;EnvelopeControl envelope;XYControl filter,motion,space;
    juce::ComboBox patchBox;juce::TextButton undo,compare,keep,hold,settings,poly,monoButton,legatoButton;
    std::vector<juce::String> presetIds;std::unique_ptr<juce::FileChooser> chooser;Patch current;juce::Array<int> notes;
    std::array<bool,hostParameterCount> edits{};std::uint64_t lastRevision=0,lastLibrary=0,lastEpoch=0,editorGeneration=0;bool touching=false;
};
InstrumentPanel::InstrumentPanel(InstrumentSession& s,std::function<void()> settings):content(std::make_unique<Content>(s,std::move(settings))){addAndMakeVisible(*content);setSize(1200,840);}
InstrumentPanel::~InstrumentPanel()=default;
void InstrumentPanel::resized(){content->setBounds(getLocalBounds());}
void InstrumentPanel::beginPreview(){content->beginPreview();}
bool InstrumentPanel::checkInteractions(){return content->checkInteractions();}
}
