# NoxWave — synthé wavetable, édition black metal

Synthétiseur wavetable (VST3 / AU / Standalone) écrit en C++ avec JUCE. Il reprend l'essentiel du
flux de travail d'un synthé type Serum — deux oscillateurs wavetable avec warp et unison, filtre,
enveloppes, LFO, matrice de modulation, macros, rack d'effets — avec des choix orientés black metal,
dungeon synth et ambient.

Projet indépendant : aucune ressource, aucun code et aucune wavetable de Serum ne sont utilisés.
Les wavetables d'usine sont générées mathématiquement au lancement.

## Fonctionnalités

**Oscillateurs A et B**
- 12 wavetables d'usine (32 frames chacune) : Basic Shapes, PWM Sweep, Razor Saw, Vox Formant (voyelles A-E-I-O-U),
  Funeral Organ, Harmonic Rise, FM Metal, Sync Sweep, Wavefold, Hollow Comb, Grim Fog, Digital Grit
- **Import de wavetables .wav** (format Serum : frames de 2048 échantillons, jusqu'à 256 frames) — bouton `WAV...`.
  La table importée est sauvegardée dans le projet/preset.
- Rendu anti-aliasing par mipmaps FFT (11 niveaux par frame), interpolation entre frames
- 8 modes de warp : Sync, Bend +, Bend −, PWM, Mirror, FM (depuis l'autre oscillateur), Bitcrush
- Unison jusqu'à 16 voix par oscillateur : Detune, Blend, Width, phase aléatoire
- Octave / demi-ton / cents, niveau, pan

**Sub & Noise** : sub-oscillateur (sinus, triangle, carré, scie, −1 à −3 octaves) et bruit stéréo à couleur réglable.

**Filtre** (par voix, stéréo) : LP12, LP24, HP12, HP24, BP, Notch (SVF zéro-délai), Ladder LP et Ladder Dirty (saturé),
drive, quantité d'enveloppe 2, keytracking, mix.

**Modulation**
- 3 enveloppes ADSR (Env 1 = ampli)
- 4 LFO : 7 formes, synchro tempo (4/1 à 1/64, pointées, triolets), retrig ou libre
- Matrice de 12 slots — sources : Env 2/3, LFO 1-4, vélocité, mod wheel, aftertouch, note, macros 1-4, aléatoire, pitch bend ;
  22 destinations, dont 4 destinations d'effets globales
- 4 macros pré-câblées (slots 9-12) : **Bright** (cutoff), **Grim** (distorsion), **Abyss** (réverbe), **Morph** (position A)

**Voix** : 16 voix de polyphonie, modes Poly / Mono / Legato, glide, pitch bend réglable, sensibilité à la vélocité, pédale de sustain.

**Effets** (dans l'ordre) : Distorsion suréchantillonnée ×2 (Tube, Hard Clip, Fuzz, Foldback, Bitcrush, Rectify) →
EQ 3 bandes → **Tremolo synchronisé** (forme « Chop » qui imite le tremolo picking des guitares) → Chorus →
Delay synchro (ping-pong) → Réverbe FDN 8 lignes (jusqu'à 30 s de decay).

**Presets d'usine** : Frostbitten Pad, Tremolo Wraith, Grim Choir, Funeral Organ, Necro Drone, Blizzard Wind,
Cold Bass, Abyss Bell, Raw Buzzsaw, Dungeon Synth, Celestial Sorrow, Black Throne Lead.
Sauvegarde / chargement de presets perso (`.noxpreset`, dans *Documents/NoxWave Presets*).

## Obtenir le plugin

### Option 1 — sans rien installer (GitHub)
1. Crée un dépôt GitHub et pousses-y ce dossier (sans le dossier `build/`).
2. Onglet **Actions** → le workflow *Build NoxWave* se lance tout seul (≈ 10 min).
3. Télécharge l'artefact `NoxWave-Windows`, `NoxWave-macOS` ou `NoxWave-Linux`.

### Option 2 — compiler sur ton ordinateur
Prérequis : CMake ≥ 3.22, Git, et un compilateur :
- **Windows** : Visual Studio 2022 (charge de travail « Développement desktop en C++ »)
- **macOS** : Xcode (ou les Command Line Tools)
- **Linux** : `sudo apt install build-essential cmake libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev libgl1-mesa-dev`

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
JUCE est téléchargé automatiquement au premier configure. Résultats dans `build/NoxWave_artefacts/Release/`.

### Installation
- **Windows** : copier `NoxWave.vst3` dans `C:\Program Files\Common Files\VST3\`
- **macOS** : `NoxWave.vst3` → `~/Library/Audio/Plug-Ins/VST3/`, `NoxWave.component` → `~/Library/Audio/Plug-Ins/Components/`.
  Les binaires non signés peuvent être bloqués par Gatekeeper : `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/NoxWave.vst3`
- **Linux** : `~/.vst3/`

Relance un scan des plugins dans ton DAW. La version **Standalone** fonctionne seule avec le clavier à l'écran.

## Conseils black metal

- **Tremolo picking** : n'importe quel son + FX *Tremolo* en forme *Chop*, division 1/16 ou 1/32 selon le tempo.
  Le preset *Tremolo Wraith* est un point de départ. Lance le transport du DAW pour une synchro parfaite à la grille.
- **Mur de guitares** : *Raw Buzzsaw* en accords de quinte (sans tierce) une octave en dessous de la ligne.
- **Nappes glaciales** : *Frostbitten Pad* / *Celestial Sorrow* — monte la macro **Abyss** pour noyer dans la réverbe.
- **Chœurs** : *Grim Choir* — la macro **Morph** change la voyelle.
- **Intros dungeon synth** : *Dungeon Synth* + *Funeral Organ* en couches.
- **Atmosphère** : *Blizzard Wind* tenu sur une note grave, *Necro Drone* en pédale sous tout le morceau.

## Structure du code

| Fichier | Rôle |
|---|---|
| `Source/Params.*` | Définition de tous les paramètres (automatisables) |
| `Source/Wavetable.*` | Génération des tables d'usine, mipmaps FFT, import WAV |
| `Source/Dsp.h` | Enveloppes, LFO, filtres SVF / Ladder / biquad |
| `Source/Engine.*` | Voix, allocation, unison, warp, matrice de modulation |
| `Source/Fx.h` | Distorsion, EQ, tremolo, chorus, delay, réverbe FDN |
| `Source/Presets.h` | Presets d'usine (faciles à éditer) |
| `Source/PluginEditor.*` | Interface (redimensionnable) |
| `Tests/RenderTest.cpp` | Rendu hors-ligne de chaque preset + tests (`-DNOX_BUILD_TESTS=ON`) |

## Différences avec Serum 2
Non inclus : éditeur graphique de wavetables, oscillateurs granulaires / spectraux / multi-sample, LFO dessinables,
modulation par glisser-déposer, effets réordonnables, arpégiateur / séquenceur de clips.
