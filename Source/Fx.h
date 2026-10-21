#pragma once
// Chaîne d'effets : Distortion -> EQ -> Tremolo -> Chorus -> Delay -> Reverb
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include "Params.h"

namespace nox
{
struct HostInfo
{
    double bpm = 120.0;
    double ppq = 0.0;
    bool playing = false;
};

//==============================================================================
class Distortion
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate; maxN = maxBlock;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxBlock);
        dry.setSize (2, maxBlock);
        reset();
    }
    void reset()
    {
        if (os) os->reset();
        lp[0] = lp[1] = dcx[0] = dcx[1] = dcy[0] = dcy[1] = 0.0f;
        holdCount[0] = holdCount[1] = 0; held[0] = held[1] = 0.0f;
    }

    void process (float* L, float* R, int n, int mode, float drive, float tone, float mix)
    {
        for (int start = 0; start < n; start += maxN)
            processChunk (L + start, R + start, std::min (maxN, n - start), mode, drive, tone, mix);
    }

private:
    void processChunk (float* L, float* R, int n, int mode, float drive, float tone, float mix)
    {
        dry.copyFrom (0, 0, L, n);
        dry.copyFrom (1, 0, R, n);

        float* chans[2] = { L, R };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
        auto up = os->processSamplesUp (block);

        const float g = std::pow (10.0f, drive * 48.0f / 20.0f);
        const int upN = (int) up.getNumSamples();
        const float levels = std::pow (2.0f, 12.0f - drive * 10.0f);
        const int hold = 1 + (int) (drive * 24.0f);

        for (int ch = 0; ch < 2; ++ch)
        {
            float* d = up.getChannelPointer ((size_t) ch);
            for (int i = 0; i < upN; ++i)
            {
                const float x = d[i];
                float y;
                switch (mode)
                {
                    case DM_Tube:  y = fastTanh (g * x + 0.15f) - fastTanh (0.15f); break;
                    case DM_Hard:  y = std::clamp (g * x, -1.0f, 1.0f); break;
                    case DM_Fuzz:
                    {
                        const float v = g * x * 1.5f;
                        y = v > 0 ? 1.0f - std::exp (-v) : -0.8f * (1.0f - std::exp (v * 1.2f));
                        break;
                    }
                    case DM_Fold:
                    {
                        const float t = 0.25f * g * 0.5f * x + 0.25f;
                        y = 1.0f - 4.0f * std::abs (t - std::floor (t + 0.5f));
                        break;
                    }
                    case DM_Crush:
                    {
                        if (--holdCount[ch] <= 0) { holdCount[ch] = hold; held[ch] = std::round (x * levels) / levels; }
                        y = held[ch] * 1.4f;
                        break;
                    }
                    case DM_Rectify:
                    default:
                    {
                        const float v = fastTanh (g * x);
                        y = 0.6f * v + 0.6f * std::abs (v) - 0.3f;
                        break;
                    }
                }
                d[i] = y;
            }
        }
        os->processSamplesDown (block);

        const float fc = 800.0f * std::pow (2.0f, tone * 4.6f);
        const float a = 1.0f - std::exp (-kTwoPi * fc / (float) sr);
        const float comp = mode == DM_Crush ? 0.8f : 0.7f;

        float* outs[2] = { L, R };
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* dr = dry.getReadPointer (ch);
            float* o = outs[ch];
            for (int i = 0; i < n; ++i)
            {
                lp[ch] += a * (o[i] - lp[ch]);
                // bloqueur de DC
                const float y = lp[ch] - dcx[ch] + 0.995f * dcy[ch];
                dcx[ch] = lp[ch]; dcy[ch] = y;
                o[i] = dr[i] + mix * (y * comp - dr[i]);
            }
        }
    }

    double sr = 48000.0; int maxN = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::AudioBuffer<float> dry;
    float lp[2] {}, dcx[2] {}, dcy[2] {}, held[2] {};
    int holdCount[2] {};
};

//==============================================================================
class Eq3
{
public:
    void prepare (double s) { sr = s; lastLow = lastMid = lastHigh = lastF = -999.0f; reset(); }
    void reset() { lo.reset(); mi.reset(); hi.reset(); }
    void process (float* L, float* R, int n, float low, float mid, float midF, float high)
    {
        if (low != lastLow)   { lo.lowShelf (sr, 140.0f, low); lastLow = low; }
        if (mid != lastMid || midF != lastF) { mi.peak (sr, midF, 0.9f, mid); lastMid = mid; lastF = midF; }
        if (high != lastHigh) { hi.highShelf (sr, 6000.0f, high); lastHigh = high; }
        for (int i = 0; i < n; ++i)
        {
            L[i] = hi.process (mi.process (lo.process (L[i], 0), 0), 0);
            R[i] = hi.process (mi.process (lo.process (R[i], 1), 1), 1);
        }
    }
private:
    double sr = 48000.0;
    Biquad lo, mi, hi;
    float lastLow, lastMid, lastHigh, lastF;
};

//==============================================================================
// Trémolo synchronisé : idéal pour imiter le "tremolo picking" des guitares black metal
class Tremolo
{
public:
    void prepare (double s) { sr = s; phase = 0.0; smooth = 1.0f; }
    void process (float* L, float* R, int n, int div, float depth, int shape, const HostInfo& host)
    {
        const double beats = syncDivBeats (div);
        const double incBeats = host.bpm / 60.0 / sr;
        const float sm = 1.0f - std::exp (-1.0f / (0.0012f * (float) sr));
        for (int i = 0; i < n; ++i)
        {
            double p;
            if (host.playing) { const double b = (host.ppq + i * incBeats) / beats; p = b - std::floor (b); }
            else              { p = phase; phase += incBeats / beats; if (phase >= 1.0) phase -= std::floor (phase); }

            const float fp = (float) p;
            float s;
            switch (shape)
            {
                case T_Sine:   s = 0.5f + 0.5f * std::cos (kTwoPi * fp); break;
                case T_Square: s = fp < 0.5f ? 1.0f : 0.0f; break;
                case T_Chop:   { const float d = 1.0f - fp; s = d * d * (1.6f - 0.6f * d); s = std::min (1.0f, s * 1.15f); break; }
                case T_Gate:
                default:       s = fp < 0.75f ? 1.0f : 0.0f; break;
            }
            const float target = 1.0f - depth * (1.0f - s);
            smooth += sm * (target - smooth);
            L[i] *= smooth; R[i] *= smooth;
        }
    }
private:
    double sr = 48000.0, phase = 0.0;
    float smooth = 1.0f;
};

//==============================================================================
class ChorusFx
{
public:
    void prepare (double s, int maxBlock)
    {
        maxN = maxBlock;
        chorus.prepare ({ s, (juce::uint32) maxBlock, 2 });
        chorus.setCentreDelay (9.0f);
        chorus.setFeedback (0.08f);
    }
    void reset() { chorus.reset(); }
    void process (float* L, float* R, int n, float rate, float depth, float mix)
    {
        chorus.setRate (rate);
        chorus.setDepth (depth);
        chorus.setMix (mix);
        for (int start = 0; start < n; start += maxN)
        {
            float* chans[2] = { L + start, R + start };
            juce::dsp::AudioBlock<float> block (chans, 2, (size_t) std::min (maxN, n - start));
            chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
        }
    }
private:
    juce::dsp::Chorus<float> chorus;
    int maxN = 512;
};

//==============================================================================
class StereoDelay
{
public:
    void prepare (double s)
    {
        sr = s;
        size = (int) (s * 4.0) + 4;
        buf[0].assign ((size_t) size, 0.0f);
        buf[1].assign ((size_t) size, 0.0f);
        w = 0; cur = -1.0f; lp[0] = lp[1] = 0.0f;
    }
    void reset() { std::fill (buf[0].begin(), buf[0].end(), 0.0f); std::fill (buf[1].begin(), buf[1].end(), 0.0f); }

    void process (float* L, float* R, int n, int div, float fb, float tone, bool ping, float mix, const HostInfo& host)
    {
        const float target = std::clamp ((float) (syncDivBeats (div) * 60.0 / host.bpm * sr), 16.0f, (float) size - 4.0f);
        if (cur < 0.0f) cur = target;
        const float fc = 600.0f * std::pow (2.0f, tone * 5.0f);
        const float a = 1.0f - std::exp (-kTwoPi * fc / (float) sr);
        for (int i = 0; i < n; ++i)
        {
            cur += (target - cur) * 0.0004f;
            float rp = (float) w - cur;
            if (rp < 0.0f) rp += (float) size;
            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % size;
            const float fr = rp - (float) i0;
            const float dl = lerp (buf[0][(size_t) i0], buf[0][(size_t) i1], fr);
            const float dr = lerp (buf[1][(size_t) i0], buf[1][(size_t) i1], fr);
            lp[0] += a * (dl - lp[0]);
            lp[1] += a * (dr - lp[1]);

            float wl, wr;
            if (ping) { wl = 0.5f * (L[i] + R[i]) + fb * lp[1]; wr = fb * lp[0]; }
            else      { wl = L[i] + fb * lp[0];               wr = R[i] + fb * lp[1]; }
            buf[0][(size_t) w] = fastTanh (wl * 0.8f) * 1.25f;
            buf[1][(size_t) w] = fastTanh (wr * 0.8f) * 1.25f;
            if (++w >= size) w = 0;

            L[i] = L[i] * (1.0f - 0.3f * mix) + dl * mix;
            R[i] = R[i] * (1.0f - 0.3f * mix) + dr * mix;
        }
    }
private:
    double sr = 48000.0;
    std::vector<float> buf[2];
    int size = 0, w = 0;
    float cur = -1.0f, lp[2] {};
};

//==============================================================================
// Réverbe FDN 8 lignes (matrice de Hadamard) avec diffusion d'entrée et modulation
class FdnReverb
{
public:
    void prepare (double s)
    {
        sr = s;
        for (auto& l : lines) l.assign ((size_t) (0.4 * s) + 64, 0.0f);
        pre[0].assign ((size_t) (0.3 * s) + 4, 0.0f);
        pre[1].assign ((size_t) (0.3 * s) + 4, 0.0f);
        const int apBase[2][4] = { { 142, 107, 379, 277 }, { 151, 113, 397, 263 } };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 4; ++i)
            {
                ap[c][i].assign ((size_t) std::max (8, (int) (apBase[c][i] * s / 44100.0)), 0.0f);
                apIdx[c][i] = 0;
            }
        for (int i = 0; i < 8; ++i) { lp[i] = 0.0f; modPh[i] = (float) i / 8.0f; curLen[i] = -1.0f; }
        w = 0; pw = 0;
    }
    void reset()
    {
        for (auto& l : lines) std::fill (l.begin(), l.end(), 0.0f);
        for (auto& p : pre) std::fill (p.begin(), p.end(), 0.0f);
        for (auto& c : ap) for (auto& a : c) std::fill (a.begin(), a.end(), 0.0f);
        for (float& v : lp) v = 0.0f;
    }

    void process (float* L, float* R, int n, float sizeP, float decay, float damp, float predelayMs, float mix)
    {
        static const float baseMs[8] = { 31.7f, 37.3f, 41.9f, 46.1f, 53.7f, 59.3f, 67.1f, 73.9f };
        const float scale = 0.4f + sizeP * 1.6f;
        float gain[8], tgt[8];
        const int lineSize = (int) lines[0].size();
        for (int i = 0; i < 8; ++i)
        {
            tgt[i] = std::min (baseMs[i] * scale * 0.001f * (float) sr, (float) lineSize - 40.0f);
            if (curLen[i] < 0.0f) curLen[i] = tgt[i];
            gain[i] = std::pow (10.0f, -3.0f * curLen[i] / (decay * (float) sr));
        }
        const float fc = 20000.0f * std::pow (0.05f, damp);
        const float dampC = 1.0f - std::exp (-kTwoPi * std::min (fc, 0.45f * (float) sr) / (float) sr);
        const int preSize = (int) pre[0].size();
        const int preD = std::clamp ((int) (predelayMs * 0.001f * (float) sr), 0, preSize - 2);
        const float modDepth = 7.0f * (float) sr / 48000.0f;
        const float modInc[8] = { 0.31f, 0.47f, 0.53f, 0.67f, 0.41f, 0.59f, 0.37f, 0.71f };

        for (int s = 0; s < n; ++s)
        {
            // pré-delay
            pre[0][(size_t) pw] = L[s]; pre[1][(size_t) pw] = R[s];
            int rp = pw - preD; if (rp < 0) rp += preSize;
            float in[2] = { pre[0][(size_t) rp], pre[1][(size_t) rp] };
            if (++pw >= preSize) pw = 0;

            // diffusion (allpass en série)
            for (int c = 0; c < 2; ++c)
                for (int a = 0; a < 4; ++a)
                {
                    auto& b = ap[c][a];
                    int& idx = apIdx[c][a];
                    const float d = b[(size_t) idx];
                    const float v = in[c] + 0.62f * d;
                    b[(size_t) idx] = v;
                    in[c] = d - 0.62f * v;
                    if (++idx >= (int) b.size()) idx = 0;
                }

            float y[8];
            for (int i = 0; i < 8; ++i)
            {
                curLen[i] += (tgt[i] - curLen[i]) * 0.0002f;
                modPh[i] += modInc[i] / (float) sr;
                if (modPh[i] >= 1.0f) modPh[i] -= 1.0f;
                const float dly = curLen[i] + modDepth * std::sin (kTwoPi * modPh[i]);
                float r = (float) w - dly;
                if (r < 0.0f) r += (float) lineSize;
                const int i0 = (int) r;
                const int i1 = i0 + 1 >= lineSize ? 0 : i0 + 1;
                const float fr = r - (float) i0;
                const float v = lerp (lines[(size_t) i][(size_t) i0], lines[(size_t) i][(size_t) i1], fr);
                lp[i] += dampC * (v - lp[i]);
                y[i] = lp[i] * gain[i];
            }

            const float outL = (y[0] - y[2] + y[4] - y[6]) * 0.5f;
            const float outR = (y[1] - y[3] + y[5] - y[7]) * 0.5f;

            // Hadamard 8x8 (transformée rapide), normalisée
            for (int len = 1; len < 8; len <<= 1)
                for (int i = 0; i < 8; i += len << 1)
                    for (int j = i; j < i + len; ++j)
                    {
                        const float a = y[j], b = y[j + len];
                        y[j] = a + b; y[j + len] = a - b;
                    }
            const float norm = 0.35355339f;
            for (int i = 0; i < 8; ++i)
            {
                const float inj = (i & 1) ? in[1] : in[0];
                lines[(size_t) i][(size_t) w] = y[i] * norm + inj * ((i & 2) ? -0.5f : 0.5f);
            }
            if (++w >= lineSize) w = 0;

            L[s] = L[s] * (1.0f - mix) + outL * mix * 1.4f;
            R[s] = R[s] * (1.0f - mix) + outR * mix * 1.4f;
        }
    }
private:
    double sr = 48000.0;
    std::array<std::vector<float>, 8> lines;
    std::vector<float> pre[2];
    std::vector<float> ap[2][4];
    int apIdx[2][4] {};
    float lp[8] {}, modPh[8] {}, curLen[8] {};
    int w = 0, pw = 0;
};

//==============================================================================
struct FxParams
{
    bool distOn = false; int distMode = 0; float distDrive = 0, distTone = 0.7f, distMix = 1;
    bool eqOn = false; float eqLow = 0, eqMid = 0, eqMidF = 1000, eqHigh = 0;
    bool tremOn = false; int tremDiv = 12; float tremDepth = 0.8f; int tremShape = 2;
    bool choOn = false; float choRate = 0.6f, choDepth = 0.4f, choMix = 0.4f;
    bool dlyOn = false; int dlyDiv = 10; float dlyFb = 0.4f, dlyTone = 0.5f; bool dlyPing = true; float dlyMix = 0.25f;
    bool revOn = false; float revSize = 0.7f, revDecay = 5, revDamp = 0.4f, revPre = 20, revMix = 0.3f;
};

class FxChain
{
public:
    void prepare (double sr, int maxBlock)
    {
        dist.prepare (sr, maxBlock); eq.prepare (sr); trem.prepare (sr);
        cho.prepare (sr, maxBlock); dly.prepare (sr); rev.prepare (sr);
    }
    void reset() { dist.reset(); eq.reset(); cho.reset(); dly.reset(); rev.reset(); }

    void process (float* L, float* R, int n, const FxParams& p, const HostInfo& host)
    {
        if (p.distOn) dist.process (L, R, n, p.distMode, p.distDrive, p.distTone, p.distMix);
        if (p.eqOn)   eq.process (L, R, n, p.eqLow, p.eqMid, p.eqMidF, p.eqHigh);
        if (p.tremOn) trem.process (L, R, n, p.tremDiv, p.tremDepth, p.tremShape, host);
        if (p.choOn)  cho.process (L, R, n, p.choRate, p.choDepth, p.choMix);
        if (p.dlyOn)  dly.process (L, R, n, p.dlyDiv, p.dlyFb, p.dlyTone, p.dlyPing, p.dlyMix, host);
        if (p.revOn)  rev.process (L, R, n, p.revSize, p.revDecay, p.revDamp, p.revPre, p.revMix);
    }
private:
    Distortion dist; Eq3 eq; Tremolo trem; ChorusFx cho; StereoDelay dly; FdnReverb rev;
};
} // namespace nox
