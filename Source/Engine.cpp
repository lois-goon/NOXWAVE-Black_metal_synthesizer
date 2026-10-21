#include "Engine.h"

namespace nox
{
Engine::Engine()
{
    heldNotes.reserve (128);
}

void Engine::bind (juce::AudioProcessorValueTreeState& a)
{
    auto r = [&a] (const juce::String& id)
    {
        PRef p; p.p = a.getRawParameterValue (id);
        jassert (p.p != nullptr);
        return p;
    };
    for (int o = 0; o < 2; ++o)
    {
        auto& R = oscR[o];
        R.on = r (oscId (o, "on")); R.table = r (oscId (o, "table")); R.pos = r (oscId (o, "pos"));
        R.warpMode = r (oscId (o, "warpMode")); R.warp = r (oscId (o, "warp")); R.oct = r (oscId (o, "oct"));
        R.semi = r (oscId (o, "semi")); R.fine = r (oscId (o, "fine")); R.unison = r (oscId (o, "unison"));
        R.detune = r (oscId (o, "detune")); R.blend = r (oscId (o, "blend")); R.width = r (oscId (o, "width"));
        R.level = r (oscId (o, "level")); R.pan = r (oscId (o, "pan")); R.rand = r (oscId (o, "rand"));
    }
    subOn = r ("sub_on"); subShape = r ("sub_shape"); subOct = r ("sub_oct"); subLevel = r ("sub_level");
    noiseOn = r ("noise_on"); noiseLevel = r ("noise_level"); noiseColor = r ("noise_color");
    fltOn = r ("flt_on"); fltType = r ("flt_type"); fltCutoff = r ("flt_cutoff"); fltRes = r ("flt_res");
    fltDrive = r ("flt_drive"); fltEnv = r ("flt_env"); fltKey = r ("flt_key"); fltMix = r ("flt_mix");
    const char* adsr[4] = { "a", "d", "s", "r" };
    for (int e = 0; e < 3; ++e)
        for (int k = 0; k < 4; ++k) envR[e][k] = r (envId (e, adsr[k]));
    for (int l = 0; l < kNumLfos; ++l)
    {
        lfoShape[l] = r (lfoId (l, "shape")); lfoRate[l] = r (lfoId (l, "rate")); lfoSync[l] = r (lfoId (l, "sync"));
        lfoDiv[l] = r (lfoId (l, "div")); lfoRetrig[l] = r (lfoId (l, "retrig"));
    }
    for (int m = 0; m < 4; ++m) macroR[m] = r (macroId (m));
    for (int m = 0; m < kNumModSlots; ++m)
    {
        modSrc[m] = r (modId (m, "src")); modDst[m] = r (modId (m, "dst")); modAmt[m] = r (modId (m, "amt"));
    }
    voiceMode = r ("voice_mode"); glide = r ("glide"); bendRange = r ("bend_range"); ampVel = r ("amp_vel"); master = r ("master");
    distOn = r ("dist_on"); distMode = r ("dist_mode"); distDrive = r ("dist_drive"); distTone = r ("dist_tone"); distMix = r ("dist_mix");
    eqOn = r ("eq_on"); eqLow = r ("eq_low"); eqMid = r ("eq_mid"); eqMidF = r ("eq_midfreq"); eqHigh = r ("eq_high");
    tremOn = r ("trem_on"); tremDiv = r ("trem_div"); tremDepth = r ("trem_depth"); tremShape = r ("trem_shape");
    choOn = r ("cho_on"); choRate = r ("cho_rate"); choDepth = r ("cho_depth"); choMix = r ("cho_mix");
    dlyOn = r ("dly_on"); dlyDiv = r ("dly_div"); dlyFb = r ("dly_fb"); dlyTone = r ("dly_tone"); dlyPing = r ("dly_ping"); dlyMix = r ("dly_mix");
    revOn = r ("rev_on"); revSize = r ("rev_size"); revDecay = r ("rev_decay"); revDamp = r ("rev_damp"); revPre = r ("rev_predelay"); revMix = r ("rev_mix");
}

void Engine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    for (auto& v : voices)
        for (auto& e : v.env) e.setSampleRate ((float) sr);
    fx.prepare (sr, std::max (64, maxBlock));
    masterGain.reset (sr, 0.03);
    dcCoef = std::exp (-kTwoPi * 5.0f / (float) sr);
    masterGain.setCurrentAndTargetValue (master.f());
    for (int l = 0; l < kNumLfos; ++l) globalLfo[l].reset (0.0f, 1234u + (uint32_t) l);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices)
    {
        for (auto& e : v.env) e.reset();
        v.note = -1; v.gate = false; v.sustained = false;
    }
    heldNotes.clear();
    fx.reset();
}

void Engine::readSnap()
{
    for (int o = 0; o < 2; ++o)
    {
        auto& R = oscR[o]; auto& O = s.osc[o];
        O.on = R.on.b(); O.table = R.table.i(); O.pos = R.pos.f(); O.warpMode = R.warpMode.i(); O.warp = R.warp.f();
        O.oct = R.oct.i(); O.semi = R.semi.i(); O.fine = R.fine.f(); O.unison = juce::jlimit (1, kMaxUnison, R.unison.i());
        O.detune = R.detune.f(); O.blend = R.blend.f(); O.width = R.width.f(); O.level = R.level.f();
        O.pan = R.pan.f(); O.rand = R.rand.f();
    }
    s.subOn = subOn.b(); s.subShape = subShape.i(); s.subOct = subOct.i(); s.subLevel = subLevel.f();
    s.noiseOn = noiseOn.b(); s.noiseLevel = noiseLevel.f(); s.noiseColor = noiseColor.f();
    s.fltOn = fltOn.b(); s.fltType = fltType.i(); s.cutoff = fltCutoff.f(); s.res = fltRes.f(); s.drive = fltDrive.f();
    s.fltEnv = fltEnv.f(); s.fltKey = fltKey.f(); s.fltMix = fltMix.f();
    for (int e = 0; e < 3; ++e)
        for (int k = 0; k < 4; ++k) s.env[e][k] = envR[e][k].f();
    for (int l = 0; l < kNumLfos; ++l)
    {
        s.lfoShape[l] = lfoShape[l].i(); s.lfoRate[l] = lfoRate[l].f(); s.lfoSync[l] = lfoSync[l].b();
        s.lfoDiv[l] = lfoDiv[l].i(); s.lfoRetrig[l] = lfoRetrig[l].b();
    }
    for (int m = 0; m < 4; ++m) s.macro[m] = macroR[m].f();
    for (int m = 0; m < kNumModSlots; ++m)
    {
        s.modSrc[m] = modSrc[m].i(); s.modDst[m] = modDst[m].i(); s.modAmt[m] = modAmt[m].f();
    }
    s.voiceMode = voiceMode.i(); s.glide = glide.f(); s.bendRange = bendRange.i(); s.ampVel = ampVel.f(); s.master = master.f();

    auto& f = s.fx;
    f.distOn = distOn.b(); f.distMode = distMode.i(); f.distDrive = distDrive.f(); f.distTone = distTone.f(); f.distMix = distMix.f();
    f.eqOn = eqOn.b(); f.eqLow = eqLow.f(); f.eqMid = eqMid.f(); f.eqMidF = eqMidF.f(); f.eqHigh = eqHigh.f();
    f.tremOn = tremOn.b(); f.tremDiv = tremDiv.i(); f.tremDepth = tremDepth.f(); f.tremShape = tremShape.i();
    f.choOn = choOn.b(); f.choRate = choRate.f(); f.choDepth = choDepth.f(); f.choMix = choMix.f();
    f.dlyOn = dlyOn.b(); f.dlyDiv = dlyDiv.i(); f.dlyFb = dlyFb.f(); f.dlyTone = dlyTone.f(); f.dlyPing = dlyPing.b(); f.dlyMix = dlyMix.f();
    f.revOn = revOn.b(); f.revSize = revSize.f(); f.revDecay = revDecay.f(); f.revDamp = revDamp.f(); f.revPre = revPre.f(); f.revMix = revMix.f();
}

float Engine::lfoRateHz (int l) const
{
    if (s.lfoSync[l])
        return (float) (host.bpm / 60.0 / syncDivBeats (s.lfoDiv[l]));
    return s.lfoRate[l];
}

const WavetableData* Engine::tableFor (int osc, int index) const
{
    if (index >= kUserTableIndex)
        return userTable[osc] != nullptr ? userTable[osc] : (const WavetableData*) factory->tables.getObjectPointer (0);
    return factory->tables.getObjectPointer (juce::jlimit (0, factory->tables.size() - 1, index));
}

//==============================================================================
void Engine::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const HostInfo& h)
{
    host = h;
    if (host.bpm <= 1.0) host.bpm = 120.0;
    readSnap();

    const int n = buffer.getNumSamples();
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    for (auto& v : voices)
        for (int e = 0; e < 3; ++e)
            v.env[e].setParams (s.env[e][0], s.env[e][1], s.env[e][2], s.env[e][3]);

    // LFO globaux (pour les destinations FX et la phase des LFO libres)
    for (int l = 0; l < kNumLfos; ++l)
    {
        if (s.lfoSync[l] && host.playing)
        {
            const double b = host.ppq / syncDivBeats (s.lfoDiv[l]);
            globalLfo[l].phase = (float) (b - std::floor (b));
        }
        globalLfoVal[l] = globalLfo[l].value (s.lfoShape[l]);
        globalLfo[l].advance (lfoRateHz (l) * (float) n / (float) sr);
    }

    int pos = 0;
    for (const auto meta : midi)
    {
        const int smp = juce::jlimit (0, n, meta.samplePosition);
        if (smp > pos) { renderRange (L, R, pos, smp); pos = smp; }
        handleMidi (meta.getMessage());
    }
    if (pos < n) renderRange (L, R, pos, n);

    // Modulations globales des FX (sources "globales" seulement)
    float gsrc[S_Count] {};
    for (int l = 0; l < kNumLfos; ++l) gsrc[S_Lfo1 + l] = globalLfoVal[l];
    gsrc[S_ModWheel] = modWheel; gsrc[S_Aftertouch] = aftertouch; gsrc[S_PitchBend] = pitchBend;
    for (int m = 0; m < 4; ++m) gsrc[S_Macro1 + m] = s.macro[m];
    float gd[D_Count] {};
    for (int m = 0; m < kNumModSlots; ++m)
        if (s.modDst[m] >= kFirstGlobalDst && s.modSrc[m] > S_None && s.modSrc[m] < S_Count)
            gd[s.modDst[m]] += s.modAmt[m] * gsrc[s.modSrc[m]];

    FxParams fp = s.fx;
    fp.distDrive = clamp01 (fp.distDrive + gd[D_DistDrive]);
    fp.tremDepth = clamp01 (fp.tremDepth + gd[D_TremDepth]);
    fp.dlyMix    = clamp01 (fp.dlyMix + gd[D_DelayMix]);
    fp.revMix    = clamp01 (fp.revMix + gd[D_RevMix]);
    fx.process (L, R, n, fp, host);

    // Volume master + limiteur doux de sécurité
    masterGain.setTargetValue (s.master * s.master * 1.6f);
    for (int i = 0; i < n; ++i)
    {
        const float g = masterGain.getNextValue();
        float l = L[i] * g, r = R[i] * g;
        // bloqueur de DC (~5 Hz)
        const float yl = l - dcX[0] + dcCoef * dcY[0]; dcX[0] = l; dcY[0] = yl; l = yl;
        const float yr = r - dcX[1] + dcCoef * dcY[1]; dcX[1] = r; dcY[1] = yr; r = yr;
        if (std::abs (l) > 0.8f) l = (l > 0 ? 1.0f : -1.0f) * (0.8f + 0.2f * fastTanh ((std::abs (l) - 0.8f) * 5.0f));
        if (std::abs (r) > 0.8f) r = (r > 0 ? 1.0f : -1.0f) * (0.8f + 0.2f * fastTanh ((std::abs (r) - 0.8f) * 5.0f));
        L[i] = std::isfinite (l) ? l : 0.0f;
        R[i] = std::isfinite (r) ? r : 0.0f;
    }

    int act = 0;
    for (auto& v : voices) if (v.active()) ++act;
    activeVoices.store (act);
    if (act == 0) { displayPos[0].store (-1.0f); displayPos[1].store (-1.0f); }
}

//==============================================================================
void Engine::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())           noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())        pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure())   aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())        aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        if (cc == 1) modWheel = (float) m.getControllerValue() / 127.0f;
        else if (cc == 64)
        {
            sustainPedal = m.getControllerValue() >= 64;
            if (! sustainPedal)
                for (auto& v : voices)
                    if (v.sustained) { v.sustained = false; v.gate = false; for (auto& e : v.env) e.noteOff(); }
        }
        else if (cc == 120 || cc == 123)
        {
            for (auto& v : voices) { v.gate = false; v.sustained = false; for (auto& e : v.env) e.noteOff(); }
            heldNotes.clear();
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        for (auto& v : voices) { v.gate = false; for (auto& e : v.env) e.noteOff(); }
        heldNotes.clear();
    }
}

void Engine::startVoice (Voice& v, int note, float vel, bool retrigger)
{
    const bool wasActive = v.active();
    v.note = note; v.vel = vel; v.gate = true; v.sustained = false;
    v.stamp = ++stampCounter;
    v.target = (float) note;
    if (! wasActive || s.glide <= 0.0005f || lastNotePitch < 0.0f)
        v.pitch = (s.glide > 0.0005f && lastNotePitch >= 0.0f) ? lastNotePitch : (float) note;
    lastNotePitch = (float) note;

    if (retrigger)
    {
        v.rnd = rng.bi();
        v.rng.seed (rng.next());
        if (! wasActive)
        {
            for (int o = 0; o < 2; ++o)
            {
                for (int u = 0; u < kMaxUnison; ++u)
                    v.phase[o][u] = s.osc[o].unison > 1 || u > 0 ? v.rng.uni() * s.osc[o].rand : 0.0f;
                v.prevPos[o] = -1.0f;
                v.lastOut[o] = 0.0f;
            }
            v.subPhase = 0.0f;
            for (auto& st : v.svf) for (auto& f : st) f.reset();
            for (auto& l : v.ladder) l.reset();
            v.prevG = -1.0f; v.prevAmp = 1.0f;
            for (auto& e : v.env) e.level = 0.0f;
        }
        for (int l = 0; l < kNumLfos; ++l)
            v.lfo[l].reset (s.lfoRetrig[l] ? 0.0f : globalLfo[l].phase, rng.next());
        for (auto& e : v.env) e.noteOn();
    }
    lastVoice = &v;
}

void Engine::noteOn (int note, float vel)
{
    if (s.voiceMode != VM_Poly)
    {
        Voice& v = voices[0];
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        const bool legato = ! heldNotes.empty() && v.gate && v.active();
        heldNotes.push_back (note);
        // couper les éventuelles autres voix restées d'un mode poly
        for (size_t i = 1; i < voices.size(); ++i) { voices[i].gate = false; for (auto& e : voices[i].env) e.noteOff(); }

        if (legato && s.voiceMode == VM_Legato)
        {
            v.note = note; v.target = (float) note; v.stamp = ++stampCounter;
            lastNotePitch = (float) note;
            if (s.glide <= 0.0005f) v.pitch = (float) note;
        }
        else
        {
            if (v.active() && s.glide > 0.0005f) lastNotePitch = v.pitch;
            startVoice (v, note, vel, true);
        }
        return;
    }

    Voice* chosen = nullptr;
    for (auto& v : voices)   // même note déjà en jeu : on la relance
        if (v.active() && v.note == note) { chosen = &v; break; }
    if (chosen == nullptr)
        for (auto& v : voices)
            if (! v.active()) { chosen = &v; break; }
    if (chosen == nullptr)   // vol de voix : la plus faible en release, sinon la plus ancienne
    {
        float best = 2.0f;
        for (auto& v : voices)
            if (! v.gate && v.env[0].level < best) { best = v.env[0].level; chosen = &v; }
        if (chosen == nullptr)
        {
            uint64_t oldest = UINT64_MAX;
            for (auto& v : voices)
                if (v.stamp < oldest) { oldest = v.stamp; chosen = &v; }
        }
    }
    startVoice (*chosen, note, vel, true);
}

void Engine::noteOff (int note)
{
    if (s.voiceMode != VM_Poly)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        Voice& v = voices[0];
        if (v.note != note) return;
        if (! heldNotes.empty())
        {
            const int prev = heldNotes.back();
            v.note = prev; v.target = (float) prev; lastNotePitch = (float) prev;
            if (s.glide <= 0.0005f) v.pitch = (float) prev;
            return;
        }
        if (sustainPedal) { v.sustained = true; return; }
        v.gate = false;
        for (auto& e : v.env) e.noteOff();
        return;
    }

    for (auto& v : voices)
        if (v.gate && v.note == note)
        {
            if (sustainPedal) { v.sustained = true; continue; }
            v.gate = false;
            for (auto& e : v.env) e.noteOff();
        }
}

//==============================================================================
void Engine::renderRange (float* L, float* R, int start, int end)
{
    for (int pos = start; pos < end; pos += kSubBlock)
    {
        const int n = std::min (kSubBlock, end - pos);
        for (auto& v : voices)
            if (v.active())
                renderVoice (v, L + pos, R + pos, n);
    }
}

namespace
{
inline float warpPhase (float p, int mode, float amt, float fmIn, float syncRatio)
{
    switch (mode)
    {
        case W_Sync:  { const float q = p * syncRatio; return q - (float) (int) q; }
        case W_BendP: { const float k = 1.0f + amt * 8.0f; return p / (p + k * (1.0f - p)); }
        case W_BendM: { const float k = 1.0f / (1.0f + amt * 8.0f); return p / (p + k * (1.0f - p)); }
        case W_PWM:
        {
            const float w = 0.5f - amt * 0.47f;
            return p < w ? p * (0.5f / w) : 0.5f + (p - w) * (0.5f / (1.0f - w));
        }
        case W_Mirror:
        {
            const float c = 1.0f - 0.5f * amt;
            return p < c ? p / c : (1.0f - p) / (1.0f - c);
        }
        case W_FM: { const float q = p + amt * 2.0f * fmIn; return q - std::floor (q); }
        default: return p;
    }
}
} // namespace

void Engine::renderVoice (Voice& v, float* outL, float* outR, int n)
{
    constexpr int TN = WavetableData::N;
    const float fsr = (float) sr;

    // --- glide
    if (s.glide > 0.0005f)
        v.pitch += (v.target - v.pitch) * (1.0f - std::exp (-(float) n / (s.glide * fsr)));
    else
        v.pitch = v.target;

    // --- sources de modulation
    float src[S_Count] {};
    src[S_Env2] = v.env[1].level;
    src[S_Env3] = v.env[2].level;
    for (int l = 0; l < kNumLfos; ++l)
    {
        src[S_Lfo1 + l] = v.lfo[l].value (s.lfoShape[l]);
        v.lfo[l].advance (lfoRateHz (l) * (float) n / fsr);
    }
    src[S_Vel] = v.vel; src[S_ModWheel] = modWheel; src[S_Aftertouch] = aftertouch;
    src[S_Key] = ((float) v.note - 60.0f) / 60.0f;
    for (int m = 0; m < 4; ++m) src[S_Macro1 + m] = s.macro[m];
    src[S_Random] = v.rnd; src[S_PitchBend] = pitchBend;

    float d[D_Count] {};
    for (int m = 0; m < kNumModSlots; ++m)
    {
        const int sI = s.modSrc[m], dI = s.modDst[m];
        if (sI > S_None && sI < S_Count && dI > D_None && dI < kFirstGlobalDst)
            d[dI] += s.modAmt[m] * src[sI];
    }

    const float basePitch = v.pitch + pitchBend * (float) s.bendRange;
    float mixL[kSubBlock] {}, mixR[kSubBlock] {};

    // --- préparation des oscillateurs
    struct Run
    {
        bool on = false; const WavetableData* wt = nullptr; int frames = 1;
        float pos0 = 0, pos1 = 0, warp = 0, syncRatio = 1, crushSteps = 4096;
        int mode = 0, U = 1;
        float inc[kMaxUnison], gL[kMaxUnison], gR[kMaxUnison];
        int lvl[kMaxUnison];
    } run[2];

    const float allowedHz = std::max (0.5f * fsr, fsr - 18000.0f);
    for (int o = 0; o < 2; ++o)
    {
        const auto& O = s.osc[o];
        auto& r = run[o];
        if (! O.on) continue;
        r.on = true;
        r.wt = tableFor (o, O.table);
        r.frames = r.wt->numFrames;
        const int base = o == 0 ? D_APos : D_BPos;
        const float pos = clamp01 (O.pos + d[base + 0]);
        r.pos1 = pos;
        r.pos0 = v.prevPos[o] < 0.0f ? pos : v.prevPos[o];
        v.prevPos[o] = pos;
        r.warp = clamp01 (O.warp + d[base + 1]);
        r.mode = O.warpMode;
        r.syncRatio = 1.0f + r.warp * 7.0f;
        r.crushSteps = std::pow (2.0f, 1.0f + (1.0f - r.warp) * 11.0f);
        const float pitch = basePitch + (float) (O.oct * 12 + O.semi) + O.fine * 0.01f + d[base + 2] * 24.0f;
        const float level = clamp01 (O.level + d[base + 3]);
        const float detune = clamp01 (O.detune + d[base + 4]);
        const float pan = std::clamp (O.pan + d[base + 5], -1.0f, 1.0f);
        r.U = O.unison;
        const float spread = std::pow (detune, 1.5f) * 1.0f;

        float sumSq = 0.0f;
        for (int u = 0; u < r.U; ++u)
        {
            const float du = r.U == 1 ? 0.0f : 2.0f * (float) u / (float) (r.U - 1) - 1.0f;
            const float f = 440.0f * std::exp2 ((pitch + du * spread - 69.0f) / 12.0f);
            r.inc[u] = std::min (f / fsr, 0.49f);
            const float harmonicsAllowed = allowedHz / (f * (r.mode == W_Sync ? r.syncRatio : 1.0f));
            r.lvl[u] = harmonicsAllowed >= 1024.0f ? 0
                     : juce::jlimit (0, WavetableData::kLevels - 1, (int) std::ceil (std::log2 (1024.0f / harmonicsAllowed)));
            const float g = 1.0f - (1.0f - O.blend) * std::abs (du) * 0.9f;
            const float p = std::clamp (pan + du * O.width * ((u & 1) ? -1.0f : 1.0f), -1.0f, 1.0f);
            const float ang = (p + 1.0f) * kPi * 0.25f;
            r.gL[u] = g * std::cos (ang);
            r.gR[u] = g * std::sin (ang);
            sumSq += g * g;
        }
        const float norm = level * 1.41421356f / std::sqrt (std::max (sumSq, 1.0e-6f));
        for (int u = 0; u < r.U; ++u) { r.gL[u] *= norm; r.gR[u] *= norm; }
    }

    // --- rendu échantillon par échantillon (B puis A, pour le FM croisé)
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) (i + 1) / (float) n;
        for (int oi = 1; oi >= 0; --oi)
        {
            auto& r = run[oi];
            if (! r.on) continue;
            const float fpos = lerp (r.pos0, r.pos1, t) * (float) (r.frames - 1);
            const int f0 = std::min ((int) fpos, r.frames - 1);
            const int f1 = std::min (f0 + 1, r.frames - 1);
            const float ff = fpos - (float) f0;
            const float fmIn = v.lastOut[1 - oi];

            float accL = 0.0f, accR = 0.0f, mono = 0.0f;
            float* ph = v.phase[oi];
            for (int u = 0; u < r.U; ++u)
            {
                float p = ph[u];
                float pw = r.mode == W_Off || r.mode == W_Crush ? p : warpPhase (p, r.mode, r.warp, fmIn, r.syncRatio);
                pw = std::clamp (pw, 0.0f, 0.99999f);
                const float idx = pw * (float) TN;
                const int i0 = (int) idx;
                const float fr = idx - (float) i0;
                const float* a = r.wt->get (f0, r.lvl[u]);
                const float* b = r.wt->get (f1, r.lvl[u]);
                const float sa = a[i0] + fr * (a[i0 + 1] - a[i0]);
                const float sb = b[i0] + fr * (b[i0 + 1] - b[i0]);
                float smp = sa + ff * (sb - sa);
                if (r.mode == W_Crush) smp = std::round (smp * r.crushSteps) / r.crushSteps;
                accL += smp * r.gL[u];
                accR += smp * r.gR[u];
                mono += smp;
                p += r.inc[u];
                if (p >= 1.0f) p -= 1.0f;
                ph[u] = p;
            }
            v.lastOut[oi] = mono / (float) r.U;
            mixL[i] += accL;
            mixR[i] += accR;
        }
    }

    // --- sous-oscillateur
    if (s.subOn)
    {
        const float lvl = clamp01 (s.subLevel + d[D_SubLevel]) * 0.8f;
        const float f = 440.0f * std::exp2 ((basePitch + (float) s.subOct * 12.0f - 69.0f) / 12.0f);
        const float inc = std::min (f / fsr, 0.49f);
        for (int i = 0; i < n; ++i)
        {
            const float p = v.subPhase;
            float y;
            switch (s.subShape)
            {
                case 0:  y = std::sin (kTwoPi * p); break;
                case 1:  y = p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f); break;
                case 2:
                {
                    y = p < 0.5f ? 1.0f : -1.0f;
                    y += polyBlep (p, inc);
                    float p2 = p + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
                    y -= polyBlep (p2, inc);
                    y *= 0.7f;
                    break;
                }
                default: y = (2.0f * p - 1.0f - polyBlep (p, inc)) * 0.8f; break;
            }
            v.subPhase += inc;
            if (v.subPhase >= 1.0f) v.subPhase -= 1.0f;
            mixL[i] += y * lvl;
            mixR[i] += y * lvl;
        }
    }

    // --- bruit
    if (s.noiseOn)
    {
        const float lvl = clamp01 (s.noiseLevel + d[D_NoiseLevel]) * 0.6f;
        const float c = s.noiseColor;
        const bool lowpass = c < 0.5f;
        const float fc = lowpass ? 150.0f * std::pow (130.0f, c * 2.0f) : 20.0f * std::pow (250.0f, (c - 0.5f) * 2.0f);
        const float a = 1.0f - std::exp (-kTwoPi * std::min (fc, 0.45f * fsr) / fsr);
        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float w = v.rng.bi();
                v.noiseLp[ch] += a * (w - v.noiseLp[ch]);
                const float y = lowpass ? v.noiseLp[ch] * (1.0f + (0.5f - c) * 4.0f) : w - v.noiseLp[ch];
                (ch == 0 ? mixL : mixR)[i] += y * lvl;
            }
    }

    // --- filtre
    if (s.fltOn)
    {
        const float oct = s.fltEnv * v.env[1].level * 8.0f + s.fltKey * (v.pitch - 60.0f) / 12.0f + d[D_Cutoff] * 8.0f;
        const float fc = std::clamp (s.cutoff * std::exp2 (oct), 16.0f, std::min (20000.0f, 0.45f * fsr));
        const float g1 = std::tan (kPi * fc / fsr);
        const float g0 = v.prevG < 0.0f ? g1 : v.prevG;
        v.prevG = g1;
        const float res = clamp01 (s.res + d[D_Res]);
        const float drive = clamp01 (s.drive + d[D_Drive]);
        const float dg = 1.0f + drive * 9.0f;
        const float mix = s.fltMix;
        const int type = s.fltType;
        const float kRes = 2.0f - 1.98f * res;
        const float kLad = res * 3.95f;

        for (int i = 0; i < n; ++i)
        {
            const float g = lerp (g0, g1, (float) (i + 1) / (float) n);
            float* io[2] = { &mixL[i], &mixR[i] };
            for (int ch = 0; ch < 2; ++ch)
            {
                const float dry = *io[ch];
                float x = drive > 0.001f ? fastTanh (dry * dg) / std::sqrt (dg) * 1.5f : dry;
                float y;
                switch (type)
                {
                    case F_LP12:  y = v.svf[0][ch].process (x, g, kRes, 0); break;
                    case F_LP24:  y = v.svf[1][ch].process (v.svf[0][ch].process (x, g, 1.414f, 0), g, kRes, 0); break;
                    case F_HP12:  y = v.svf[0][ch].process (x, g, kRes, 1); break;
                    case F_HP24:  y = v.svf[1][ch].process (v.svf[0][ch].process (x, g, 1.414f, 1), g, kRes, 1); break;
                    case F_BP:    y = v.svf[0][ch].process (x, g, std::max (0.08f, kRes), 2); break;
                    case F_Notch: y = v.svf[0][ch].process (x, g, std::max (0.3f, kRes), 3); break;
                    case F_Ladder:      y = v.ladder[ch].process (x, g, kLad, false); break;
                    case F_LadderDirty:
                    default:            y = v.ladder[ch].process (x, g, kLad * 1.05f, true); break;
                }
                *io[ch] = dry + mix * (y - dry);
            }
        }
    }

    // --- ampli
    const float velGain = 1.0f - s.ampVel * (1.0f - v.vel);
    const float amp1 = std::clamp (1.0f + d[D_Amp], 0.0f, 2.0f);
    const float amp0 = v.prevAmp;
    v.prevAmp = amp1;
    for (int i = 0; i < n; ++i)
    {
        const float e = v.env[0].next();
        const float g = e * velGain * lerp (amp0, amp1, (float) (i + 1) / (float) n) * 0.5f;
        outL[i] += mixL[i] * g;
        outR[i] += mixR[i] * g;
    }
    v.env[1].advance (n);
    v.env[2].advance (n);

    if (&v == lastVoice)
    {
        displayPos[0].store (run[0].on ? run[0].pos1 : -1.0f);
        displayPos[1].store (run[1].on ? run[1].pos1 : -1.0f);
    }
    if (! v.active()) { v.note = -1; v.gate = false; }
}
} // namespace nox
