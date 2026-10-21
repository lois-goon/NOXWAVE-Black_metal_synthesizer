#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"

class NoxWaveProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    NoxWaveProcessor();
    ~NoxWaveProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NoxWave"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Tables utilisateur (thread message uniquement)
    juce::String loadUserTable (int osc, const juce::File& file);
    const nox::WavetableData* getDisplayTable (int osc) const;   // table actuellement choisie (thread message)
    float getDisplayPos (int osc) const { return engine.displayPos[osc].load(); }
    int getActiveVoices() const { return engine.activeVoices.load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    juce::String presetName { "Init" };

private:
    void timerCallback() override;
    void setUserTable (int osc, nox::WavetableData::Ptr table);

    nox::Engine engine;
    int currentProgram = 0;

    juce::SpinLock tableLock;
    nox::WavetableData::Ptr userTable[2];
    juce::ReferenceCountedArray<nox::WavetableData> graveyard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoxWaveProcessor)
};
