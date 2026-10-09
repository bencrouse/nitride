#include "InstrumentSession.h"
#include "PresetStore.h"
#include <limits>

namespace Nitride
{
namespace
{
juce::String factoryId(int index){return "factory:"+juce::String(index);}
}

InstrumentSession::InstrumentSession(juce::File directory)
    : store(std::make_unique<PresetStore>(directory==juce::File{}?PresetStore::defaultDirectory():directory))
{
    currentName=factoryPatches()[0].name;selectedId=factoryId(0);reference=factoryPatches()[0];
    storeConditions(reference);userPresets=store->loadAll();
}
InstrumentSession::~InstrumentSession()=default;

double InstrumentSession::clamp(Parameter p,double value) noexcept
{
    return clampParameter(p,value);
}

InstrumentSession::Conditions InstrumentSession::readConditions() const
{
    Conditions result{};
    // Independently automated controls are published atomically. A snapshot reads
    // the latest value of each; neither host listeners nor readers wait for a writer.
    for(size_t i=0;i<parameterCount;++i)result.values[i]=values[i].load(std::memory_order_relaxed);
    result.mono=mono.load();result.chord=chord.load();return result;
}
void InstrumentSession::storeConditions(const Patch& patch) noexcept
{
    for(size_t i=0;i<parameterCount;++i)values[i].store(clamp(static_cast<Parameter>(i),patch.values[i]),std::memory_order_relaxed);
    mono.store(patch.mono);chord.store(patch.chord);
}
void InstrumentSession::closeCompare() noexcept
{
    if(comparing.exchange(false))revision.fetch_add(1,std::memory_order_release);
}
void InstrumentSession::storeFromHost(int index,double value) noexcept
{
    closeCompare();
    if(index<0||index>=hostParameterCount||!std::isfinite(value))return;
    if(index!=monoHostIndex)
    {
        const auto p=conditionIndex(index);value=clamp(p,value);
        const auto previous=values[static_cast<size_t>(p)].load();
        // UI/factory conditions retain their precise double representation when a
        // float host notification merely echoes the same value. Real automation
        // changes replace it. This protects the approved nonlinear render references.
        const auto tolerance=4*std::numeric_limits<float>::epsilon()*std::max(1.0,std::abs(value));
        if(std::abs(previous-value)<=tolerance)return;
        values[static_cast<size_t>(p)].store(value);
    }
    else mono.store(value>=.5);
    revision.fetch_add(1,std::memory_order_release);
}
void InstrumentSession::setInternal(int index,double value,bool notify)
{
    closeCompare();
    {
        if(index!=monoHostIndex){const auto p=conditionIndex(index);value=clamp(p,value);values[static_cast<size_t>(p)].store(value);}
        else mono.store(value>=.5);
    }
    if(notify&&writeHostParameter)writeHostParameter(index,value);
    revision.fetch_add(1,std::memory_order_release);
}
void InstrumentSession::set(Parameter p,double value)
{
    const auto index=hostIndex(p);bool single=false;
    {const juce::ScopedLock lock(documentLock);single=gestures[static_cast<size_t>(index)]==0;}
    if(single)beginEdit({index});
    {const juce::ScopedLock lock(documentLock);if(editStart)editStart->parameters[static_cast<size_t>(index)]=true;}
    setInternal(index,value,true);if(single)endEdit({index});
}
void InstrumentSession::setMono(bool value)
{
    beginEdit({monoHostIndex});
    {const juce::ScopedLock lock(documentLock);if(editStart)editStart->parameters[monoHostIndex]=true;}
    setInternal(monoHostIndex,value?1:0,true);endEdit({monoHostIndex});
}

void InstrumentSession::syncProgram() const
{
    if(const auto index=pendingProgram.exchange(-1);index>=0)
    {
        for(int i=0;i<hostParameterCount;++i)if(gestures[static_cast<size_t>(i)]>0)
        {
            gestures[static_cast<size_t>(i)]=0;if(endHostGesture)endHostGesture(i);
        }
        editDepth=0;editStart.reset();
        currentName=factoryPatches()[static_cast<size_t>(index)].name;selectedId=factoryId(index);
        reference=factoryPatches()[static_cast<size_t>(index)];history.clear();future.clear();
    }
}
Patch InstrumentSession::read() const
{
    const juce::ScopedLock lock(documentLock);syncProgram();const auto conditions=readConditions();
    return{currentName,conditions.values,conditions.mono,conditions.chord};
}
Patch InstrumentSession::readVisible() const
{
    const juce::ScopedLock lock(documentLock);syncProgram();return comparing.load()?reference:read();
}
InstrumentSession::Snapshot InstrumentSession::snapshot() const
{
    return{read(),reference,selectedId,program.load()};
}
bool InstrumentSession::same(const Patch& a,const Patch& b)
{
    if(a.name!=b.name||a.mono!=b.mono||a.chord!=b.chord)return false;
    for(size_t i=0;i<parameterCount;++i)if(std::abs(a.values[i]-b.values[i])>1.0e-12)return false;
    return true;
}
void InstrumentSession::beginEdit(std::initializer_list<int> parameters)
{
    const juce::ScopedLock lock(documentLock);syncProgram();closeCompare();
    if(editDepth++==0){editStart=snapshot();editStart->wholePatch=false;}
    for(const auto index:parameters)
        if(index>=0&&index<hostParameterCount&&gestures[static_cast<size_t>(index)]++==0&&beginHostGesture)beginHostGesture(index);
}
void InstrumentSession::endEdit(std::initializer_list<int> parameters)
{
    const juce::ScopedLock lock(documentLock);
    for(const auto index:parameters)
        if(index>=0&&index<hostParameterCount&&gestures[static_cast<size_t>(index)]>0
            &&--gestures[static_cast<size_t>(index)]==0&&endHostGesture)endHostGesture(index);
    if(editDepth>0&&--editDepth==0)
    {
        finishEdit();
    }
}
void InstrumentSession::endAllEdits()
{
    const juce::ScopedLock lock(documentLock);
    for(int i=0;i<hostParameterCount;++i)if(gestures[static_cast<size_t>(i)]>0){gestures[static_cast<size_t>(i)]=0;if(endHostGesture)endHostGesture(i);}
    editDepth=0;finishEdit();
}
bool InstrumentSession::editChanged() const
{
    if(!editStart)return false;
    const auto current=readConditions();
    for(size_t i=0;i<parameterCount;++i)
        if(editStart->parameters[static_cast<size_t>(hostIndex(static_cast<Parameter>(i)))]&&std::abs(editStart->patch.values[i]-current.values[i])>1.0e-12)return true;
    return editStart->parameters[monoHostIndex]&&editStart->patch.mono!=current.mono;
}
void InstrumentSession::finishEdit()
{
    if(editChanged()){history.push_back(*editStart);if(history.size()>40)history.erase(history.begin());future.clear();}
    editStart.reset();revision.fetch_add(1,std::memory_order_release);
}
void InstrumentSession::writePatch(const Patch& patch,bool notify)
{
    closeCompare();storeConditions(patch);
    if(notify&&writeHostParameter)
        for(int i=0;i<hostParameterCount;++i)
        {
            const auto own=gestures[static_cast<size_t>(i)]==0;
            if(own&&beginHostGesture)beginHostGesture(i);
            writeHostParameter(i,i!=monoHostIndex?patch.values[static_cast<size_t>(conditionIndex(i))]:(patch.mono?1:0));
            if(own&&endHostGesture)endHostGesture(i);
        }
    revision.fetch_add(1,std::memory_order_release);
}
void InstrumentSession::apply(const Patch& patch)
{
    const juce::ScopedLock lock(documentLock);syncProgram();const auto before=snapshot();
    writePatch(patch,true);currentName=patch.name;
    if(!same(before.patch,read())){history.push_back(before);if(history.size()>40)history.erase(history.begin());future.clear();}
}
void InstrumentSession::applyProgram(int index)
{
    if(index<0||index>=static_cast<int>(factoryPatches().size()))return;
    program.store(index);
    // Host program selection may arrive on a render thread: no document/file lock.
    closeCompare();storeConditions(factoryPatches()[static_cast<size_t>(index)]);
    if(writeHostParameter)for(int i=0;i<hostParameterCount;++i)writeHostParameter(i,i!=monoHostIndex
        ?factoryPatches()[static_cast<size_t>(index)].values[static_cast<size_t>(conditionIndex(i))]:(factoryPatches()[static_cast<size_t>(index)].mono?1:0));
    pendingProgram.store(index);restoreEpoch.fetch_add(1);
    revision.fetch_add(1,std::memory_order_release);
}
bool InstrumentSession::canUndo() const{const juce::ScopedLock lock(documentLock);syncProgram();return!history.empty();}
bool InstrumentSession::canRedo() const{const juce::ScopedLock lock(documentLock);syncProgram();return!future.empty();}
void InstrumentSession::restoreSnapshot(const Snapshot& s)
{
    if(s.wholePatch)
    {
        pendingProgram.store(-1);program.store(s.program);currentName=s.patch.name;selectedId=s.selected;reference=s.reference;writePatch(s.patch,true);
        return;
    }
    for(int i=0;i<hostParameterCount;++i)if(s.parameters[static_cast<size_t>(i)])
    {
        if(beginHostGesture)beginHostGesture(i);
        setInternal(i,i!=monoHostIndex?s.patch.values[static_cast<size_t>(conditionIndex(i))]:(s.patch.mono?1:0),true);
        if(endHostGesture)endHostGesture(i);
    }
}
void InstrumentSession::undo()
{
    endAllEdits();const juce::ScopedLock lock(documentLock);syncProgram();if(history.empty())return;
    const auto s=history.back();history.pop_back();auto inverse=snapshot();inverse.wholePatch=s.wholePatch;inverse.parameters=s.parameters;
    future.push_back(inverse);restoreSnapshot(s);
}
void InstrumentSession::redo()
{
    endAllEdits();const juce::ScopedLock lock(documentLock);syncProgram();if(future.empty())return;
    const auto s=future.back();future.pop_back();auto inverse=snapshot();inverse.wholePatch=s.wholePatch;inverse.parameters=s.parameters;
    history.push_back(inverse);restoreSnapshot(s);
}
void InstrumentSession::setComparing(bool enabled)
{
    const juce::ScopedLock lock(documentLock);syncProgram();
    if(enabled){for(size_t i=0;i<parameterCount;++i)compareValues[i].store(reference.values[i]);compareMono.store(reference.mono);comparing.store(true);}
    else comparing.store(false);
    revision.fetch_add(1,std::memory_order_release);
}
Patch InstrumentSession::compareReference()const{const juce::ScopedLock lock(documentLock);syncProgram();return reference;}
void InstrumentSession::setCompareReference(const Patch& p){const juce::ScopedLock lock(documentLock);reference=p;revision.fetch_add(1,std::memory_order_release);}

std::vector<StoredPreset> InstrumentSession::presets() const
{
    const juce::ScopedLock lock(documentLock);std::vector<StoredPreset> result;
    for(int i=0;i<4;++i)result.push_back({factoryId(i),factoryPatches()[static_cast<size_t>(i)]});
    result.insert(result.end(),userPresets.begin(),userPresets.end());
    for(const auto& portable:portablePresets)
        if(std::none_of(result.begin(),result.end(),[&](const auto& p){return p.id==portable.id;}))result.push_back(portable);
    return result;
}
juce::String InstrumentSession::selectedPresetId()const{const juce::ScopedLock lock(documentLock);syncProgram();return selectedId;}
void InstrumentSession::rememberPortable(const StoredPreset& preset)
{
    const auto found=std::find_if(portablePresets.begin(),portablePresets.end(),[&](const auto& p){return p.id==preset.id;});
    if(found!=portablePresets.end())portablePresets.erase(found);
    portablePresets.push_back(preset);
    if(portablePresets.size()>256)portablePresets.erase(portablePresets.begin());
}
bool InstrumentSession::selectPreset(const juce::String& id)
{
    const juce::ScopedLock lock(documentLock);syncProgram();
    const auto list=presets();const auto found=std::find_if(list.begin(),list.end(),[&](const auto& p){return p.id==id;});
    if(found==list.end())return false;
    const auto before=snapshot();writePatch(found->patch,true);currentName=found->patch.name;selectedId=id;reference=found->patch;
    if(id.startsWith("factory:"))program.store(juce::jlimit(0,3,id.fromFirstOccurrenceOf(":",false,false).getIntValue()));
    else rememberPortable(*found);
    history.push_back(before);if(history.size()>40)history.erase(history.begin());future.clear();revision.fetch_add(1,std::memory_order_release);return true;
}
juce::Result InstrumentSession::keep(const juce::String& name)
{
    return keepPatch(name,read());
}
juce::Result InstrumentSession::keepPatch(const juce::String& name,const Patch& conditions)
{
    if(name.trim().isEmpty()||name.length()>256)return juce::Result::fail("Enter a sound name (up to 256 characters).");
    StoredPreset preset{juce::Uuid().toString(),conditions};preset.patch.name=name.trim();
    const auto result=store->save(preset);if(result.failed())return result;
    {const juce::ScopedLock lock(documentLock);userPresets.push_back(preset);libraryRevision.fetch_add(1);}
    selectPreset(preset.id);return juce::Result::ok();
}
juce::Result InstrumentSession::importPreset(const juce::File& file)
{
    auto preset=PresetStore::importFile(file);if(!preset)return juce::Result::fail("This is not a supported Nitride preset.");
    preset->id=juce::Uuid().toString();const auto result=store->save(*preset);if(result.failed())return result;
    {const juce::ScopedLock lock(documentLock);userPresets.push_back(*preset);libraryRevision.fetch_add(1);}
    selectPreset(preset->id);return juce::Result::ok();
}
juce::Result InstrumentSession::exportPreset(const juce::File& file)const{return PresetStore::exportFile(file,{juce::Uuid().toString(),read()});}
const juce::File& InstrumentSession::presetDirectory()const{return store->directory();}
void InstrumentSession::refreshPresets(){auto loaded=store->loadAll();const juce::ScopedLock lock(documentLock);userPresets=std::move(loaded);libraryRevision.fetch_add(1);}

juce::var InstrumentSession::encodeSnapshot(const Snapshot& s)
{
    auto* object=new juce::DynamicObject();object->setProperty("patch",PresetStore::encode({"snapshot",s.patch}));
    object->setProperty("reference",PresetStore::encode({"reference",s.reference}));object->setProperty("selected",s.selected);object->setProperty("program",s.program);
    int mask=0;for(int i=0;i<hostParameterCount;++i)if(s.parameters[static_cast<size_t>(i)])mask|=1<<i;
    object->setProperty("wholePatch",s.wholePatch);object->setProperty("parameters",mask);return juce::var(object);
}
std::optional<InstrumentSession::Snapshot> InstrumentSession::decodeSnapshot(const juce::var& data)
{
    const auto patch=PresetStore::decode(data["patch"]),referencePatch=PresetStore::decode(data["reference"]);
    if(!patch||!referencePatch||!data["program"].isInt())return std::nullopt;
    const auto p=static_cast<int>(data["program"]);if(p<0||p>3)return std::nullopt;
    Snapshot result{patch->patch,referencePatch->patch,data["selected"].toString(),p};
    if(data.hasProperty("wholePatch")||data.hasProperty("parameters"))
    {
        if(!data["wholePatch"].isBool()||!data["parameters"].isInt())return std::nullopt;
        const auto mask=static_cast<int>(data["parameters"]);if(mask<0||mask>=(1<<hostParameterCount))return std::nullopt;
        result.wholePatch=static_cast<bool>(data["wholePatch"]);
        for(int i=0;i<hostParameterCount;++i)result.parameters[static_cast<size_t>(i)]=(mask&(1<<i))!=0;
    }
    return result;
}
juce::var InstrumentSession::saveDocument()const
{
    const juce::ScopedLock lock(documentLock);syncProgram();auto* object=new juce::DynamicObject();
    object->setProperty("format","nitride-session");object->setProperty("version",3);object->setProperty("current",encodeSnapshot(snapshot()));
    juce::Array<juce::var> undo,redo,kept;
    for(const auto& s:history)undo.add(encodeSnapshot(s));
    if(editChanged())
    {
        if(undo.size()==40)undo.remove(0);
        undo.add(encodeSnapshot(*editStart));
    }
    else for(const auto& s:future)redo.add(encodeSnapshot(s));
    for(const auto& p:portablePresets)kept.add(PresetStore::encode(p));
    object->setProperty("undo",undo);object->setProperty("redo",redo);object->setProperty("kept",kept);return juce::var(object);
}
juce::Result InstrumentSession::restoreDocument(const juce::var& data)
{
    const auto version=static_cast<int>(data["version"]);
    if(data["format"].toString()!="nitride-session"||(version!=2&&version!=3))return juce::Result::fail("Unsupported session state.");
    const auto current=decodeSnapshot(data["current"]);if(!current)return juce::Result::fail("Invalid sound conditions.");
    std::vector<Snapshot> newHistory,newFuture;std::vector<StoredPreset> newKept;
    for(const auto field:{"undo","redo"})
    {
        const auto* array=data[field].getArray();if(array==nullptr||array->size()>40)return juce::Result::fail("Invalid history.");
        for(const auto& item:*array){auto s=decodeSnapshot(item);if(!s)return juce::Result::fail("Invalid history sound.");(juce::String(field)=="undo"?newHistory:newFuture).push_back(*s);}
    }
    const auto* kept=data["kept"].getArray();if(kept==nullptr||kept->size()>256)return juce::Result::fail("Invalid kept sounds.");
    for(const auto& item:*kept){auto p=PresetStore::decode(item);if(!p)return juce::Result::fail("Invalid kept sound.");newKept.push_back(*p);}
    endAllEdits();const juce::ScopedLock lock(documentLock);closeCompare();
    pendingProgram.store(-1);program.store(current->program);currentName=current->patch.name;selectedId=current->selected;reference=current->reference;
    storeConditions(current->patch);history=std::move(newHistory);future=std::move(newFuture);portablePresets=std::move(newKept);
    restoreEpoch.fetch_add(1);libraryRevision.fetch_add(1);revision.fetch_add(1,std::memory_order_release);return juce::Result::ok();
}
void InstrumentSession::establishRestoredPatch(const Patch& patch,int index)
{
    endAllEdits();const juce::ScopedLock lock(documentLock);closeCompare();pendingProgram.store(-1);program.store(index);
    currentName=patch.name;selectedId=factoryId(index);reference=patch;storeConditions(patch);history.clear();future.clear();portablePresets.clear();
    restoreEpoch.fetch_add(1);revision.fetch_add(1,std::memory_order_release);
}
}
