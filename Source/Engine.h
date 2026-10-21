#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Dsp.h"
#include "Params.h"
#include "Wavetable.h"
#include "Fx.h"

namespace nox
{
constexpr int kMaxVoices = 16;
constexpr int kMaxUnison = 16;
constexpr int kSubBlock  = 32;

struct PRef
{
    std::atomic<float>* p = nullptr;
    float f() const { return p->load (std::memory_order_relaxed); }
    int   i() const { return (int) std::lround (p->load (std::memory_order_relaxed)); }
    bool  b() const { return p->load (std::memory_order_relaxed) > 0.5f; }
};

struct OscSnap
{
    bool on; int table; float pos; int warpMode; float warp; int oct, semi; float fine;
    int unison; float detune, blend, width, level, pan, rand;
};

struct Snap
{
    OscSnap osc[2];
    bool subOn; int subShape, subOct; float subLevel;
    bool noiseOn; float noiseLevel, noiseColor;
    bool fltOn; int fltType; float cutoff, res, drive, fltEnv, fltKey, fltMix;
    float env[3][4];
    int lfoShape[kNumLfos]; float lfoRate[kNumLfos]; bool lfoSync[kNumLfos]; int lfoDiv[kNumLfos]; bool lfoRetrig[kNumLfos];
    float macro[4];
    int modSrc[kNumModSlots], modDst[kNumModSlots]; float modAmt[kNumModSlots];
    int voiceMode; float glide; int bendRange; float ampVel, master;
    FxParams fx;
};

struct Voice
{
    int note = -1;
    float vel = 0.0f;
    bool gate = false, sustained = false;
    uint64_t stamp = 0;
    float pitch = 60.0f, target = 60.0f, rnd = 0.0f;
    Envelope env[3];
    Lfo lfo[kNumLfos];
    float phase[2][kMaxUnison] {};
    float lastOut[2] {};
    float prevPos[2] { -1.0f, -1.0f };
    float subPhase = 0.0f;
    float noiseLp[2] {};
    Svf svf[2][2];
    Ladder ladder[2];
    float prevG = -1.0f, prevAmp = 1.0f;
    Rng rng;
    bool active() const { return env[0].active(); }
};

class Engine
{
public:
    Engine();
    void bind (juce::AudioProcessorValueTreeState& apvts);
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const HostInfo& host);

    // Tables utilisateur (pointeurs bruts valides pendant le bloc, gérés par le processeur)
    void setUserTables (const WavetableData* a, const WavetableData* b) { userTable[0] = a; userTable[1] = b; }
    const FactoryBank& getFactory() const { return *factory; }

    std::atomic<float> displayPos[2] { { -1.0f }, { -1.0f } };
    std::atomic<int> activeVoices { 0 };

private:
    void readSnap();
    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int note, float vel);
    void noteOff (int note);
    void startVoice (Voice& v, int note, float vel, bool retrigger);
    void renderRange (float* L, float* R, int start, int end);
    void renderVoice (Voice& v, float* L, float* R, int n);
    float lfoRateHz (int l) const;
    const WavetableData* tableFor (int osc, int index) const;

    juce::SharedResourcePointer<FactoryBank> factory;
    const WavetableData* userTable[2] { nullptr, nullptr };

    // Références aux paramètres
    struct OscRefs { PRef on, table, pos, warpMode, warp, oct, semi, fine, unison, detune, blend, width, level, pan, rand; } oscR[2];
    PRef subOn, subShape, subOct, subLevel, noiseOn, noiseLevel, noiseColor;
    PRef fltOn, fltType, fltCutoff, fltRes, fltDrive, fltEnv, fltKey, fltMix;
    PRef envR[3][4];
    PRef lfoShape[kNumLfos], lfoRate[kNumLfos], lfoSync[kNumLfos], lfoDiv[kNumLfos], lfoRetrig[kNumLfos];
    PRef macroR[4], modSrc[kNumModSlots], modDst[kNumModSlots], modAmt[kNumModSlots];
    PRef voiceMode, glide, bendRange, ampVel, master;
    PRef distOn, distMode, distDrive, distTone, distMix, eqOn, eqLow, eqMid, eqMidF, eqHigh;
    PRef tremOn, tremDiv, tremDepth, tremShape, choOn, choRate, choDepth, choMix;
    PRef dlyOn, dlyDiv, dlyFb, dlyTone, dlyPing, dlyMix, revOn, revSize, revDecay, revDamp, revPre, revMix;

    Snap s {};
    HostInfo host;
    double sr = 48000.0;
    std::array<Voice, kMaxVoices> voices;
    Lfo globalLfo[kNumLfos];
    float globalLfoVal[kNumLfos] {};
    uint64_t stampCounter = 0;
    Voice* lastVoice = nullptr;

    float modWheel = 0.0f, aftertouch = 0.0f, pitchBend = 0.0f;
    bool sustainPedal = false;
    std::vector<int> heldNotes;       // pile de notes pour les modes mono/legato
    float lastNotePitch = -1.0f;
    Rng rng;

    FxChain fx;
    juce::SmoothedValue<float> masterGain;
    float dcX[2] {}, dcY[2] {}, dcCoef = 0.9993f;
};
} // namespace nox
