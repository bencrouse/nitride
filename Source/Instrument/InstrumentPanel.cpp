#include "InstrumentPanel.h"
#include "SupportingControls.h"
#include "PitchMotionPanel.h"
#include "InstrumentKeyboard.h"
#include "MotionPanel.h"
#include "FmPanel.h"
#include "../DSP/Fm.h"
#include "BuildInfo.h"
#include "FactoryPrograms.h"
#include <iostream>

namespace Nitride
{
using namespace UI;
class InstrumentPanel::Content final : public juce::Component,private juce::Timer
{
public:
    Content(InstrumentSession& s,std::function<void()> settingsCallback,PresetPresentation presentation)
        :session(s),live(s.live),audioSettings(std::move(settingsCallback)),presetPresentation(presentation),glidePanel(s,false),autobendPanel(s,true),field(live),ribbon(live),keyboard(s),
        fmValue("FM depth",0,4,1.65),pitchValue("Pitch",-12,12,0),attackValue("Attack",.001,4,.65,true),decayValue("Decay",.01,4,.7,true),
        sustainValue("Sustain",0,1,.72),releaseValue("Release",.02,8,1.4,true),outputValue("Output",-24,0,-6),
        filterAttackValue("Attack",.001,4,.01,true),filterDecayValue("Decay",.01,4,.3,true),filterSustainValue("Sustain",0,1,.5),
        filterReleaseValue("Release",.02,8,.4,true),filterAmountValue("Amount",-6,6,0),filterEnvelope("Filter envelope"),
        filterTrackingValue("Key track",0,1,0),filterVelocityValue("Velocity",-6,6,0),filterFmValue("Filter FM",0,6,0),
        filter(XYControl::Kind::filter),motion(s),space(XYControl::Kind::space),advancedFm(s)
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
        for(auto* button:{&undo,&compare,&hold,&settings})addAndMakeVisible(button);
        if(!hostPresets()){addAndMakeVisible(keep);addAndMakeVisible(patchBox);}
        fmDisclosure.setButtonText("FM...");fmDisclosure.setName("Open advanced FM");fmDisclosure.setClickingTogglesState(true);
        fmDisclosure.onClick=[this]{advancedFm.finishEdits();advancedFm.setVisible(fmDisclosure.getToggleState());if(advancedFm.isVisible()){advancedFm.toFront(true);advancedFm.grabKeyboardFocus();}};
        advancedFm.onClose=[this]{fmDisclosure.setToggleState(false,juce::dontSendNotification);advancedFm.setVisible(false);fmDisclosure.grabKeyboardFocus();};
        addAndMakeVisible(fmDisclosure);addChildComponent(advancedFm);
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
        fmValue.unit=ValueUnit::index;pitchValue.unit=ValueUnit::semitones;outputValue.unit=ValueUnit::decibels;sustainValue.unit=ValueUnit::normalized;
        for(auto* value:{&attackValue,&decayValue,&releaseValue})value->unit=ValueUnit::seconds;
        for(auto* value:{&attackValue,&decayValue,&releaseValue})value->format=[](double v){return v>=1?juce::String(v,2)+" s":juce::String(v*1000,0)+" ms";};
        sustainValue.format=[](double v){return juce::String(v*100,0)+"%";};outputValue.format=[](double v){return juce::String(v,1)+" dB";};
        for(auto pair:{std::pair{&filterAttackValue,filterAttack},{&filterDecayValue,filterDecay},{&filterSustainValue,filterSustain},{&filterReleaseValue,filterRelease},{&filterAmountValue,filterAmount},
            {&filterTrackingValue,filterKeyTracking},{&filterVelocityValue,filterVelocity},{&filterFmValue,filterFmDepth}})
        {
            auto* value=pair.first;const auto parameter=pair.second;const auto name=hostParameters[static_cast<size_t>(parameter)].name;
            value->setName(name);value->setTitle(name);value->setDescription(juce::String("Drag vertically to change ")+name+". Arrows adjust. Double-click resets.");
            value->onBegin=[this,parameter]{begin({hostIndex(parameter)});};value->onEnd=[this,parameter]{end({hostIndex(parameter)});};
            value->onChange=[this,parameter](double v){set(parameter,v);};addAndMakeVisible(*value);
        }
        for(auto* value:{&filterAttackValue,&filterDecayValue,&filterReleaseValue})value->format=attackValue.format;
        for(auto* value:{&filterAttackValue,&filterDecayValue,&filterReleaseValue})value->unit=ValueUnit::seconds;
        for(auto* value:{&filterSustainValue,&filterTrackingValue})value->unit=ValueUnit::normalized;
        for(auto* value:{&filterAmountValue,&filterVelocityValue,&filterFmValue})value->unit=ValueUnit::octaves;
        filterSustainValue.format=sustainValue.format;filterAmountValue.format=[](double v){return(v>0?"+":"")+juce::String(v,2)+" oct";};
        filterAmountValue.setTooltip("Envelope depth in octaves. Positive opens the filter; negative closes it. Zero preserves the base cutoff.");
        filterTrackingValue.format=sustainValue.format;filterVelocityValue.format=filterAmountValue.format;
        filterFmValue.format=[](double v){return juce::String(v,2)+" oct";};
        filterTrackingValue.setTooltip("Cutoff key tracking around MIDI note 60. 0% is fixed; 100% tracks one octave per octave. Uses the most recent held note.");
        filterVelocityValue.setTooltip("Signed octave offset at full velocity. Positive opens the cutoff for harder notes; negative closes it. Retained through release.");
        filterFmValue.setTooltip("Audio-rate cutoff modulation from the pre-filter oscillator mix. Depth in octaves; the oscillator signal is not control-rate smoothed.");
        filterEnvelope.onBegin=[this]{begin({hostIndex(filterAttack),hostIndex(filterDecay),hostIndex(filterSustain),hostIndex(filterRelease)});};
        filterEnvelope.onEnd=[this]{end({hostIndex(filterAttack),hostIndex(filterDecay),hostIndex(filterSustain),hostIndex(filterRelease)});};
        filterEnvelope.onChange=[this](std::array<double,4> v){group({{filterAttack,v[0]},{filterDecay,v[1]},{filterSustain,v[2]},{filterRelease,v[3]}});};addAndMakeVisible(filterEnvelope);
        poly.setButtonText("POLY");monoButton.setButtonText("MONO");legatoButton.setButtonText("LEGATO");legatoButton.setName("Legato play mode");
        poly.onClick=[this]{playMode(0);};monoButton.onClick=[this]{playMode(1);};legatoButton.onClick=[this]{playMode(2);};
        for(auto* b:{&poly,&monoButton,&legatoButton})addAndMakeVisible(*b);addAndMakeVisible(glidePanel);addAndMakeVisible(autobendPanel);
        envelope.onBegin=[this]{begin({attack,decay,sustain,release});};envelope.onEnd=[this]{end({attack,decay,sustain,release});};
        envelope.onChange=[this](std::array<double,4> v){group({{attack,v[0]},{decay,v[1]},{sustain,v[2]},{release,v[3]}});};addAndMakeVisible(envelope);
        filter.onBegin=[this]{begin({cutoff,resonance});};filter.onEnd=[this]{end({cutoff,resonance});};
        space.onBegin=[this]{begin({spaceMix,spaceSize});};space.onEnd=[this]{end({spaceMix,spaceSize});};
        filter.onChange=[this](float x,float y){group({{cutoff,40*std::pow(450.0,x)},{resonance,y*.85}});};
        space.onChange=[this](float x,float y){group({{spaceMix,x*.65},{spaceSize,y}});};
        for(auto* p:{&filter,&space})addAndMakeVisible(*p);addAndMakeVisible(motion);
        keyboard.setAvailableRange(48,84);keyboard.setKeyPressBaseOctave(4);keyboard.setOctaveForMiddleC(4);keyboard.setMidiChannel(15);
        keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xff2b312b));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,glass());
        keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId,muted());keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,accent().withAlpha(.65f));addAndMakeVisible(keyboard);
        sync();setSize(1200,960);startTimerHz(30);
    }
    ~Content() override{stopTimer();endAudition();session.endAllEdits();session.detachEditor();setLookAndFeel(nullptr);}
    void beginPreview(){hold.setToggleState(true,juce::dontSendNotification);startAudition();}
    bool checkInteractions()
    {
        session.apply(factoryPatches()[0]);session.setCompareReference(session.read());sync();const auto initial=current.values[coupling];
        field.keyPressed(juce::KeyPress(juce::KeyPress::rightKey));const auto changed=current.values[coupling]>initial;undo.onClick();const auto undone=std::abs(current.values[coupling]-initial)<.001;
        fmValue.getAccessibilityHandler()->getValueInterface()->setValue(3.2);const auto source=std::abs(current.values[fm]-3.2)<.001;
        envelope.onChange({.1,.2,.5,.4});const auto env=std::abs(current.values[attack]-.1)<.001;
        filterEnvelope.onBegin();filterEnvelope.onChange({.03,.15,.25,.6});filterEnvelope.onEnd();
        filterAmountValue.getAccessibilityHandler()->getValueInterface()->setValue(-2);
        const auto filterEnv=std::abs(current.values[filterAttack]-.03)<.001&&std::abs(current.values[filterAmount]+2)<.001&&std::abs(current.values[attack]-.1)<.001;
        filter.onChange(.7f,.4f);session.set(couplingLfoOn,1);session.set(couplingLfoDepth,.2);sync();space.onChange(.3f,.7f);const auto support=current.values[cutoff]>1000&&current.values[couplingLfoDepth]>.1&&current.values[spaceMix]>.1;
        filterTrackingValue.getAccessibilityHandler()->getValueInterface()->setValue(1);
        filterVelocityValue.getAccessibilityHandler()->getValueInterface()->setValue(2);
        filterFmValue.getAccessibilityHandler()->getValueInterface()->setValue(3);
        const auto filterMod=std::abs(current.values[filterKeyTracking]-1)<.00001&&std::abs(current.values[filterVelocity]-2)<.00001&&std::abs(current.values[filterFmDepth]-3)<.00001;
        fmDisclosure.setToggleState(true,juce::dontSendNotification);fmDisclosure.onClick();
        session.set(fmRatio,1.414);session.set(fmShape,1);session.set(fmShapeAmount,.8);session.set(fmMix,.25);session.set(fmEnvelopeAmount,2);sync();
        const auto expanded=advancedFm.isShowing()&&std::abs(current.values[fmRatio]-1.414)<.001;
        advancedFm.onClose();
        const auto count=session.presets().size();const auto kept=session.keep("Interaction test").wasOk()&&session.presets().size()==count+1;rebuildPresets();sync();
        const auto saved=current;set(stress,.8);session.setComparing(true);sync();const auto compared=std::abs(current.values[stress]-saved.values[stress])<.001;
        session.setComparing(false);sync();const auto returned=std::abs(current.values[stress]-.8)<.001;
        field.keyPressed(juce::KeyPress(juce::KeyPress::spaceKey));const auto played=!notes.isEmpty();field.keyStateChanged(false);const auto stopped=notes.isEmpty();
        std::cout<<"Native interaction checks: "<<changed<<undone<<source<<env<<filterEnv<<filterMod<<expanded<<support<<kept<<compared<<returned<<played<<stopped<<'\n';
        return changed&&undone&&source&&env&&filterEnv&&filterMod&&expanded&&support&&kept&&compared&&returned&&played&&stopped;
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(chassis());g.setColour(juce::Colour(0xff1b201d));g.fillRect(0,92,286,424);g.fillRect(918,92,282,424);g.setColour(juce::Colour(0xff30372f));
        for(int y:{92,530,726,846})g.drawHorizontalLine(y,24,1176);g.drawVerticalLine(286,110,510);g.drawVerticalLine(918,110,510);g.drawVerticalLine(392,553,710);g.drawVerticalLine(800,553,710);
        g.setColour(ink());g.setFont(juce::Font(juce::FontOptions("Helvetica Neue",36,juce::Font::bold)));g.drawText("nitride",25,23,200,43,juce::Justification::left);
        if(hostPresets())
        {
            text(g,"HOST PRESETS",{310,26,320,14},muted(),8);
            text(g,"Load and save in the host header",{310,46,350,18},muted(),9);
        }
        else text(g,"PATCH",{300,13,120,14},muted(),8);
        section(g,"SOUND",{24,112,238,18});section(g,"NETWORK",{310,112,584,18});section(g,"AMPLITUDE",{942,112,234,18});
        text(g,"COUPLING "+juce::String(current.values[coupling]*100,0)+"%   /   STRESS "+juce::String(current.values[stress]*100,0)+"%",{310,428,584,17},muted(),8);
        sourceWave(g);text(g,"SOURCE / NETWORK FM",{36,190,225,14},muted(),8);
        section(g,"TONE",{24,550,344,18});text(g,"LP / 12 dB",{205,550,163,18},muted(),8,juce::Justification::right);
        section(g,"MOTION",{416,550,360,18});int active=0;for(const auto& lane:lfoParameters)active+=current.values[lane.enabled]>=.5?1:0;
        text(g,juce::String(active)+" / 6 LANES",{580,550,196,18},muted(),8,juce::Justification::right);
        section(g,"SPACE",{824,550,352,18});text(g,"DIFFUSION",{1000,550,176,18},muted(),8,juce::Justification::right);
        text(g,formatCutoff(current.values[cutoff])+"  /  RES "+juce::String(current.values[resonance]*100,0)+"%",{36,692,332,17},muted(),9);
        text(g,"MIX "+juce::String(current.values[spaceMix]*100,0)+"%  /  SIZE "+juce::String(current.values[spaceSize]*100,0)+"%",{836,692,340,17},muted(),9);
        section(g,"FILTER ENVELOPE",{24,742,344,18});text(g,"SHARED > CUTOFF",{836,742,340,18},muted(),8);
        text(g,"SIGNED DEPTH / OCTAVES",{836,810,340,17},muted(),8);
        text(g,"NATIVE INSTRUMENT",{26,928,260,17},muted(),8);text(g,juce::String(live.voices.load())+" VOICES",{600,928,146,17},muted(),8);text(g,"AUDIO PROCESSOR",{250,928,340,17},muted(),8);
        const auto peak=live.peak.load();for(int i=0;i<14;++i){g.setColour(peak>static_cast<float>(i)/20?accent().withAlpha(.8f):juce::Colour(0xff30372f));g.fillRect(790+i*8,933,5,10);}
    }
    void resized() override
    {
        patchBox.setBounds(300,33,330,34);undo.setBounds(668,33,70,34);compare.setBounds(746,33,94,34);keep.setBounds(848,33,70,34);hold.setBounds(968,33,80,34);settings.setBounds(1102,33,74,34);
        if(hostPresets()){undo.setBounds(746,33,70,34);compare.setBounds(824,33,94,34);}
        field.setBounds(310,145,584,274);ribbon.setBounds(310,448,584,82);fmValue.setBounds(36,208,226,28);pitchValue.setBounds(36,242,226,28);
        fmDisclosure.setBounds(192,108,70,26);advancedFm.setBounds(310,145,584,385);
        poly.setBounds(36,280,70,24);monoButton.setBounds(112,280,70,24);legatoButton.setBounds(188,280,74,24);glidePanel.setBounds(36,317,226,85);autobendPanel.setBounds(36,418,226,85);
        envelope.setBounds(942,145,234,147);attackValue.setBounds(942,309,234,34);decayValue.setBounds(942,351,234,34);sustainValue.setBounds(942,393,234,34);releaseValue.setBounds(942,435,234,34);
        filter.setBounds(36,586,152,96);filterTrackingValue.setBounds(196,586,172,28);
        filterVelocityValue.setBounds(196,620,172,28);filterFmValue.setBounds(196,654,172,28);
        motion.setBounds(428,576,348,136);space.setBounds(836,586,340,96);
        filterEnvelope.setBounds(36,767,332,64);filterAttackValue.setBounds(428,767,166,28);filterDecayValue.setBounds(610,767,166,28);
        filterSustainValue.setBounds(428,803,166,28);filterReleaseValue.setBounds(610,803,166,28);filterAmountValue.setBounds(836,767,340,32);
        keyboard.setBounds(26,861,1148,53);keyboard.setKeyWidth(1148.0f/22);outputValue.setBounds(914,923,262,28);
    }
private:
    bool hostPresets()const noexcept{return presetPresentation==PresetPresentation::host;}
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
        const auto revision=session.libraryRevision.load();presetIds.clear();patchBox.clear(juce::dontSendNotification);
        if(!hostPresets())
        {
            juce::String category;
            for(const auto& entry:factoryPrograms)
            {
                if(category!=entry.category){category=entry.category;patchBox.addSectionHeading(category);}
                presetIds.emplace_back(entry.id);patchBox.addItem(entry.name,static_cast<int>(presetIds.size()));
            }
            bool keptHeading=false;
            for(const auto& preset:session.presets())if(!preset.id.startsWith("factory:")&&!preset.id.startsWith("review:"))
            {
                if(!keptHeading){patchBox.addSectionHeading("Kept sounds");keptHeading=true;}
                presetIds.push_back(preset.id);patchBox.addItem(preset.patch.name,static_cast<int>(presetIds.size()));
            }
        }
        lastLibrary=revision;
    }
    void sync()
    {
        const auto revision=session.revision.load(std::memory_order_acquire),epoch=session.restoreEpoch.load();
        if(lastEpoch!=epoch)
        {
            endAudition();hold.setToggleState(false,juce::dontSendNotification);touching=false;edits={};field.cancelGesture();ribbon.cancelGesture();envelope.cancelGesture();filterEnvelope.cancelGesture();
            for(auto* v:{&fmValue,&pitchValue,&attackValue,&decayValue,&sustainValue,&releaseValue,&outputValue,&filterAttackValue,&filterDecayValue,&filterSustainValue,&filterReleaseValue,&filterAmountValue,&filterTrackingValue,&filterVelocityValue,&filterFmValue})v->cancelGesture();for(auto* p:{&filter,&space})p->cancelGesture();lastEpoch=epoch;
        }
        if(lastLibrary!=session.libraryRevision.load())rebuildPresets();current=session.readVisible();glidePanel.sync(current,epoch);autobendPanel.sync(current,epoch);
        field.setPosition(static_cast<float>(current.values[coupling]),static_cast<float>(current.values[stress]));ribbon.setResponse(current.values[response]);
        fmValue.setValue(current.values[fm]);pitchValue.setValue(current.values[pitch]);outputValue.setValue(current.values[output]);
        attackValue.setValue(current.values[attack]);decayValue.setValue(current.values[decay]);sustainValue.setValue(current.values[sustain]);releaseValue.setValue(current.values[release]);envelope.setEnvelope({current.values[attack],current.values[decay],current.values[sustain],current.values[release]});
        filterAttackValue.setValue(current.values[filterAttack]);filterDecayValue.setValue(current.values[filterDecay]);filterSustainValue.setValue(current.values[filterSustain]);filterReleaseValue.setValue(current.values[filterRelease]);filterAmountValue.setValue(current.values[filterAmount]);
        filterEnvelope.setEnvelope({current.values[filterAttack],current.values[filterDecay],current.values[filterSustain],current.values[filterRelease]});
        filter.setPosition(static_cast<float>(std::log(current.values[cutoff]/40)/std::log(450.0)),static_cast<float>(current.values[resonance]/.85));
        filterTrackingValue.setValue(current.values[filterKeyTracking]);filterVelocityValue.setValue(current.values[filterVelocity]);filterFmValue.setValue(current.values[filterFmDepth]);
        motion.sync(current,epoch);space.setPosition(static_cast<float>(current.values[spaceMix]/.65),static_cast<float>(current.values[spaceSize]));
        advancedFm.sync(current,epoch);
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
        juce::PopupMenu menu;if(audioSettings)menu.addItem(1,"Audio / MIDI...");menu.addItem(2,"Reduced motion",true,field.reducedMotion);menu.addItem(3,"About Nitride");menu.addSeparator();menu.addItem(4,"Redo",session.canRedo());
        juce::PopupMenu legacy;legacy.addItem(5,"Load .nitridepreset...");legacy.addItem(6,"Export current .nitridepreset...");legacy.addItem(7,"Refresh kept sounds");
        std::vector<juce::String> legacyIds;
        for(const auto& preset:session.presets())if(!preset.id.startsWith("factory:")&&!preset.id.startsWith("review:"))
        {
            if(legacyIds.empty())legacy.addSectionHeader("Legacy kept sounds — load, then save in host");
            legacyIds.push_back(preset.id);legacy.addItem(2000+static_cast<int>(legacyIds.size())-1,preset.patch.name);
        }
        if(hostPresets())menu.addSubMenu("Legacy preset migration",legacy);
        else
        {
            menu.addItem(5,"Import preset...");menu.addItem(6,"Export preset...");menu.addItem(7,"Refresh kept sounds");
            juce::PopupMenu archive;
            for(size_t i=0;i<builtInPresets().size();++i)if(!factoryProgramName(static_cast<int>(i)))archive.addItem(1000+static_cast<int>(i),builtInPresets()[i].patch.name);
            menu.addSubMenu("Evaluation archive",archive);
        }
        const juce::Component::SafePointer<Content> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings),[safe,legacyIds](int choice)
        {
            if(!safe)return;
            if(choice==1&&safe->audioSettings)safe->audioSettings();
            else if(choice==2){safe->field.reducedMotion=!safe->field.reducedMotion;safe->ribbon.reducedMotion=safe->field.reducedMotion;}
            else if(choice==4){safe->session.redo();safe->sync();}
            else if(choice==5)safe->filePreset(true);
            else if(choice==6)safe->filePreset(false);
            else if(choice==7){safe->session.refreshPresets();safe->rebuildPresets();safe->sync();}
            else if(choice>=2000&&choice-2000<static_cast<int>(legacyIds.size())){safe->session.selectPreset(legacyIds[static_cast<size_t>(choice-2000)]);safe->sync();}
            else if(choice>=1000&&choice-1000<static_cast<int>(builtInPresets().size())){safe->session.selectPreset(builtInPresets()[static_cast<size_t>(choice-1000)].id);safe->sync();}
            else if(choice==3)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Nitride",juce::String("Nitride ")+buildVersion+" / "+evaluationBuild+"\n\n"+(safe->hostPresets()?"Use the host Settings menu to load and save categorized presets. Legacy Nitride sounds remain available in the migration menu.":"Choose a factory category or kept sound. Keep saves to your Nitride library; Compare retains edits."));
        });
    }
    static void section(juce::Graphics& g,const juce::String& label,juce::Rectangle<int> r){g.setColour(ink());g.setFont(juce::Font(juce::FontOptions("Helvetica Neue",11,juce::Font::bold)));g.drawText(label,r,juce::Justification::left);}
    static juce::String formatCutoff(double hz){return hz>=1000?juce::String(hz/1000,2)+" kHz":juce::String(hz,0)+" Hz";}
    void sourceWave(juce::Graphics& g)
    {
        const juce::Rectangle<float> p{36,145,226,40};g.setColour(glass());g.fillRoundedRectangle(p,4);juce::Path wave;
        const auto harmonics=DSP::fmHarmonics(static_cast<DSP::FmShape>(static_cast<int>(current.values[fmShape])),current.values[fmShapeAmount]);
        for(int i=0;i<=140;++i){const auto t=static_cast<float>(i)/140,phase=t*pi*4;const auto sample=static_cast<float>(std::sin(phase+current.values[fm]*DSP::fmWave(phase*current.values[fmRatio],harmonics)));const auto x=p.getX()+12+t*(p.getWidth()-24),y=p.getCentreY()-sample*12;if(i==0)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}g.setColour(signal().withAlpha(.7f));g.strokePath(wave,juce::PathStrokeType(1.1f));
    }
    void timerCallback() override
    {
        if(editorGeneration!=session.editorMidi.epoch()){editorGeneration=session.editorMidi.epoch();session.synchronizeEditorInput();notes.clear();hold.setToggleState(false,juce::dontSendNotification);touching=false;}
        if(lastRevision!=session.revision.load(std::memory_order_acquire)||lastLibrary!=session.libraryRevision.load())sync();keyboard.repaint();field.repaint();ribbon.repaint();motion.repaint();repaint(24,923,1152,34);
    }
    juce::LookAndFeel_V4 look;InstrumentSession& session;LiveState& live;std::function<void()> audioSettings;const PresetPresentation presetPresentation;
    PitchMotionPanel glidePanel,autobendPanel;PlayingField field;ResponseRibbon ribbon;InstrumentKeyboard keyboard;
    ValueControl fmValue,pitchValue,attackValue,decayValue,sustainValue,releaseValue,outputValue;
    ValueControl filterAttackValue,filterDecayValue,filterSustainValue,filterReleaseValue,filterAmountValue;
    EnvelopeControl filterEnvelope,envelope;ValueControl filterTrackingValue,filterVelocityValue,filterFmValue;
    XYControl filter;MotionPanel motion;XYControl space;FmPanel advancedFm;
    juce::ComboBox patchBox;juce::TextButton undo,compare,keep,hold,settings,poly,monoButton,legatoButton,fmDisclosure;
    std::vector<juce::String> presetIds;std::unique_ptr<juce::FileChooser> chooser;Patch current;juce::Array<int> notes;
    std::array<bool,hostParameterCount> edits{};std::uint64_t lastRevision=0,lastLibrary=0,lastEpoch=0,editorGeneration=0;bool touching=false;
};
InstrumentPanel::InstrumentPanel(InstrumentSession& s,std::function<void()> settings,PresetPresentation presentation):content(std::make_unique<Content>(s,std::move(settings),presentation)){addAndMakeVisible(*content);setSize(1200,960);}
InstrumentPanel::~InstrumentPanel()=default;
void InstrumentPanel::resized(){content->setBounds(getLocalBounds());}
void InstrumentPanel::beginPreview(){content->beginPreview();}
bool InstrumentPanel::checkInteractions(){return content->checkInteractions();}
}
