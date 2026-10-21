// Outil de test hors-ligne : rend chaque preset en WAV, vérifie NaN / clipping / silence,
// mesure le coût CPU, et peut capturer l'interface en PNG.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& buf, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (w) { os.release(); w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples()); }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : "renders");
    outDir.createDirectory();
    const double sr = 48000.0;
    const int block = 256;

    NoxWaveProcessor p;
    p.setPlayConfigDetails (0, 2, sr, block);
    p.prepareToPlay (sr, block);

    // Interface -> PNG pour chaque onglet
    if (argc > 2 && juce::String (argv[2]) == "--snapshot")
    {
        p.setCurrentProgram (1);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (NoxWaveEditor::baseW, NoxWaveEditor::baseH);
        const char* tabs[3] = { "OSC", "FX", "MATRIX" };
        for (int t = 0; t < 3; ++t)
        {
            if (auto* ne = dynamic_cast<NoxWaveEditor*> (ed.get())) ne->setTab (t);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds());
            juce::PNGImageFormat png;
            auto f = outDir.getChildFile (juce::String ("ui_") + tabs[t] + ".png");
            f.deleteFile();
            juce::FileOutputStream fos (f);
            png.writeImageToStream (img, fos);
        }
        ed = nullptr;
    }

    bool allOk = true;
    const int numPresets = p.getNumPrograms();
    for (int pr = 0; pr < numPresets; ++pr)
    {
        p.setCurrentProgram (pr);
        p.prepareToPlay (sr, block);   // reset propre entre presets

        const double seconds = 7.0;
        const int total = (int) (seconds * sr);
        juce::AudioBuffer<float> out (2, total);
        juce::AudioBuffer<float> buf (2, block);

        // Progression mi mineur (E - C - D - Em) jouée en accords ; mono : ligne mélodique
        const bool mono = p.apvts.getRawParameterValue ("voice_mode")->load() > 0.5f;
        struct Ev { double t; int note; bool on; };
        std::vector<Ev> evs;
        const int chords[4][4] = { { 40, 52, 55, 59 }, { 36, 48, 52, 55 }, { 38, 50, 54, 57 }, { 40, 52, 55, 59 } };
        if (mono)
        {
            const int line[8] = { 40, 43, 47, 52, 50, 47, 43, 45 };
            for (int i = 0; i < 8; ++i) { evs.push_back ({ i * 0.6, line[i] + 12, true }); evs.push_back ({ i * 0.6 + 0.65, line[i] + 12, false }); }
        }
        else
            for (int c = 0; c < 4; ++c)
                for (int n : chords[c]) { evs.push_back ({ c * 1.25, n + 12, true }); evs.push_back ({ c * 1.25 + 1.2, n + 12, false }); }

        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            juce::MidiBuffer midi;
            for (auto& e : evs)
            {
                const int s = (int) (e.t * sr) - pos;
                if (s >= 0 && s < n)
                    midi.addEvent (e.on ? juce::MidiMessage::noteOn (1, e.note, (juce::uint8) 100)
                                        : juce::MidiMessage::noteOff (1, e.note), s);
            }
            buf.setSize (2, n, false, false, true);
            p.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, n);
        }
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;

        float peak = 0.0f; double sumSq = 0.0; int bad = 0, clipped = 0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < total; ++i)
            {
                const float v = out.getSample (ch, i);
                if (! std::isfinite (v)) { ++bad; continue; }
                peak = std::max (peak, std::abs (v));
                if (std::abs (v) > 0.999f) ++clipped;
                sumSq += (double) v * v;
            }
        const double rmsDb = 10.0 * std::log10 (sumSq / (2.0 * total) + 1e-12);
        const bool ok = bad == 0 && peak > 0.01f && peak <= 1.0f;
        allOk = allOk && ok;
        std::printf ("%-20s peak %5.2f  rms %6.1f dB  clip %6d  NaN %d  CPU %5.1f%% temps reel  %s\n",
                     p.getProgramName (pr).toRawUTF8(), peak, rmsDb, clipped, bad, 100.0 * ms / (seconds * 1000.0), ok ? "OK" : "FAIL");
        writeWav (outDir.getChildFile (juce::String (pr).paddedLeft ('0', 2) + "_" + p.getProgramName (pr).replaceCharacter (' ', '_') + ".wav"), out, sr);
    }

    // Test d'aller-retour de l'état
    juce::MemoryBlock mb;
    p.setCurrentProgram (5);
    p.getStateInformation (mb);
    p.setCurrentProgram (0);
    p.setStateInformation (mb.getData(), (int) mb.getSize());
    const bool stateOk = std::abs (p.apvts.getRawParameterValue ("A_unison")->load() - 8.0f) < 0.01f;
    std::printf ("State save/restore : %s\n", stateOk ? "OK" : "FAIL");

    // Test import de wavetable utilisateur (fichier généré : 16 frames)
    {
        auto f = outDir.getChildFile ("test_table.wav");
        juce::AudioBuffer<float> t (2, 16 * 2048);
        for (int fr = 0; fr < 16; ++fr)
            for (int i = 0; i < 2048; ++i)
            {
                const float ph = (float) i / 2048.0f;
                const float v = std::sin (6.2831853f * ph) * (1.0f - fr / 16.0f) + (2.0f * ph - 1.0f) * fr / 16.0f;
                t.setSample (0, fr * 2048 + i, v); t.setSample (1, fr * 2048 + i, v);
            }
        writeWav (f, t, 48000.0);
        const auto err = p.loadUserTable (0, f);
        const auto* dt = p.getDisplayTable (0);
        const bool wtOk = err.isEmpty() && dt != nullptr && dt->numFrames == 16;
        std::printf ("Import wavetable WAV : %s %s\n", wtOk ? "OK" : "FAIL", err.toRawUTF8());
        allOk = allOk && wtOk;
    }

    return allOk && stateOk ? 0 : 1;
}
