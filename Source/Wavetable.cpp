#include "Wavetable.h"
#include "Dsp.h"
#include <complex>
#include <functional>

namespace nox
{
using Cpx = std::complex<float>;
using Spectrum = std::vector<Cpx>;   // index = numéro d'harmonique (0..1024)

namespace
{
constexpr int N = WavetableData::N;
constexpr int kHarm = N / 2;   // 1024

Spectrum toSpectrum (const float* x)
{
    juce::dsp::FFT fft (11);
    std::vector<Cpx> in ((size_t) N), out ((size_t) N);
    for (int i = 0; i < N; ++i) in[(size_t) i] = Cpx (x[i], 0.0f);
    fft.perform (in.data(), out.data(), false);
    Spectrum s ((size_t) kHarm + 1);
    for (int k = 0; k <= kHarm; ++k) s[(size_t) k] = out[(size_t) k];
    return s;
}

void fromSpectrum (const Spectrum& s, float* out, int maxHarm, juce::dsp::FFT& fft,
                   std::vector<Cpx>& freq, std::vector<Cpx>& time)
{
    std::fill (freq.begin(), freq.end(), Cpx());
    const int lim = std::min (maxHarm, kHarm - 1);
    for (int k = 1; k <= lim; ++k)
    {
        freq[(size_t) k] = s[(size_t) k];
        freq[(size_t) (N - k)] = std::conj (s[(size_t) k]);
    }
    fft.perform (freq.data(), time.data(), true);
    for (int i = 0; i < N; ++i) out[i] = time[(size_t) i].real();
    out[N] = out[0];
}

WavetableData::Ptr buildFromSpectra (const std::vector<Spectrum>& specs, const juce::String& name,
                                     std::vector<float> raw = {})
{
    WavetableData::Ptr wt (new WavetableData());
    wt->name = name;
    wt->numFrames = (int) specs.size();
    wt->data.resize ((size_t) wt->numFrames * WavetableData::kLevels * (N + 1));
    wt->rawFrames = std::move (raw);

    juce::dsp::FFT fft (11);
    std::vector<Cpx> freq ((size_t) N), time ((size_t) N);

    for (int f = 0; f < wt->numFrames; ++f)
    {
        float scale = 1.0f;
        for (int L = 0; L < WavetableData::kLevels; ++L)
        {
            float* dst = const_cast<float*> (wt->get (f, L));
            fromSpectrum (specs[(size_t) f], dst, kHarm >> L, fft, freq, time);
            if (L == 0)
            {
                float peak = 0.0f;
                for (int i = 0; i < N; ++i) peak = std::max (peak, std::abs (dst[i]));
                scale = peak > 1.0e-9f ? 1.0f / peak : 0.0f;
            }
            for (int i = 0; i <= N; ++i) dst[i] *= scale;
        }
    }

    // Aperçu pour l'interface
    const int dispFrames = std::min (wt->numFrames, 64);
    for (int d = 0; d < dispFrames; ++d)
    {
        const int f = dispFrames == 1 ? 0 : juce::roundToInt ((float) d * (float) (wt->numFrames - 1) / (float) (dispFrames - 1));
        const float* src = wt->get (f, 0);
        std::vector<float> pts (256);
        for (int i = 0; i < 256; ++i) pts[(size_t) i] = src[i * 8];
        wt->display.push_back (std::move (pts));
    }
    return wt;
}

Spectrum additive (const std::function<float (int)>& amp, const std::function<float (int)>& phase = nullptr)
{
    Spectrum s ((size_t) kHarm + 1);
    for (int k = 1; k < kHarm; ++k)
    {
        const float a = amp (k);
        if (a != 0.0f)
            s[(size_t) k] = std::polar (a, -kPi * 0.5f + (phase ? phase (k) : 0.0f));
    }
    return s;
}

using TimeFn = std::function<float (float t, float x)>;   // t = phase [0,1), x = morph [0,1]

WavetableData::Ptr fromTimeFn (const juce::String& name, int frames, const TimeFn& fn)
{
    std::vector<Spectrum> specs;
    std::vector<float> buf ((size_t) N);
    for (int f = 0; f < frames; ++f)
    {
        const float x = (float) f / (float) (frames - 1);
        for (int i = 0; i < N; ++i) buf[(size_t) i] = fn ((float) i / (float) N, x);
        specs.push_back (toSpectrum (buf.data()));
    }
    return buildFromSpectra (specs, name);
}

WavetableData::Ptr fromAdditive (const juce::String& name, int frames,
                                 const std::function<Spectrum (float x)>& fn)
{
    std::vector<Spectrum> specs;
    for (int f = 0; f < frames; ++f)
        specs.push_back (fn ((float) f / (float) (frames - 1)));
    return buildFromSpectra (specs, name);
}

float frac (float v) { return v - std::floor (v); }
float sawT (float t) { return 2.0f * frac (t) - 1.0f; }
float triT (float t) { t = frac (t); return t < 0.25f ? 4.0f * t : (t < 0.75f ? 2.0f - 4.0f * t : 4.0f * t - 4.0f); }
float sqrT (float t) { return frac (t) < 0.5f ? 1.0f : -1.0f; }
} // namespace

//==============================================================================
WavetableData::Ptr WavetableData::fromFrames (const std::vector<float>& frames, int numFrames, const juce::String& name)
{
    std::vector<Spectrum> specs;
    for (int f = 0; f < numFrames; ++f)
        specs.push_back (toSpectrum (frames.data() + (size_t) f * N));
    return buildFromSpectra (specs, name, frames);
}

FactoryBank::FactoryBank()
{
    constexpr int F = 32;

    // 0. Basic Shapes : sinus -> triangle -> dent de scie -> carré
    tables.add (fromTimeFn ("Basic Shapes", F, [] (float t, float x)
    {
        const float s = std::sin (kTwoPi * t), tr = triT (t), sw = sawT (t + 0.5f), sq = sqrT (t);
        const float seg = x * 3.0f;
        if (seg < 1.0f) return lerp (s, tr, seg);
        if (seg < 2.0f) return lerp (tr, sw, seg - 1.0f);
        return lerp (sw, sq, std::min (1.0f, seg - 2.0f));
    }));

    // 1. PWM Sweep
    tables.add (fromTimeFn ("PWM Sweep", F, [] (float t, float x)
    {
        const float w = 0.5f - x * 0.47f;
        return t < w ? 1.0f : -1.0f;
    }));

    // 2. Razor Saw : dent de scie avec résonance balayée
    tables.add (fromAdditive ("Razor Saw", F, [] (float x)
    {
        const float c = 3.0f + x * 45.0f, w = 2.0f + 6.0f * x;
        return additive ([=] (int k)
        {
            const float d = ((float) k - c) / w;
            return (1.0f / (float) k) * (1.0f + 4.0f * x * std::exp (-0.5f * d * d));
        });
    }));

    // 3. Vox Formant : A - E - I - O - U
    tables.add (fromAdditive ("Vox Formant", F, [] (float x)
    {
        static const float V[5][3] = { { 800, 1150, 2900 }, { 530, 1840, 2480 }, { 300, 2300, 3000 },
                                       { 450, 800, 2830 }, { 325, 700, 2530 } };
        const float p = x * 4.0f;
        const int i0 = std::min (3, (int) p);
        const float fr = p - (float) i0;
        float F1[3];
        for (int j = 0; j < 3; ++j) F1[j] = lerp (V[i0][j], V[i0 + 1][j], fr);
        const float A[3] = { 1.0f, 0.55f, 0.3f }, BW[3] = { 110.0f, 140.0f, 200.0f };
        const float f0 = 110.0f;
        return additive ([=] (int k)
        {
            const float fk = (float) k * f0;
            float a = 0.02f / std::sqrt ((float) k);
            for (int j = 0; j < 3; ++j)
            {
                const float d = (fk - F1[j]) / BW[j];
                a += A[j] * std::exp (-0.5f * d * d);
            }
            return fk > 9000.0f ? 0.0f : a;
        });
    }));

    // 4. Funeral Organ : registrations flûte -> principal -> plein jeu -> anches
    tables.add (fromAdditive ("Funeral Organ", F, [] (float x)
    {
        static const int H[10] = { 1, 2, 3, 4, 5, 6, 8, 10, 12, 16 };
        static const float R[4][10] = {
            { 1.0f, 0.5f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            { 1.0f, 0.8f, 0.6f, 0.5f, 0.2f, 0.3f, 0.25f, 0.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 0.8f, 0.9f, 0.6f, 0.7f, 0.8f, 0.5f, 0.4f, 0.3f },
            { 1.0f, 0.3f, 0.9f, 0.3f, 0.8f, 0.2f, 0.35f, 0.4f, 0.3f, 0.45f } };
        const float p = x * 3.0f;
        const int i0 = std::min (2, (int) p);
        const float fr = p - (float) i0;
        return additive ([=] (int k)
        {
            for (int j = 0; j < 10; ++j)
                if (H[j] == k) return lerp (R[i0][j], R[i0 + 1][j], fr);
            return 0.0f;
        });
    }));

    // 5. Harmonic Rise : on ajoute les harmoniques une par une
    tables.add (fromAdditive ("Harmonic Rise", F, [] (float x)
    {
        const float n = 1.0f + x * 63.0f;
        return additive ([=] (int k)
        {
            const float fk = (float) k;
            if (fk <= std::floor (n)) return 1.0f / fk;
            if (k == (int) std::floor (n) + 1) return (n - std::floor (n)) / fk;
            return 0.0f;
        });
    }));

    // 6. FM Metal
    tables.add (fromTimeFn ("FM Metal", F, [] (float t, float x)
    {
        const float i1 = x * 5.0f, i2 = x * x * 3.0f;
        return std::sin (kTwoPi * t + i1 * std::sin (kTwoPi * 3.0f * t) + i2 * std::sin (kTwoPi * 7.0f * t));
    }));

    // 7. Sync Sweep
    tables.add (fromTimeFn ("Sync Sweep", F, [] (float t, float x)
    {
        const float r = 1.0f + x * 7.0f;
        return sawT (t * r) * (1.0f - 0.3f * t);
    }));

    // 8. Wavefold
    tables.add (fromTimeFn ("Wavefold", F, [] (float t, float x)
    {
        const float g = 1.0f + x * 7.0f;
        return std::sin (kPi * 0.5f * g * std::sin (kTwoPi * t));
    }));

    // 9. Hollow Comb
    tables.add (fromAdditive ("Hollow Comb", F, [] (float x)
    {
        return additive ([=] (int k)
        {
            const float fk = (float) k;
            float a = (k % 2 == 1) ? 1.0f / fk : x / fk;
            a *= 0.15f + 0.85f * std::abs (std::cos (kPi * fk / (4.0f + x * 12.0f)));
            return a;
        });
    }));

    // 10. Grim Fog : spectres aléatoires qui se transforment (textures de drone)
    {
        Rng rng; rng.seed (666);
        Spectrum S[3];
        for (auto& s : S)
        {
            s.assign ((size_t) kHarm + 1, Cpx());
            for (int k = 1; k < 400; ++k)
                s[(size_t) k] = std::polar (rng.uni() * std::pow ((float) k, -0.9f), rng.uni() * kTwoPi);
        }
        tables.add (fromAdditive ("Grim Fog", F, [S] (float x)
        {
            const float p = x * 2.0f;
            const int i0 = std::min (1, (int) p);
            const float fr = p - (float) i0;
            Spectrum out ((size_t) kHarm + 1);
            for (size_t k = 0; k < out.size(); ++k) out[k] = S[i0][k] * (1.0f - fr) + S[i0 + 1][k] * fr;
            return out;
        }));
    }

    // 11. Digital Grit : bitcrush + réduction d'échantillonnage
    tables.add (fromTimeFn ("Digital Grit", F, [] (float t, float x)
    {
        const float hold = 1.0f + std::floor (x * 48.0f);
        const float th = std::floor (t * (float) N / hold) * hold / (float) N;
        const float levels = std::pow (2.0f, 8.0f - x * 6.0f);
        const float v = lerp (sawT (th + 0.5f), sqrT (th), x * 0.5f);
        return std::round (v * levels) / levels;
    }));
}

//==============================================================================
WavetableData::Ptr loadWavetableFile (const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr) { error = "Format de fichier non reconnu."; return nullptr; }

    const auto len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) 256 * N);
    if (len < 32) { error = "Fichier trop court."; return nullptr; }

    juce::AudioBuffer<float> buf ((int) std::max (1u, reader->numChannels), len);
    reader->read (&buf, 0, len, 0, true, true);

    std::vector<float> mono ((size_t) len);
    for (int c = 0; c < buf.getNumChannels(); ++c)
        for (int i = 0; i < len; ++i)
            mono[(size_t) i] += buf.getSample (c, i) / (float) buf.getNumChannels();

    std::vector<float> frames;
    int numFrames = 0;
    if (len < N)
    {
        // un seul cycle de taille arbitraire : on rééchantillonne à 2048
        frames.resize ((size_t) N);
        for (int i = 0; i < N; ++i)
        {
            const float p = (float) i * (float) len / (float) N;
            const int i0 = (int) p;
            const int i1 = (i0 + 1) % len;
            frames[(size_t) i] = lerp (mono[(size_t) i0], mono[(size_t) i1], p - (float) i0);
        }
        numFrames = 1;
    }
    else
    {
        numFrames = len / N;
        frames.assign (mono.begin(), mono.begin() + (size_t) numFrames * N);
    }
    return WavetableData::fromFrames (frames, numFrames, file.getFileNameWithoutExtension());
}
} // namespace nox
