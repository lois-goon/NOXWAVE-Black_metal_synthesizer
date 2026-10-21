#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace nox
{
namespace col
{
    const juce::Colour bg       { 0xff09090b };
    const juce::Colour panel    { 0xff121216 };
    const juce::Colour panelHi  { 0xff1a1a20 };
    const juce::Colour line     { 0xff2a2a33 };
    const juce::Colour text     { 0xffd9d6cf };   // os blanchi
    const juce::Colour dim      { 0xff7b7a80 };
    const juce::Colour ice      { 0xff9cc6e4 };   // bleu glacial
    const juce::Colour blood    { 0xffb3212b };   // rouge sang
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.5f)); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (12.0f, juce::Font::bold)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (13.0f)); }
};

class WavetableView : public juce::Component, private juce::Timer
{
public:
    WavetableView (NoxWaveProcessor& p, int oscIndex);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    NoxWaveProcessor& proc;
    int osc;
    float lastPos = -2.0f;
    const void* lastTable = nullptr;
};

class EnvView : public juce::Component
{
public:
    EnvView (NoxWaveProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    int env = 0;
private:
    NoxWaveProcessor& proc;
};

class LfoView : public juce::Component
{
public:
    LfoView (NoxWaveProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    int lfo = 0;
private:
    NoxWaveProcessor& proc;
};
} // namespace nox

//==============================================================================
class NoxWaveEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoxWaveEditor (NoxWaveProcessor&);
    ~NoxWaveEditor() override;
    void resized() override;
    void setTab (int t);

    static constexpr int baseW = 1180, baseH = 836;

private:
    struct Canvas : public juce::Component
    {
        std::function<void (juce::Graphics&)> painter;
        void paint (juce::Graphics& g) override { if (painter) painter (g); }
    };
    struct Entry { juce::Component* c; int tab; int env; int lfo; };
    struct Panel { juce::Rectangle<int> r; juce::String title; int tab; };

    void timerCallback() override;
    void paintContent (juce::Graphics&);
    void updateVisibility();
    void buildHeader();
    void buildOscTab();
    void buildFxTab();
    void buildMatrixTab();

    juce::Slider* knob (const juce::String& id, const juce::String& label, juce::Rectangle<int> r,
                        int tab, int env = -1, int lfo = -1);
    juce::ComboBox* combo (const juce::String& id, juce::Rectangle<int> r, int tab, int env = -1, int lfo = -1);
    juce::ToggleButton* toggle (const juce::String& id, const juce::String& text, juce::Rectangle<int> r,
                                int tab, int env = -1, int lfo = -1);
    template <typename T> T* own (std::unique_ptr<T> c, int tab, int env = -1, int lfo = -1);
    void panel (juce::Rectangle<int> r, const juce::String& title, int tab) { panels.push_back ({ r, title, tab }); }

    NoxWaveProcessor& proc;
    nox::LookAndFeel laf;
    Canvas content;
    juce::MidiKeyboardComponent keyboard;

    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<Entry> entries;
    std::vector<Panel> panels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;

    juce::TextButton tabButtons[3];
    juce::TextButton envButtons[3], lfoButtons[4];
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, loadBtn { "LOAD" };
    juce::Label voiceLabel;
    nox::EnvView* envView = nullptr;
    nox::LfoView* lfoView = nullptr;
    std::unique_ptr<juce::FileChooser> chooser;

    int tab = 0, envSel = 0, lfoSel = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoxWaveEditor)
};
