#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

NoxWaveProcessor::NoxWaveProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NoxWaveState", nox::createParameterLayout())
{
    engine.bind (apvts);
    startTimer (2000);
}

NoxWaveProcessor::~NoxWaveProcessor()
{
    stopTimer();
}

bool NoxWaveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NoxWaveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    keyboardState.reset();
}

void NoxWaveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    nox::HostInfo host;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) host.bpm = *bpm;
            if (auto ppq = pos->getPpqPosition()) host.ppq = *ppq;
            host.playing = pos->getIsPlaying();
        }

    // On garde le verrou pendant tout le bloc : les tables user ne peuvent pas être libérées pendant le rendu.
    const juce::SpinLock::ScopedTryLockType lock (tableLock);
    if (lock.isLocked())
        engine.setUserTables (userTable[0].get(), userTable[1].get());
    else
        engine.setUserTables (nullptr, nullptr);

    engine.process (buffer, midi, host);
    midi.clear();
}

//==============================================================================
int NoxWaveProcessor::getNumPrograms() { return (int) nox::factoryPresets().size(); }

const juce::String NoxWaveProcessor::getProgramName (int index)
{
    const auto& p = nox::factoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? juce::String (p[(size_t) index].name) : juce::String();
}

void NoxWaveProcessor::setCurrentProgram (int index)
{
    const auto& presets = nox::factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;
    const auto& pr = presets[(size_t) index];

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : pr.values)
    {
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
        else
            jassertfalse;   // identifiant inconnu dans un preset
    }
    presetName = pr.name;
}

//==============================================================================
void NoxWaveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("presetName", presetName);
    xml->setAttribute ("program", currentProgram);

    for (int o = 0; o < 2; ++o)
        if (userTable[o] != nullptr && ! userTable[o]->rawFrames.empty())
        {
            auto* e = xml->createNewChildElement (o == 0 ? "UserTableA" : "UserTableB");
            e->setAttribute ("name", userTable[o]->name);
            e->setAttribute ("frames", userTable[o]->numFrames);
            juce::MemoryBlock mb (userTable[o]->rawFrames.data(), userTable[o]->rawFrames.size() * sizeof (float));
            e->setAttribute ("data", mb.toBase64Encoding());
        }
    copyXmlToBinary (*xml, destData);
}

void NoxWaveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    presetName = xml->getStringAttribute ("presetName", "Init");
    currentProgram = xml->getIntAttribute ("program", 0);

    for (int o = 0; o < 2; ++o)
    {
        nox::WavetableData::Ptr t;
        if (auto* e = xml->getChildByName (o == 0 ? "UserTableA" : "UserTableB"))
        {
            juce::MemoryBlock mb;
            const int frames = e->getIntAttribute ("frames");
            if (mb.fromBase64Encoding (e->getStringAttribute ("data")) && frames > 0
                && mb.getSize() == (size_t) frames * nox::WavetableData::N * sizeof (float))
            {
                std::vector<float> raw (mb.getSize() / sizeof (float));
                std::memcpy (raw.data(), mb.getData(), mb.getSize());
                t = nox::WavetableData::fromFrames (raw, frames, e->getStringAttribute ("name"));
            }
        }
        setUserTable (o, t);
    }
    xml->deleteAllChildElementsWithTagName ("UserTableA");
    xml->deleteAllChildElementsWithTagName ("UserTableB");
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
void NoxWaveProcessor::setUserTable (int osc, nox::WavetableData::Ptr table)
{
    const juce::SpinLock::ScopedLockType lock (tableLock);
    if (userTable[osc] != nullptr) graveyard.add (userTable[osc]);
    userTable[osc] = table;
}

juce::String NoxWaveProcessor::loadUserTable (int osc, const juce::File& file)
{
    juce::String error;
    auto t = nox::loadWavetableFile (file, error);
    if (t == nullptr) return error.isEmpty() ? "Impossible de charger le fichier." : error;
    setUserTable (osc, t);
    if (auto* p = apvts.getParameter (nox::oscId (osc, "table")))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) nox::kUserTableIndex));
    return {};
}

const nox::WavetableData* NoxWaveProcessor::getDisplayTable (int osc) const
{
    const int idx = (int) std::lround (apvts.getRawParameterValue (nox::oscId (osc, "table"))->load());
    if (idx >= nox::kUserTableIndex)
        return userTable[osc] != nullptr ? userTable[osc].get() : engine.getFactory().tables.getObjectPointer (0);
    return engine.getFactory().tables.getObjectPointer (juce::jlimit (0, nox::kNumFactoryTables - 1, idx));
}

void NoxWaveProcessor::timerCallback()
{
    if (graveyard.isEmpty()) return;
    const juce::SpinLock::ScopedLockType lock (tableLock);
    graveyard.clear();
}

juce::AudioProcessorEditor* NoxWaveProcessor::createEditor()
{
    return new NoxWaveEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoxWaveProcessor();
}
