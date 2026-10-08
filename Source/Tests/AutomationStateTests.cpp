#include "PluginEditor.h"
#include "Instrument/PresetStore.h"
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool sameSound(const Nitride::Patch& a,const Nitride::Patch& b)
{
    if(a.mono!=b.mono||a.chord!=b.chord)return false;
    for(size_t i=0;i<Nitride::parameterCount;++i)if(std::abs(a.values[i]-b.values[i])>.00001*std::max(1.0,std::abs(a.values[i])))return false;
    return true;
}
struct TestDirectory
{
    TestDirectory():folder(juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("nitride-session-tests","",false))
    {require(folder.createDirectory().wasOk(),"Cannot create temporary preset directory");}
    ~TestDirectory(){folder.deleteRecursively();}
    juce::File folder;
};
struct Listener final:juce::AudioProcessorParameter::Listener
{
    void parameterValueChanged(int,float)override{++changes;}
    void parameterGestureChanged(int,bool start)override{if(start)++begins;else ++ends;}
    int changes=0,begins=0,ends=0;
};
template<typename T>T* child(juce::Component& parent,const juce::String& name)
{
    for(auto* c:parent.getChildren())
    {if(auto* found=dynamic_cast<T*>(c);found&&c->getName()==name)return found;if(auto* found=child<T>(*c,name))return found;}
    return nullptr;
}
template<typename Predicate>void flushMessages(Predicate ready)
{
    // Parameter callbacks are synchronous; the UI polls the atomic model on its
    // message-thread timer. This exercises actual host -> displayed-control sync.
    for(int i=0;i<100&&!ready();++i)juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
}
juce::MouseEvent pointerEvent(juce::Component& component,juce::Point<float> point,juce::Point<float> start)
{
    const auto now=juce::Time::getCurrentTime();
    return{juce::Desktop::getInstance().getMainMouseSource(),point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,&component,&component,now,start,now,1,true};
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    try
    {
        TestDirectory directory;
        NitrideAudioProcessor processor(directory.folder);
        auto& session=processor.getInstrumentSession();auto& parameters=processor.getState();
        require(processor.getParameters().size()==Nitride::hostParameterCount,"Host automation contract is incomplete");
        for(const auto& d:Nitride::hostParameters)require(parameters.getParameter(d.id)!=nullptr,"Stable host parameter ID is missing");
        require(parameters.getParameter("mono")!=nullptr,"Mono automation parameter is missing");

        // Undo owns UI edits, not unrelated host automation, including changes
        // arriving while the gesture is open and after history is serialized.
        NitrideAudioProcessor scoped(directory.folder);
        auto& scopedSession=scoped.getInstrumentSession();auto& scopedParameters=scoped.getState();
        const auto automate=[&](const char* id,float value){auto* p=scopedParameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};
        scopedSession.beginEdit({Nitride::cutoff});automate("fm_depth",2.2f);scopedSession.endEdit({Nitride::cutoff});
        require(!scopedSession.canUndo(),"Unchanged UI gesture captured host automation as an undo edit");
        scopedSession.beginEdit({Nitride::fm});scopedSession.set(Nitride::fm,3.2);automate("cutoff",900);scopedSession.endEdit({Nitride::fm});
        automate("output",-12);scopedSession.undo();
        require(std::abs(scopedSession.read().values[Nitride::fm]-2.2)<.001,"Scoped undo did not restore its own parameter");
        require(std::abs(scopedSession.read().values[Nitride::cutoff]-900)<.001&&std::abs(scopedSession.read().values[Nitride::output]+12)<.001,"Undo reverted unrelated host automation");
        automate("cutoff",700);scopedSession.redo();
        require(std::abs(scopedSession.read().values[Nitride::fm]-3.2)<.001&&std::abs(scopedSession.read().values[Nitride::cutoff]-700)<.001,"Redo reverted unrelated host automation");
        juce::MemoryBlock scopedState;scoped.getStateInformation(scopedState);
        NitrideAudioProcessor scopedRestored(directory.folder);scopedRestored.setStateInformation(scopedState.getData(),static_cast<int>(scopedState.getSize()));
        auto* restoredCutoff=scopedRestored.getState().getParameter("cutoff");restoredCutoff->setValueNotifyingHost(restoredCutoff->convertTo0to1(400));
        scopedRestored.getInstrumentSession().undo();
        require(std::abs(scopedRestored.getInstrumentSession().read().values[Nitride::cutoff]-400)<.001,"Serialized undo lost parameter ownership");
        scopedSession.beginEdit({Nitride::response});const auto beforeActiveSave=scopedSession.read();scopedSession.set(Nitride::response,.6);
        scoped.getStateInformation(scopedState);scopedRestored.setStateInformation(scopedState.getData(),static_cast<int>(scopedState.getSize()));
        scopedRestored.getInstrumentSession().undo();
        require(sameSound(scopedRestored.getInstrumentSession().read(),beforeActiveSave),"Saving during an active gesture lost its undo baseline");
        scopedSession.endEdit({Nitride::response});

        Listener couplingListener,stressListener;
        parameters.getParameter("coupling")->addListener(&couplingListener);parameters.getParameter("stress")->addListener(&stressListener);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());editor->addToDesktop(0);editor->setVisible(true);
        auto* field=child<juce::Component>(*editor,"Coupling and Stress playing field");require(field!=nullptr,"Approved field missing");
        const auto old=session.read();field->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
        require(couplingListener.begins==1&&couplingListener.ends==1&&stressListener.begins==1&&stressListener.ends==1,"Field automation gestures are not balanced");
        require(session.read().values[Nitride::coupling]>old.values[Nitride::coupling],"Field did not update its host parameter");
        session.undo();require(sameSound(session.read(),old),"Field undo failed to restore its complete conditions");
        session.redo();require(session.read().values[Nitride::coupling]>old.values[Nitride::coupling],"Redo lost the field gesture");

        auto* response=child<juce::Component>(*editor,"Response timing gesture");require(response!=nullptr,"Response gesture missing");
        Listener responseListener;parameters.getParameter("response")->addListener(&responseListener);
        response->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
        require(responseListener.begins==1&&responseListener.ends==1,"Response automation gesture is not balanced");

        session.beginEdit({Nitride::fm});session.set(Nitride::fm,3.2);session.set(Nitride::fm,3.4);session.endEdit({Nitride::fm});
        const auto edited=session.read();session.setComparing(true);
        require(std::abs(session.audioValue(Nitride::fm)-session.compareReference().values[Nitride::fm])<.001,"Compare did not audition the reference");
        require(sameSound(session.read(),edited),"Compare destroyed edited conditions");
        require(std::abs(parameters.getRawParameterValue("fm_depth")->load()-3.4f)<.001f,"Compare overwrote host automation values");

        juce::MemoryBlock saved;processor.getStateInformation(saved);
        NitrideAudioProcessor restored(directory.folder);restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
        require(!restored.getInstrumentSession().isComparing()&&sameSound(restored.getInstrumentSession().read(),edited),"Saving during Compare did not restore the edited sound");
        require(sameSound(restored.getInstrumentSession().compareReference(),session.compareReference()),"Session lost the compare reference");
        require(restored.getInstrumentSession().canUndo(),"Session lost undo history");
        restored.getInstrumentSession().undo();restored.getInstrumentSession().redo();require(sameSound(restored.getInstrumentSession().read(),edited),"Saved history failed undo/redo");

        auto* hostFm=parameters.getParameter("fm_depth");hostFm->setValueNotifyingHost(hostFm->convertTo0to1(1.1f));
        require(!session.isComparing()&&std::abs(session.read().values[Nitride::fm]-1.1)<.001,"Automation during Compare was overwritten");
        auto* depth=child<juce::Component>(*editor,"FM depth");require(depth!=nullptr,"FM depth control missing");
        flushMessages([&]{return std::abs(depth->getAccessibilityHandler()->getValueInterface()->getCurrentValue()-1.1)<.001;});
        const auto displayed=depth->getAccessibilityHandler()->getValueInterface()->getCurrentValue();
        if(std::abs(displayed-1.1)>=.001)std::cerr<<"Displayed FM "<<displayed<<", model FM "<<session.readVisible().values[Nitride::fm]<<", revision "<<session.revision.load()<<'\n';
        require(std::abs(displayed-1.1)<.001,"Host automation did not update the visible source control");

        // Restoring with an open editor cancels stale pointer/timer state, before
        // the next UI polling tick as well as after it.
        processor.getStateInformation(saved);const auto beforeRestore=session.read();
        for(auto* control:{field,response,depth,child<juce::Component>(*editor,"Amplitude envelope"),child<juce::Component>(*editor,"Filter cutoff and resonance")})
        {
            require(control!=nullptr,"Restore test control missing");
            const juce::Point<float> origin{static_cast<float>(control->getWidth())*.4f,static_cast<float>(control->getHeight())*.5f};
            control->mouseDown(pointerEvent(*control,origin,origin));
            processor.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
            const auto moved=origin+juce::Point<float>{40,-15};control->mouseDrag(pointerEvent(*control,moved,origin));control->mouseUp(pointerEvent(*control,moved,origin));
            juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
            require(sameSound(session.read(),beforeRestore),"Old pointer gesture overwrote recalled sound");
        }
        const juce::Point<float> ribbonPoint{80,35};response->mouseDown(pointerEvent(*response,ribbonPoint,ribbonPoint));
        processor.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
        juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
        require(sameSound(session.read(),beforeRestore),"Response timer overwrote recalled sound");
        response->mouseUp(pointerEvent(*response,ribbonPoint,ribbonPoint));
        processor.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
        field->mouseDown(pointerEvent(*field,{120,100},{120,100}));field->mouseUp(pointerEvent(*field,{120,100},{120,100}));
        require(couplingListener.begins==couplingListener.ends&&stressListener.begins==stressListener.ends,"New gesture before the restore polling tick lost its end notification");
        field->mouseDown(pointerEvent(*field,{100,100},{100,100}));processor.setCurrentProgram(2);
        flushMessages([&]{return couplingListener.begins==couplingListener.ends&&stressListener.begins==stressListener.ends;});
        field->mouseDrag(pointerEvent(*field,{300,150},{100,100}));field->mouseUp(pointerEvent(*field,{300,150},{100,100}));
        require(sameSound(session.read(),Nitride::factoryPatches()[2])&&!session.canUndo(),"Host program change retained stale edit state");

        // Closing during a pointer gesture ends host automation and preserves the edit.
        const auto now=juce::Time::getCurrentTime();const auto point=juce::Point<float>(static_cast<float>(field->getWidth())*.6f,static_cast<float>(field->getHeight())*.5f);
        const juce::MouseEvent press(juce::Desktop::getInstance().getMainMouseSource(),point,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,field,field,now,point,now,1,false);
        field->mouseDown(press);editor.reset();
        require(couplingListener.begins==couplingListener.ends&&stressListener.begins==stressListener.ends,"Editor close left an open host gesture");
        parameters.getParameter("response")->removeListener(&responseListener);
        parameters.getParameter("coupling")->removeListener(&couplingListener);parameters.getParameter("stress")->removeListener(&stressListener);

        // Closing a grouped control commits exactly one complete undo operation.
        NitrideAudioProcessor grouped(directory.folder);Listener amplitudeListener;
        auto* attackParameter=grouped.getState().getParameter("amp_attack");attackParameter->addListener(&amplitudeListener);
        std::unique_ptr<juce::AudioProcessorEditor> groupedEditor(grouped.createEditor());groupedEditor->addToDesktop(0);groupedEditor->setVisible(true);
        auto* envelope=child<juce::Component>(*groupedEditor,"Amplitude envelope");require(envelope!=nullptr,"Amplitude graph missing");
        const auto beforeEnvelope=grouped.getInstrumentSession().read();
        envelope->mouseDown(pointerEvent(*envelope,{70,20},{70,20}));envelope->mouseDrag(pointerEvent(*envelope,{100,20},{70,20}));groupedEditor.reset();
        require(amplitudeListener.begins==1&&amplitudeListener.ends==1,"Grouped control close left an unbalanced host gesture");
        require(!sameSound(grouped.getInstrumentSession().read(),beforeEnvelope),"Grouped-control test did not edit the sound");
        grouped.getInstrumentSession().undo();require(sameSound(grouped.getInstrumentSession().read(),beforeEnvelope)&&!grouped.getInstrumentSession().canUndo(),"Grouped control close split or lost its undo edit");
        attackParameter->removeListener(&amplitudeListener);

        require(session.keep("Recorded material").wasOk(),"Keep could not persist a named preset");
        const auto kept=session.read();const auto keptId=session.selectedPresetId();
        require(kept.name=="Recorded material"&&directory.folder.findChildFiles(juce::File::findFiles,false,"*.nitridepreset").size()==1,"Keep did not write a real preset");
        NitrideAudioProcessor another(directory.folder);require(another.getInstrumentSession().selectPreset(keptId),"A new instance could not find the kept sound");
        require(sameSound(another.getInstrumentSession().read(),kept),"Preset reload changed the sound");
        const auto exportFile=directory.folder.getChildFile("export.nitridepreset");require(session.exportPreset(exportFile).wasOk(),"Preset export failed");
        require(another.getInstrumentSession().importPreset(exportFile).wasOk()&&sameSound(another.getInstrumentSession().read(),kept),"Preset import changed the sound");

        processor.getStateInformation(saved);
        TestDirectory emptyLibrary;NitrideAudioProcessor portable(emptyLibrary.folder);portable.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
        require(portable.getInstrumentSession().selectedPresetId()==keptId&&portable.getInstrumentSession().selectPreset(keptId),"Session cannot recall a kept sound without its external preset file");
        require(sameSound(portable.getInstrumentSession().read(),kept),"Embedded kept preset changed conditions");
        std::unique_ptr<juce::AudioProcessorEditor> reopened(portable.createEditor());reopened->addToDesktop(0);reopened->setVisible(true);
        require(child<juce::ComboBox>(*reopened,"Starting patch")->getText()=="Recorded material","Editor reopening lost the custom preset name");reopened.reset();

        // The v1 numeric conditions from milestone 1 remain loadable.
        juce::XmlElement v1("NITRIDE_INSTRUMENT");v1.setAttribute("version",1);v1.setAttribute("program",2);v1.setAttribute("fm",2.125);v1.setAttribute("motionRate",.8);
        juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(v1,legacy);portable.setStateInformation(legacy.getData(),static_cast<int>(legacy.getSize()));
        require(portable.getCurrentProgram()==2&&std::abs(portable.getInstrumentSession().read().values[Nitride::fm]-2.125)<.001,"Milestone-1 migration failed");
        const auto beforeInvalid=portable.getInstrumentSession().read();const std::array<char,4> garbage{'n','o','p','e'};
        portable.setStateInformation(garbage.data(),4);require(sameSound(portable.getInstrumentSession().read(),beforeInvalid),"Malformed state damaged the current sound");
        auto malformed=Nitride::PresetStore::encode({juce::Uuid().toString(),kept});malformed.getDynamicObject()->setProperty("version",99);
        require(!Nitride::PresetStore::decode(malformed),"Unsupported preset version was accepted");
        std::cout<<"Milestone 2 passed: stable host parameters, gestures, automation/UI sync, compare-safe state, history, persistent/portable presets, migration and malformed-state rejection.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
