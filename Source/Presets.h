#pragma once
// Presets d'usine orientés black metal / dungeon synth.
// Valeurs en unités réelles (Hz, secondes, index de choix...). Tout paramètre absent = valeur par défaut.
#include "Params.h"
#include "Dsp.h"
#include <vector>
#include <utility>

namespace nox
{
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline float sdiv (const char* n) { return (float) syncDivIndex (n); }

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets
    {
        { "Init", {} },

        { "Frostbitten Pad", {
            { "A_table", 2 }, { "A_pos", 0.3f }, { "A_unison", 7 }, { "A_detune", 0.35f }, { "A_blend", 0.7f },
            { "A_width", 0.9f }, { "A_level", 0.75f },
            { "B_on", 1 }, { "B_table", 3 }, { "B_pos", 0.2f }, { "B_oct", 1 }, { "B_unison", 4 },
            { "B_detune", 0.25f }, { "B_width", 0.8f }, { "B_level", 0.3f },
            { "sub_on", 1 }, { "sub_level", 0.25f },
            { "flt_type", F_LP24 }, { "flt_cutoff", 1800 }, { "flt_res", 0.2f }, { "flt_env", 0.25f },
            { "env1_a", 1.2f }, { "env1_d", 1.0f }, { "env1_s", 0.9f }, { "env1_r", 2.5f },
            { "env2_a", 2.5f }, { "env2_d", 3.0f }, { "env2_s", 0.4f }, { "env2_r", 3.0f },
            { "lfo1_rate", 0.15f }, { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APos }, { "mod1_amt", 0.25f },
            { "lfo2_shape", L_Smooth }, { "lfo2_rate", 0.08f }, { "lfo2_retrig", 0 },
            { "mod2_src", S_Lfo2 }, { "mod2_dst", D_Cutoff }, { "mod2_amt", 0.08f },
            { "cho_on", 1 }, { "cho_rate", 0.4f }, { "cho_depth", 0.4f }, { "cho_mix", 0.4f },
            { "rev_on", 1 }, { "rev_size", 0.85f }, { "rev_decay", 9.0f }, { "rev_damp", 0.5f },
            { "rev_predelay", 40 }, { "rev_mix", 0.45f } } },

        { "Tremolo Wraith", {
            { "A_table", 0 }, { "A_pos", 0.66f }, { "A_unison", 5 }, { "A_detune", 0.18f }, { "A_blend", 0.6f },
            { "A_width", 0.6f },
            { "B_on", 1 }, { "B_table", 7 }, { "B_pos", 0.3f }, { "B_level", 0.5f }, { "B_unison", 3 },
            { "B_detune", 0.12f }, { "B_fine", 7 },
            { "flt_type", F_LP24 }, { "flt_cutoff", 3500 }, { "flt_res", 0.15f }, { "flt_env", 0.2f }, { "flt_key", 0.5f },
            { "env1_a", 0.003f }, { "env1_d", 0.3f }, { "env1_s", 0.85f }, { "env1_r", 0.25f },
            { "dist_on", 1 }, { "dist_mode", DM_Tube }, { "dist_drive", 0.55f }, { "dist_tone", 0.6f },
            { "eq_on", 1 }, { "eq_low", -3 }, { "eq_mid", 3 }, { "eq_midfreq", 1200 }, { "eq_high", -4 },
            { "trem_on", 1 }, { "trem_div", sdiv ("1/16") }, { "trem_depth", 0.85f }, { "trem_shape", T_Chop },
            { "rev_on", 1 }, { "rev_size", 0.7f }, { "rev_decay", 5.0f }, { "rev_mix", 0.3f }, { "glide", 0.03f } } },

        { "Grim Choir", {
            { "A_table", 3 }, { "A_pos", 0.0f }, { "A_unison", 6 }, { "A_detune", 0.2f }, { "A_width", 1.0f },
            { "A_blend", 0.8f },
            { "B_on", 1 }, { "B_table", 3 }, { "B_pos", 0.6f }, { "B_oct", -1 }, { "B_unison", 4 },
            { "B_detune", 0.3f }, { "B_level", 0.5f },
            { "lfo1_rate", 0.07f }, { "lfo1_retrig", 0 }, { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APos }, { "mod1_amt", 0.35f },
            { "lfo2_shape", L_Tri }, { "lfo2_rate", 0.11f }, { "lfo2_retrig", 0 },
            { "mod2_src", S_Lfo2 }, { "mod2_dst", D_BPos }, { "mod2_amt", -0.3f },
            { "flt_type", F_LP12 }, { "flt_cutoff", 3200 }, { "flt_res", 0.1f },
            { "env1_a", 0.8f }, { "env1_d", 1.0f }, { "env1_s", 1.0f }, { "env1_r", 3.0f },
            { "cho_on", 1 },
            { "rev_on", 1 }, { "rev_size", 0.95f }, { "rev_decay", 14.0f }, { "rev_damp", 0.4f },
            { "rev_predelay", 60 }, { "rev_mix", 0.55f } } },

        { "Funeral Organ", {
            { "A_table", 4 }, { "A_pos", 0.6f }, { "A_unison", 2 }, { "A_detune", 0.08f }, { "A_width", 0.4f },
            { "B_on", 1 }, { "B_table", 4 }, { "B_pos", 0.9f }, { "B_oct", 1 }, { "B_level", 0.35f },
            { "flt_type", F_LP12 }, { "flt_cutoff", 6000 },
            { "env1_a", 0.02f }, { "env1_d", 0.1f }, { "env1_s", 1.0f }, { "env1_r", 0.6f },
            { "lfo1_rate", 5.5f }, { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APitch }, { "mod1_amt", 0.006f },
            { "mod2_src", S_Lfo1 }, { "mod2_dst", D_BPitch }, { "mod2_amt", 0.006f },
            { "dist_on", 1 }, { "dist_drive", 0.25f }, { "dist_mix", 0.5f },
            { "cho_on", 1 }, { "cho_rate", 0.8f },
            { "rev_on", 1 }, { "rev_size", 0.9f }, { "rev_decay", 7.0f }, { "rev_mix", 0.4f } } },

        { "Necro Drone", {
            { "A_table", 10 }, { "A_pos", 0.2f }, { "A_unison", 8 }, { "A_detune", 0.4f }, { "A_width", 1.0f }, { "A_oct", -1 },
            { "B_on", 1 }, { "B_table", 8 }, { "B_pos", 0.4f }, { "B_oct", -2 }, { "B_level", 0.5f },
            { "B_warpMode", W_FM }, { "B_warp", 0.2f },
            { "lfo1_shape", L_Smooth }, { "lfo1_rate", 0.12f }, { "lfo1_retrig", 0 },
            { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APos }, { "mod1_amt", 0.5f },
            { "lfo2_rate", 0.05f }, { "lfo2_retrig", 0 }, { "mod2_src", S_Lfo2 }, { "mod2_dst", D_BPos }, { "mod2_amt", 0.4f },
            { "lfo3_shape", L_Tri }, { "lfo3_rate", 0.09f }, { "lfo3_retrig", 0 },
            { "mod3_src", S_Lfo3 }, { "mod3_dst", D_Cutoff }, { "mod3_amt", 0.15f },
            { "sub_on", 1 }, { "sub_level", 0.4f }, { "sub_oct", -1 },
            { "flt_type", F_Ladder }, { "flt_cutoff", 900 }, { "flt_res", 0.45f }, { "flt_drive", 0.4f },
            { "env1_a", 3.0f }, { "env1_d", 2.0f }, { "env1_s", 1.0f }, { "env1_r", 6.0f },
            { "dist_on", 1 }, { "dist_mode", DM_Fold }, { "dist_drive", 0.2f }, { "dist_mix", 0.3f },
            { "dly_on", 1 }, { "dly_div", sdiv ("1/4") }, { "dly_fb", 0.5f }, { "dly_mix", 0.2f },
            { "rev_on", 1 }, { "rev_size", 1.0f }, { "rev_decay", 20.0f }, { "rev_damp", 0.6f }, { "rev_mix", 0.5f }, { "master", 0.85f } } },

        { "Blizzard Wind", {
            { "A_on", 0 }, { "noise_on", 1 }, { "noise_level", 1.0f }, { "noise_color", 0.6f },
            { "flt_type", F_BP }, { "flt_cutoff", 900 }, { "flt_res", 0.55f }, { "master", 0.95f }, { "flt_key", 0.5f },
            { "lfo1_shape", L_Smooth }, { "lfo1_rate", 0.25f }, { "lfo1_retrig", 0 },
            { "mod1_src", S_Lfo1 }, { "mod1_dst", D_Cutoff }, { "mod1_amt", 0.35f },
            { "lfo2_shape", L_Smooth }, { "lfo2_rate", 0.4f }, { "lfo2_retrig", 0 },
            { "mod2_src", S_Lfo2 }, { "mod2_dst", D_Amp }, { "mod2_amt", 0.3f },
            { "env1_a", 2.0f }, { "env1_d", 1.0f }, { "env1_s", 1.0f }, { "env1_r", 4.0f },
            { "cho_on", 1 },
            { "rev_on", 1 }, { "rev_size", 1.0f }, { "rev_decay", 12.0f }, { "rev_mix", 0.5f } } },

        { "Cold Bass", {
            { "voice_mode", VM_Mono }, { "glide", 0.05f },
            { "A_table", 0 }, { "A_pos", 0.66f }, { "A_unison", 2 }, { "A_detune", 0.08f },
            { "B_on", 1 }, { "B_table", 7 }, { "B_level", 0.3f },
            { "sub_on", 1 }, { "sub_level", 0.6f }, { "sub_oct", -1 },
            { "flt_type", F_Ladder }, { "flt_cutoff", 600 }, { "flt_res", 0.3f }, { "flt_env", 0.45f }, { "flt_drive", 0.3f },
            { "env1_a", 0.002f }, { "env1_d", 0.5f }, { "env1_s", 0.9f }, { "env1_r", 0.15f },
            { "env2_a", 0.001f }, { "env2_d", 0.35f }, { "env2_s", 0.2f }, { "env2_r", 0.2f },
            { "dist_on", 1 }, { "dist_drive", 0.3f }, { "dist_mix", 0.6f },
            { "eq_on", 1 }, { "eq_low", 3 } } },

        { "Abyss Bell", {
            { "A_table", 6 }, { "A_pos", 0.1f }, { "A_unison", 3 }, { "A_detune", 0.1f },
            { "env3_a", 0.0005f }, { "env3_d", 1.5f }, { "env3_s", 0.0f }, { "env3_r", 1.5f },
            { "mod1_src", S_Env3 }, { "mod1_dst", D_APos }, { "mod1_amt", 0.5f },
            { "flt_type", F_LP12 }, { "flt_cutoff", 9000 },
            { "env1_a", 0.001f }, { "env1_d", 4.0f }, { "env1_s", 0.0f }, { "env1_r", 4.0f },
            { "dly_on", 1 }, { "dly_div", sdiv ("1/8.") }, { "dly_fb", 0.45f }, { "dly_mix", 0.3f },
            { "rev_on", 1 }, { "rev_size", 0.9f }, { "rev_decay", 10.0f }, { "rev_mix", 0.4f } } },

        { "Raw Buzzsaw", {
            { "A_table", 0 }, { "A_pos", 0.66f }, { "A_unison", 7 }, { "A_detune", 0.22f }, { "A_width", 1.0f },
            { "A_blend", 0.6f },
            { "B_on", 1 }, { "B_table", 2 }, { "B_pos", 0.5f }, { "B_unison", 5 }, { "B_detune", 0.3f },
            { "B_oct", -1 }, { "B_level", 0.6f },
            { "flt_type", F_HP12 }, { "flt_cutoff", 120 }, { "flt_res", 0.1f },
            { "env1_a", 0.002f }, { "env1_d", 0.2f }, { "env1_s", 1.0f }, { "env1_r", 0.15f },
            { "dist_on", 1 }, { "dist_mode", DM_Fuzz }, { "dist_drive", 0.75f }, { "dist_tone", 0.45f },
            { "eq_on", 1 }, { "eq_low", -2 }, { "eq_mid", 2 }, { "eq_midfreq", 1500 }, { "eq_high", -6 },
            { "rev_on", 1 }, { "rev_size", 0.5f }, { "rev_decay", 2.5f }, { "rev_mix", 0.18f } } },

        { "Dungeon Synth", {
            { "A_table", 1 }, { "A_pos", 0.35f }, { "A_unison", 3 }, { "A_detune", 0.12f },
            { "lfo1_shape", L_Tri }, { "lfo1_rate", 0.3f }, { "lfo1_retrig", 0 },
            { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APos }, { "mod1_amt", 0.2f },
            { "B_on", 1 }, { "B_table", 4 }, { "B_pos", 0.3f }, { "B_oct", 1 }, { "B_level", 0.3f },
            { "flt_type", F_LP24 }, { "flt_cutoff", 2200 }, { "flt_res", 0.1f },
            { "env1_a", 0.25f }, { "env1_d", 0.5f }, { "env1_s", 0.85f }, { "env1_r", 1.5f },
            { "dist_on", 1 }, { "dist_mode", DM_Crush }, { "dist_drive", 0.35f }, { "dist_mix", 0.35f },
            { "cho_on", 1 }, { "cho_rate", 0.3f }, { "cho_depth", 0.5f }, { "cho_mix", 0.5f },
            { "rev_on", 1 }, { "rev_size", 0.8f }, { "rev_decay", 6.0f }, { "rev_mix", 0.4f } } },

        { "Celestial Sorrow", {
            { "A_table", 5 }, { "A_pos", 0.4f }, { "A_unison", 9 }, { "A_detune", 0.3f }, { "A_width", 1.0f },
            { "B_on", 1 }, { "B_table", 3 }, { "B_pos", 0.7f }, { "B_oct", 1 }, { "B_level", 0.35f }, { "B_unison", 3 },
            { "lfo1_rate", 0.1f }, { "lfo1_retrig", 0 }, { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APos }, { "mod1_amt", 0.3f },
            { "env1_a", 2.0f }, { "env1_d", 2.0f }, { "env1_s", 1.0f }, { "env1_r", 5.0f },
            { "env2_a", 4.0f }, { "env2_d", 4.0f }, { "env2_s", 0.6f }, { "env2_r", 5.0f },
            { "flt_type", F_LP24 }, { "flt_cutoff", 2600 }, { "flt_res", 0.15f }, { "flt_env", 0.2f },
            { "dly_on", 1 }, { "dly_div", sdiv ("1/4") }, { "dly_fb", 0.55f }, { "dly_tone", 0.4f }, { "dly_mix", 0.25f },
            { "rev_on", 1 }, { "rev_size", 1.0f }, { "rev_decay", 18.0f }, { "rev_damp", 0.3f }, { "rev_mix", 0.55f } } },

        { "Black Throne Lead", {
            { "voice_mode", VM_Legato }, { "glide", 0.08f },
            { "A_table", 2 }, { "A_pos", 0.6f }, { "A_unison", 3 }, { "A_detune", 0.1f },
            { "B_on", 1 }, { "B_table", 0 }, { "B_pos", 1.0f }, { "B_level", 0.4f }, { "B_oct", -1 },
            { "lfo1_rate", 5.5f }, { "mod1_src", S_Lfo1 }, { "mod1_dst", D_APitch }, { "mod1_amt", 0.008f },
            { "mod2_src", S_ModWheel }, { "mod2_dst", D_Cutoff }, { "mod2_amt", 0.3f },
            { "flt_type", F_LP24 }, { "flt_cutoff", 2800 }, { "flt_res", 0.3f }, { "flt_env", 0.3f },
            { "env1_a", 0.01f }, { "env1_d", 0.4f }, { "env1_s", 0.9f }, { "env1_r", 0.4f },
            { "dist_on", 1 }, { "dist_drive", 0.45f },
            { "dly_on", 1 }, { "dly_div", sdiv ("1/8") }, { "dly_fb", 0.35f }, { "dly_mix", 0.22f },
            { "rev_on", 1 }, { "rev_size", 0.6f }, { "rev_mix", 0.25f } } },
    };
    return presets;
}
} // namespace nox
