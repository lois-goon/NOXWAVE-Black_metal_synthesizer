#include "Params.h"

namespace nox
{
const juce::StringArray& tableNames()
{
    static juce::StringArray a { "Basic Shapes", "PWM Sweep", "Razor Saw", "Vox Formant", "Funeral Organ",
                                 "Harmonic Rise", "FM Metal", "Sync Sweep", "Wavefold", "Hollow Comb",
                                 "Grim Fog", "Digital Grit", "User (WAV)" };
    return a;
}
const juce::StringArray& warpNames()
{
    static juce::StringArray a { "Off", "Sync", "Bend +", "Bend -", "PWM", "Mirror", "FM (autre osc)", "Bitcrush" };
    return a;
}
const juce::StringArray& filterNames()
{
    static juce::StringArray a { "LP 12", "LP 24", "HP 12", "HP 24", "BP 12", "Notch", "Ladder LP", "Ladder Dirty" };
    return a;
}
const juce::StringArray& lfoShapeNames()
{
    static juce::StringArray a { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Sample & Hold", "Smooth Random" };
    return a;
}
const juce::StringArray& syncDivNames()
{
    static juce::StringArray a { "4/1", "2/1", "1/1", "1/2", "1/2.", "1/2T", "1/4", "1/4.", "1/4T",
                                 "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T", "1/32", "1/32T", "1/64" };
    return a;
}
float syncDivBeats (int i)
{
    static const float b[] = { 16.0f, 8.0f, 4.0f, 2.0f, 3.0f, 4.0f / 3.0f, 1.0f, 1.5f, 2.0f / 3.0f,
                               0.5f, 0.75f, 1.0f / 3.0f, 0.25f, 0.375f, 1.0f / 6.0f, 0.125f, 1.0f / 12.0f, 0.0625f };
    return b[juce::jlimit (0, 17, i)];
}
int syncDivIndex (const char* name) { return juce::jmax (0, syncDivNames().indexOf (name)); }

const juce::StringArray& distNames()
{
    static juce::StringArray a { "Tube", "Hard Clip", "Fuzz", "Foldback", "Bitcrush", "Rectify" };
    return a;
}
const juce::StringArray& tremShapeNames()
{
    static juce::StringArray a { "Sine", "Square", "Chop (picking)", "Gate 75%" };
    return a;
}
const juce::StringArray& subShapeNames()
{
    static juce::StringArray a { "Sine", "Triangle", "Square", "Saw" };
    return a;
}
const juce::StringArray& srcNames()
{
    static juce::StringArray a { "-", "Env 2", "Env 3", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Velocity",
                                 "Mod Wheel", "Aftertouch", "Note (Key)", "Macro 1", "Macro 2", "Macro 3",
                                 "Macro 4", "Random", "Pitch Bend" };
    return a;
}
const juce::StringArray& dstNames()
{
    static juce::StringArray a { "-", "A Position", "A Warp", "A Pitch", "A Level", "A Detune", "A Pan",
                                 "B Position", "B Warp", "B Pitch", "B Level", "B Detune", "B Pan",
                                 "Sub Level", "Noise Level", "Filter Cutoff", "Filter Reso", "Filter Drive",
                                 "Voice Amp", "FX Dist Drive", "FX Tremolo Depth", "FX Delay Mix", "FX Reverb Mix" };
    return a;
}
const juce::StringArray& voiceModeNames()
{
    static juce::StringArray a { "Poly", "Mono", "Legato" };
    return a;
}

//==============================================================================
namespace
{
using APF  = juce::AudioParameterFloat;
using APC  = juce::AudioParameterChoice;
using API  = juce::AudioParameterInt;
using Attr = juce::AudioParameterFloatAttributes;

juce::String pct (float v, int)  { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String bip (float v, int)  { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String secs (float v, int) { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s"; }
juce::String hz (float v, int)   { return v < 1000.0f ? juce::String (v, v < 10 ? 2 : 0) + " Hz" : juce::String (v / 1000.0f, 2) + " kHz"; }
juce::String db (float v, int)   { return juce::String (v, 1) + " dB"; }
juce::String pan (float v, int)
{
    if (std::abs (v) < 0.005f) return "C";
    return juce::String (juce::roundToInt (std::abs (v) * 100.0f)) + (v < 0 ? " L" : " R");
}

struct Builder
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    void f (const juce::String& id, const juce::String& name, juce::NormalisableRange<float> r, float def,
            juce::String (*fn) (float, int) = pct)
    {
        params.push_back (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def,
                                                 Attr().withStringFromValueFunction (fn)));
    }
    void c (const juce::String& id, const juce::String& name, const juce::StringArray& ch, int def)
    { params.push_back (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, ch, def)); }
    // Interrupteurs : choix Off/On (valeur toujours quantifiée -> restauration d'état exacte)
    void b (const juce::String& id, const juce::String& name, bool def)
    { params.push_back (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, juce::StringArray { "Off", "On" }, def ? 1 : 0)); }
    void i (const juce::String& id, const juce::String& name, int lo, int hi, int def)
    { params.push_back (std::make_unique<API> (juce::ParameterID { id, 1 }, name, lo, hi, def)); }
};

juce::NormalisableRange<float> lin (float a, float b) { return { a, b }; }
juce::NormalisableRange<float> skewed (float a, float b, float centre)
{
    juce::NormalisableRange<float> r (a, b);
    r.setSkewForCentre (centre);
    return r;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    Builder p;

    for (int o = 0; o < 2; ++o)
    {
        const juce::String N = o == 0 ? "Osc A " : "Osc B ";
        p.b (oscId (o, "on"), N + "On", o == 0);
        p.c (oscId (o, "table"), N + "Wavetable", tableNames(), 0);
        p.f (oscId (o, "pos"), N + "Position", lin (0, 1), o == 0 ? 0.66f : 0.0f);
        p.c (oscId (o, "warpMode"), N + "Warp Mode", warpNames(), 0);
        p.f (oscId (o, "warp"), N + "Warp", lin (0, 1), 0.0f);
        p.i (oscId (o, "oct"), N + "Octave", -4, 4, 0);
        p.i (oscId (o, "semi"), N + "Semi", -12, 12, 0);
        p.f (oscId (o, "fine"), N + "Fine", lin (-100, 100), 0.0f,
             [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; });
        p.i (oscId (o, "unison"), N + "Unison", 1, 16, 1);
        p.f (oscId (o, "detune"), N + "Detune", lin (0, 1), 0.25f);
        p.f (oscId (o, "blend"), N + "Blend", lin (0, 1), 0.75f);
        p.f (oscId (o, "width"), N + "Width", lin (0, 1), 0.8f);
        p.f (oscId (o, "level"), N + "Level", lin (0, 1), 0.75f);
        p.f (oscId (o, "pan"), N + "Pan", lin (-1, 1), 0.0f, pan);
        p.f (oscId (o, "rand"), N + "Phase Rand", lin (0, 1), 1.0f);
    }

    p.b ("sub_on", "Sub On", false);
    p.c ("sub_shape", "Sub Shape", subShapeNames(), 0);
    p.i ("sub_oct", "Sub Octave", -3, 0, -1);
    p.f ("sub_level", "Sub Level", lin (0, 1), 0.5f);
    p.b ("noise_on", "Noise On", false);
    p.f ("noise_level", "Noise Level", lin (0, 1), 0.3f);
    p.f ("noise_color", "Noise Color", lin (0, 1), 0.5f);

    p.b ("flt_on", "Filter On", true);
    p.c ("flt_type", "Filter Type", filterNames(), F_LP24);
    p.f ("flt_cutoff", "Filter Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 8000.0f, hz);
    p.f ("flt_res", "Filter Reso", lin (0, 1), 0.1f);
    p.f ("flt_drive", "Filter Drive", lin (0, 1), 0.0f);
    p.f ("flt_env", "Filter Env Amt", lin (-1, 1), 0.0f, bip);
    p.f ("flt_key", "Filter Keytrack", lin (0, 1), 0.0f);
    p.f ("flt_mix", "Filter Mix", lin (0, 1), 1.0f);

    const float envDef[3][4] = { { 0.005f, 0.3f, 0.8f, 0.3f }, { 0.01f, 0.5f, 0.3f, 0.5f }, { 0.01f, 1.0f, 0.0f, 1.0f } };
    for (int e = 0; e < 3; ++e)
    {
        const juce::String N = "Env " + juce::String (e + 1) + " ";
        p.f (envId (e, "a"), N + "Attack", skewed (0.0005f, 20.0f, 1.0f), envDef[e][0], secs);
        p.f (envId (e, "d"), N + "Decay", skewed (0.001f, 20.0f, 1.0f), envDef[e][1], secs);
        p.f (envId (e, "s"), N + "Sustain", lin (0, 1), envDef[e][2]);
        p.f (envId (e, "r"), N + "Release", skewed (0.001f, 30.0f, 1.0f), envDef[e][3], secs);
    }

    for (int l = 0; l < kNumLfos; ++l)
    {
        const juce::String N = "LFO " + juce::String (l + 1) + " ";
        p.c (lfoId (l, "shape"), N + "Shape", lfoShapeNames(), 0);
        p.f (lfoId (l, "rate"), N + "Rate", skewed (0.01f, 30.0f, 2.0f), 1.0f, hz);
        p.b (lfoId (l, "sync"), N + "Sync", false);
        p.c (lfoId (l, "div"), N + "Division", syncDivNames(), syncDivIndex ("1/4"));
        p.b (lfoId (l, "retrig"), N + "Retrig", true);
    }

    for (int m = 0; m < 4; ++m)
        p.f (macroId (m), "Macro " + juce::String (m + 1), lin (0, 1), 0.0f);

    // Les slots 9..12 sont pré-câblés sur les macros (Bright / Grim / Abyss / Morph)
    for (int m = 0; m < kNumModSlots; ++m)
    {
        int defSrc = S_None, defDst = D_None;
        float defAmt = 0.0f;
        if (m == 8)  { defSrc = S_Macro1; defDst = D_Cutoff;    defAmt = 0.45f; }
        if (m == 9)  { defSrc = S_Macro2; defDst = D_DistDrive; defAmt = 0.6f; }
        if (m == 10) { defSrc = S_Macro3; defDst = D_RevMix;    defAmt = 0.45f; }
        if (m == 11) { defSrc = S_Macro4; defDst = D_APos;      defAmt = 0.6f; }
        const juce::String N = "Mod " + juce::String (m + 1) + " ";
        p.c (modId (m, "src"), N + "Source", srcNames(), defSrc);
        p.c (modId (m, "dst"), N + "Destination", dstNames(), defDst);
        p.f (modId (m, "amt"), N + "Amount", lin (-1, 1), defAmt, bip);
    }

    p.c ("voice_mode", "Voice Mode", voiceModeNames(), VM_Poly);
    p.f ("glide", "Glide", skewed (0.0f, 2.0f, 0.2f), 0.0f, secs);
    p.i ("bend_range", "Bend Range", 0, 24, 2);
    p.f ("amp_vel", "Velocity Sens", lin (0, 1), 0.5f);
    p.f ("master", "Master", lin (0, 1), 0.7f);

    // --- FX -------------------------------------------------------------------
    p.b ("dist_on", "Dist On", false);
    p.c ("dist_mode", "Dist Mode", distNames(), DM_Tube);
    p.f ("dist_drive", "Dist Drive", lin (0, 1), 0.4f);
    p.f ("dist_tone", "Dist Tone", lin (0, 1), 0.7f);
    p.f ("dist_mix", "Dist Mix", lin (0, 1), 1.0f);

    p.b ("eq_on", "EQ On", false);
    p.f ("eq_low", "EQ Low", lin (-15, 15), 0.0f, db);
    p.f ("eq_mid", "EQ Mid", lin (-15, 15), 0.0f, db);
    p.f ("eq_midfreq", "EQ Mid Freq", skewed (150.0f, 8000.0f, 1000.0f), 1000.0f, hz);
    p.f ("eq_high", "EQ High", lin (-15, 15), 0.0f, db);

    p.b ("trem_on", "Tremolo On", false);
    p.c ("trem_div", "Tremolo Rate", syncDivNames(), syncDivIndex ("1/16"));
    p.f ("trem_depth", "Tremolo Depth", lin (0, 1), 0.8f);
    p.c ("trem_shape", "Tremolo Shape", tremShapeNames(), T_Chop);

    p.b ("cho_on", "Chorus On", false);
    p.f ("cho_rate", "Chorus Rate", skewed (0.05f, 5.0f, 0.8f), 0.6f, hz);
    p.f ("cho_depth", "Chorus Depth", lin (0, 1), 0.4f);
    p.f ("cho_mix", "Chorus Mix", lin (0, 1), 0.4f);

    p.b ("dly_on", "Delay On", false);
    p.c ("dly_div", "Delay Time", syncDivNames(), syncDivIndex ("1/8."));
    p.f ("dly_fb", "Delay Feedback", lin (0, 0.95f), 0.4f);
    p.f ("dly_tone", "Delay Tone", lin (0, 1), 0.5f);
    p.b ("dly_ping", "Delay Ping-Pong", true);
    p.f ("dly_mix", "Delay Mix", lin (0, 1), 0.25f);

    p.b ("rev_on", "Reverb On", false);
    p.f ("rev_size", "Reverb Size", lin (0, 1), 0.7f);
    p.f ("rev_decay", "Reverb Decay", skewed (0.3f, 30.0f, 4.0f), 5.0f, secs);
    p.f ("rev_damp", "Reverb Damp", lin (0, 1), 0.4f);
    p.f ("rev_predelay", "Reverb Predelay", lin (0, 250), 20.0f,
         [] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; });
    p.f ("rev_mix", "Reverb Mix", lin (0, 1), 0.3f);

    return { p.params.begin(), p.params.end() };
}
} // namespace nox
