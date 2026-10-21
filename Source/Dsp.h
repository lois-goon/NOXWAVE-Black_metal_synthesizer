#pragma once
// Briques DSP de base : RNG, enveloppes, LFO, filtres (SVF, ladder, biquad), polyBLEP.
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace nox
{
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717958f;

inline float clamp01 (float x) { return std::clamp (x, 0.0f, 1.0f); }
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }

// tanh rapide (approximation de Padé, exacte à ±3)
inline float fastTanh (float x)
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    void seed (uint32_t v) { s = v ? v : 1u; }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    float bi() { return uni() * 2.0f - 1.0f; }
};

//==============================================================================
struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float level = 0.0f, sr = 48000.0f;
    float sus = 1.0f, attInc = 1.0f, decCoef = 0.0f, relCoef = 0.0f;

    void setSampleRate (float s) { sr = s; }
    void setParams (float a, float d, float s, float r)
    {
        attInc  = 1.0f / std::max (1.0f, a * sr);
        decCoef = std::exp (-6.9f / std::max (1.0f, d * sr));
        relCoef = std::exp (-6.9f / std::max (1.0f, r * sr));
        sus = s;
    }
    void noteOn()  { stage = Attack; }                 // repart du niveau courant (pas de clic)
    void noteOff() { if (stage != Idle) stage = Release; }
    void reset()   { stage = Idle; level = 0.0f; }
    bool active() const { return stage != Idle; }

    float next()
    {
        switch (stage)
        {
            case Attack:  level += attInc; if (level >= 1.0f) { level = 1.0f; stage = Decay; } break;
            case Decay:   level = sus + (level - sus) * decCoef;
                          if (std::abs (level - sus) < 1.0e-4f) { level = sus; stage = Sustain; } break;
            case Sustain: level += (sus - level) * 0.002f; break;
            case Release: level *= relCoef; if (level < 1.0e-5f) { level = 0.0f; stage = Idle; } break;
            case Idle: default: break;
        }
        return level;
    }
    void advance (int n) { for (int i = 0; i < n; ++i) next(); }
};

//==============================================================================
enum LfoShape { L_Sine, L_Tri, L_SawUp, L_SawDown, L_Square, L_SampleHold, L_Smooth };

struct Lfo
{
    float phase = 0.0f, sh = 0.0f, smA = 0.0f, smB = 0.0f;
    Rng rng;

    void reset (float ph, uint32_t seed)
    {
        phase = ph; rng.seed (seed);
        sh = rng.bi(); smA = rng.bi(); smB = rng.bi();
    }
    float value (int shape) const
    {
        const float p = phase;
        switch (shape)
        {
            case L_Sine:       return std::sin (kTwoPi * p);
            case L_Tri:        return p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f);
            case L_SawUp:      return 2.0f * p - 1.0f;
            case L_SawDown:    return 1.0f - 2.0f * p;
            case L_Square:     return p < 0.5f ? 1.0f : -1.0f;
            case L_SampleHold: return sh;
            case L_Smooth:     { const float t = p * p * (3.0f - 2.0f * p); return lerp (smA, smB, t); }
            default:           return 0.0f;
        }
    }
    void advance (float inc)
    {
        phase += inc;
        while (phase >= 1.0f)
        {
            phase -= 1.0f;
            sh = rng.bi(); smA = smB; smB = rng.bi();
        }
    }
};

//==============================================================================
// SVF zéro-délai (Simper / Cytomic)
struct Svf
{
    float ic1 = 0.0f, ic2 = 0.0f;
    void reset() { ic1 = ic2 = 0.0f; }
    // mode : 0 LP, 1 HP, 2 BP, 3 Notch
    inline float process (float v0, float g, float k, int mode)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        switch (mode)
        {
            case 0:  return v2;
            case 1:  return v0 - k * v1 - v2;
            case 2:  return v1 * k;
            default: return v0 - k * v1;
        }
    }
};

// Ladder 4 pôles TPT avec résolution de la boucle de feedback
struct Ladder
{
    float s[4] = { 0, 0, 0, 0 };
    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }
    inline float process (float x, float g, float k, bool dirty)
    {
        const float G = g / (1.0f + g);
        const float inv = 1.0f / (1.0f + g);
        const float sigma = G * G * G * s[0] * inv + G * G * s[1] * inv + G * s[2] * inv + s[3] * inv;
        float u = (x * (1.0f + 0.5f * k) - k * sigma) / (1.0f + k * G * G * G * G);
        u = dirty ? fastTanh (u * 1.6f) : fastTanh (u * 0.7f) * 1.4286f;
        float in = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v = (in - s[i]) * G;
            const float y = v + s[i];
            s[i] = y + v;
            in = y;
        }
        return in;
    }
};

//==============================================================================
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] = { 0, 0 }, z2[2] = { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
    void set (float B0, float B1, float B2, float A0, float A1, float A2)
    { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }

    void lowShelf (double sr, float f, float dB)
    {
        const float A = std::pow (10.0f, dB / 40.0f), w = kTwoPi * f / (float) sr;
        const float c = std::cos (w), al = std::sin (w) * 0.70710678f, sA = 2.0f * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sA),
             (A + 1) + (A - 1) * c + sA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sA);
    }
    void highShelf (double sr, float f, float dB)
    {
        const float A = std::pow (10.0f, dB / 40.0f), w = kTwoPi * f / (float) sr;
        const float c = std::cos (w), al = std::sin (w) * 0.70710678f, sA = 2.0f * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sA),
             (A + 1) - (A - 1) * c + sA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sA);
    }
    void peak (double sr, float f, float q, float dB)
    {
        const float A = std::pow (10.0f, dB / 40.0f), w = kTwoPi * f / (float) sr;
        const float c = std::cos (w), al = std::sin (w) / (2.0f * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    inline float process (float x, int ch)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}
} // namespace nox
