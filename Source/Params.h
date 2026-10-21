#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace nox
{
// ---- Identifiants ------------------------------------------------------------
inline juce::String oscId (int o, const char* n)   { return juce::String (o == 0 ? "A_" : "B_") + n; }
inline juce::String envId (int e, const char* n)   { return "env" + juce::String (e + 1) + "_" + n; }
inline juce::String lfoId (int l, const char* n)   { return "lfo" + juce::String (l + 1) + "_" + n; }
inline juce::String modId (int m, const char* n)   { return "mod" + juce::String (m + 1) + "_" + n; }
inline juce::String macroId (int m)                { return "macro" + juce::String (m + 1); }

constexpr int kNumFactoryTables = 12;   // l'index 12 = table utilisateur (WAV)
constexpr int kUserTableIndex   = kNumFactoryTables;
constexpr int kNumModSlots      = 12;
constexpr int kNumLfos          = 4;

enum WarpMode   { W_Off, W_Sync, W_BendP, W_BendM, W_PWM, W_Mirror, W_FM, W_Crush };
enum FilterType { F_LP12, F_LP24, F_HP12, F_HP24, F_BP, F_Notch, F_Ladder, F_LadderDirty };
enum DistMode   { DM_Tube, DM_Hard, DM_Fuzz, DM_Fold, DM_Crush, DM_Rectify };
enum TremShape  { T_Sine, T_Square, T_Chop, T_Gate };
enum VoiceMode  { VM_Poly, VM_Mono, VM_Legato };

enum Src
{
    S_None, S_Env2, S_Env3, S_Lfo1, S_Lfo2, S_Lfo3, S_Lfo4, S_Vel, S_ModWheel, S_Aftertouch,
    S_Key, S_Macro1, S_Macro2, S_Macro3, S_Macro4, S_Random, S_PitchBend, S_Count
};

enum Dst
{
    D_None,
    D_APos, D_AWarp, D_APitch, D_ALevel, D_ADetune, D_APan,
    D_BPos, D_BWarp, D_BPitch, D_BLevel, D_BDetune, D_BPan,
    D_SubLevel, D_NoiseLevel, D_Cutoff, D_Res, D_Drive, D_Amp,
    D_DistDrive, D_TremDepth, D_DelayMix, D_RevMix, D_Count
};
constexpr int kFirstGlobalDst = D_DistDrive;

const juce::StringArray& tableNames();
const juce::StringArray& warpNames();
const juce::StringArray& filterNames();
const juce::StringArray& lfoShapeNames();
const juce::StringArray& syncDivNames();
const juce::StringArray& distNames();
const juce::StringArray& tremShapeNames();
const juce::StringArray& subShapeNames();
const juce::StringArray& srcNames();
const juce::StringArray& dstNames();
const juce::StringArray& voiceModeNames();

float syncDivBeats (int index);   // longueur d'une division en noires
int   syncDivIndex (const char* name);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace nox
