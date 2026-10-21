#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace nox
{
// Une wavetable = N frames de 2048 échantillons, chacune stockée en 11 niveaux de
// mipmap à bande limitée (1024, 512, ... 1 harmonique) pour un rendu sans aliasing.
struct WavetableData : public juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<WavetableData>;
    static constexpr int N = 2048;
    static constexpr int kLevels = 11;

    juce::String name;
    int numFrames = 0;
    std::vector<float> data;                       // [frame][level][N + 1]
    std::vector<std::vector<float>> display;       // aperçu pour l'UI (<= 64 frames x 256 points)
    std::vector<float> rawFrames;                  // frames d'origine (pour sauvegarde des tables user)

    const float* get (int frame, int level) const noexcept
    {
        return data.data() + ((size_t) frame * kLevels + (size_t) level) * (N + 1);
    }

    static Ptr fromFrames (const std::vector<float>& frames, int numFrames, const juce::String& name);
};

// Banque d'usine générée procéduralement (partagée entre instances)
struct FactoryBank
{
    FactoryBank();
    juce::ReferenceCountedArray<WavetableData> tables;
};

// Charge un .wav/.aif/.flac au format "Serum" (frames de 2048 échantillons)
WavetableData::Ptr loadWavetableFile (const juce::File& file, juce::String& error);
} // namespace nox
