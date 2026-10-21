#include "PluginEditor.h"
#include "Presets.h"

using namespace nox;

//==============================================================================
// LookAndFeel
//==============================================================================
nox::LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
    setColour (juce::ComboBox::backgroundColourId, col::panelHi);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::arrowColourId, col::ice);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141418));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::blood.withAlpha (0.6f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::TextButton::buttonColourId, col::panelHi);
    setColour (juce::TextButton::buttonOnColourId, col::blood);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ToggleButton::textColourId, col::text);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee101014));
    setColour (juce::BubbleComponent::outlineColourId, col::ice.withAlpha (0.6f));
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::AlertWindow::backgroundColourId, col::panel);
    setColour (juce::AlertWindow::textColourId, col::text);
}

void nox::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float start, float end, juce::Slider& s)
{
    const float r = (float) juce::jmin (w, h) * 0.5f - 4.0f;
    const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float angle = start + pos * (end - start);
    const float from = bipolar ? (start + end) * 0.5f : start;

    juce::Path track;
    track.addCentredArc (cx, cy, r, r, 0.0f, start, end, true);
    g.setColour (col::line);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (cx, cy, r, r, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    const auto accent = s.isMouseOverOrDragging() ? col::ice.brighter (0.3f) : col::ice;
    g.setColour (accent);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = r * 0.68f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a31), cx, cy - br, juce::Colour (0xff0f0f12), cx, cy + br, false));
    g.fillEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f);
    g.setColour (col::line.brighter (0.2f));
    g.drawEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f, 1.0f);

    juce::Path ptr;
    ptr.addRoundedRectangle (-1.2f, -br + 2.0f, 2.4f, br * 0.55f, 1.0f);
    g.setColour (col::text);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

void nox::LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cy = (float) y + (float) h * 0.5f;
    g.setColour (col::line);
    g.fillRoundedRectangle ((float) x, cy - 2.0f, (float) w, 4.0f, 2.0f);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zero = bipolar ? (float) x + (float) w * 0.5f : (float) x;
    const float a = juce::jmin (zero, pos), b = juce::jmax (zero, pos);
    const auto c = s.getValue() < 0.0 ? col::blood.brighter (0.2f) : col::ice;
    g.setColour (c);
    g.fillRoundedRectangle (a, cy - 2.0f, b - a, 4.0f, 2.0f);
    g.setColour (col::text);
    g.fillEllipse (pos - 5.0f, cy - 5.0f, 10.0f, 10.0f);
}

void nox::LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hi, bool)
{
    const auto on = b.getToggleState();
    const float s = 12.0f;
    const float yy = ((float) b.getHeight() - s) * 0.5f;
    g.setColour (on ? col::blood : col::panelHi);
    g.fillRoundedRectangle (2.0f, yy, s, s, 2.5f);
    g.setColour (on ? col::blood.brighter (0.5f) : (hi ? col::ice : col::line.brighter (0.3f)));
    g.drawRoundedRectangle (2.0f, yy, s, s, 2.5f, 1.0f);
    g.setColour (on ? col::text : col::dim);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (b.getButtonText(), 20, 0, b.getWidth() - 20, b.getHeight(), juce::Justification::centredLeft);
}

void nox::LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? col::blood.withAlpha (0.85f) : (down ? col::line : (hi ? col::panelHi.brighter (0.15f) : col::panelHi)));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (on ? col::blood.brighter (0.4f) : col::line);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

void nox::LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::panelHi);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.isMouseOver (true) ? col::ice.withAlpha (0.6f) : col::line);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    juce::Path p;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    p.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (col::ice);
    g.fillPath (p);
}

//==============================================================================
// Vues
//==============================================================================
WavetableView::WavetableView (NoxWaveProcessor& p, int o) : proc (p), osc (o) { startTimerHz (30); }

void WavetableView::timerCallback()
{
    const auto* t = proc.getDisplayTable (osc);
    float pos = proc.getDisplayPos (osc);
    if (pos < 0.0f) pos = proc.apvts.getRawParameterValue (oscId (osc, "pos"))->load();
    if (std::abs (pos - lastPos) > 0.001f || t != lastTable) { lastPos = pos; lastTable = t; repaint(); }
}

void WavetableView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0b0b0e));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);

    const auto* t = proc.getDisplayTable (osc);
    if (t == nullptr || t->display.empty()) return;
    const bool on = proc.apvts.getRawParameterValue (oscId (osc, "on"))->load() > 0.5f;
    const float pos = juce::jlimit (0.0f, 1.0f, lastPos);
    const int nf = (int) t->display.size();

    // pile 3D des frames
    const float w = b.getWidth() * 0.78f, h = b.getHeight() * 0.42f;
    const float dx = (b.getWidth() - w - 16.0f) / (float) juce::jmax (1, nf - 1);
    const float dy = (b.getHeight() - h - 24.0f) / (float) juce::jmax (1, nf - 1);
    const int step = juce::jmax (1, nf / 24);
    for (int f = nf - 1; f >= 0; f -= step)
    {
        const auto& pts = t->display[(size_t) f];
        const float ox = 8.0f + dx * (float) f, oy = b.getHeight() - 12.0f - h * 0.5f - dy * (float) f;
        juce::Path p;
        for (size_t i = 0; i < pts.size(); ++i)
        {
            const float px = ox + w * (float) i / (float) (pts.size() - 1);
            const float py = oy - pts[i] * h * 0.45f;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        g.setColour (col::ice.withAlpha (0.08f + 0.1f * (1.0f - (float) f / (float) nf)));
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }

    // frame courante (interpolée), mise en avant
    const float fp = pos * (float) (nf - 1);
    const int f0 = juce::jmin ((int) fp, nf - 1), f1 = juce::jmin (f0 + 1, nf - 1);
    const float fr = fp - (float) f0;
    const float ox = 8.0f + dx * fp, oy = b.getHeight() - 12.0f - h * 0.5f - dy * fp;
    juce::Path cur, fill;
    const auto& A = t->display[(size_t) f0];
    const auto& B = t->display[(size_t) f1];
    for (size_t i = 0; i < A.size(); ++i)
    {
        const float v = A[i] + fr * (B[i] - A[i]);
        const float px = ox + w * (float) i / (float) (A.size() - 1);
        const float py = oy - v * h * 0.45f;
        if (i == 0) { cur.startNewSubPath (px, py); fill.startNewSubPath (px, oy); fill.lineTo (px, py); }
        else { cur.lineTo (px, py); fill.lineTo (px, py); }
    }
    fill.lineTo (ox + w, oy);
    fill.closeSubPath();
    const auto c = on ? col::ice : col::dim;
    g.setColour (c.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (c);
    g.strokePath (cur, juce::PathStrokeType (2.0f));

    g.setColour (col::dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (t->name + "  ·  frame " + juce::String (juce::roundToInt (fp) + 1) + "/" + juce::String (t->numFrames),
                getLocalBounds().reduced (8, 4), juce::Justification::topRight);
}

void EnvView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0b0b0e));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);

    auto v = [this] (const char* k) { return proc.apvts.getRawParameterValue (envId (env, k))->load(); };
    const float a = v ("a"), d = v ("d"), s = v ("s"), r = v ("r");
    auto tx = [] (float t) { return std::sqrt (t / 20.0f); };   // échelle compressée
    const float ta = tx (a), td = tx (d), tr = tx (r), hold = 0.25f;
    const float total = ta + td + hold + tr;
    const auto area = b.reduced (8.0f, 10.0f);
    auto X = [&] (float t) { return area.getX() + area.getWidth() * t / total; };
    auto Y = [&] (float lv) { return area.getBottom() - area.getHeight() * lv; };

    juce::Path p;
    p.startNewSubPath (X (0), Y (0));
    p.lineTo (X (ta), Y (1));
    p.quadraticTo (X (ta + td * 0.25f), Y (s), X (ta + td), Y (s));
    p.lineTo (X (ta + td + hold), Y (s));
    p.quadraticTo (X (ta + td + hold + tr * 0.2f), Y (0), X (total), Y (0));
    juce::Path f (p);
    f.lineTo (X (0), Y (0));
    f.closeSubPath();
    g.setColour (col::ice.withAlpha (0.12f));
    g.fillPath (f);
    g.setColour (col::ice);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

void LfoView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0b0b0e));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);

    const int shape = (int) proc.apvts.getRawParameterValue (lfoId (lfo, "shape"))->load();
    Lfo l;
    l.reset (0.0f, 77u);
    const auto area = b.reduced (8.0f, 12.0f);
    g.setColour (col::line);
    g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());
    juce::Path p;
    const int N = 200;
    for (int i = 0; i <= N; ++i)
    {
        const float t = (float) i / (float) N;
        l.phase = 0.0f;
        Lfo tmp = l;
        tmp.advance (t * 2.0f);   // deux cycles
        const float vv = tmp.value (shape);
        const float x = area.getX() + area.getWidth() * t;
        const float y = area.getCentreY() - vv * area.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::ice);
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

//==============================================================================
// Editor
//==============================================================================
namespace
{
struct KnobComp : public juce::Component
{
    juce::Slider slider;
    juce::Label label;
    KnobComp (const juce::String& text)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (slider);
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, col::dim);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromBottom (15));
        slider.setBounds (r);
    }
};
} // namespace

template <typename T>
T* NoxWaveEditor::own (std::unique_ptr<T> c, int t, int env, int lfo)
{
    auto* raw = c.get();
    content.addAndMakeVisible (*raw);
    entries.push_back ({ raw, t, env, lfo });
    owned.push_back (std::move (c));
    return raw;
}

juce::Slider* NoxWaveEditor::knob (const juce::String& id, const juce::String& label, juce::Rectangle<int> r,
                                   int t, int env, int lfo)
{
    auto* k = own (std::make_unique<KnobComp> (label), t, env, lfo);
    k->setBounds (r);
    k->slider.setPopupDisplayEnabled (true, true, &content);
    sliderAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider));
    if (auto* p = proc.apvts.getParameter (id))
        k->slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    return &k->slider;
}

juce::ComboBox* NoxWaveEditor::combo (const juce::String& id, juce::Rectangle<int> r, int t, int env, int lfo)
{
    auto* c = own (std::make_unique<juce::ComboBox>(), t, env, lfo);
    c->setBounds (r);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c->addItemList (p->choices, 1);
    comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *c));
    return c;
}

juce::ToggleButton* NoxWaveEditor::toggle (const juce::String& id, const juce::String& text, juce::Rectangle<int> r,
                                           int t, int env, int lfo)
{
    auto* b = own (std::make_unique<juce::ToggleButton> (text), t, env, lfo);
    b->setBounds (r);
    buttonAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b));
    return b;
}

NoxWaveEditor::NoxWaveEditor (NoxWaveProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&laf);
    content.painter = [this] (juce::Graphics& g) { paintContent (g); };
    content.setBounds (0, 0, baseW, baseH);
    addAndMakeVisible (content);

    buildHeader();
    buildOscTab();
    buildFxTab();
    buildMatrixTab();

    keyboard.setBounds (12, 768, baseW - 24, 60);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9d6cf));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff111114));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, col::blood.withAlpha (0.8f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::ice.withAlpha (0.4f));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff444448));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, col::panelHi);
    keyboard.setLowestVisibleKey (24);
    keyboard.setKeyWidth (16.0f);
    content.addAndMakeVisible (keyboard);

    setTab (0);
    timerCallback();

    setResizable (true, true);
    setResizeLimits ((int) (baseW * 0.55), (int) (baseH * 0.55), (int) (baseW * 1.6), (int) (baseH * 1.6));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize ((int) (baseW * 0.9), (int) (baseH * 0.9));
    startTimerHz (15);
}

NoxWaveEditor::~NoxWaveEditor()
{
    stopTimer();
    sliderAtt.clear(); comboAtt.clear(); buttonAtt.clear();
    setLookAndFeel (nullptr);
}

void NoxWaveEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
}

//------------------------------------------------------------------------------
void NoxWaveEditor::buildHeader()
{
    const char* names[3] = { "OSC", "FX", "MATRIX" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = tabButtons[i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (false);
        b.setBounds (262 + i * 84, 18, 80, 26);
        b.onClick = [this, i] { setTab (i); };
        content.addAndMakeVisible (b);
    }

    const auto& presets = factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);
    presetBox.setTextWhenNothingSelected ("Preset");
    presetBox.setBounds (550, 18, 220, 26);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id - 1 != proc.getCurrentProgram()) proc.setCurrentProgram (id - 1);
        else if (id > 0 && presetBox.getText() != proc.presetName) proc.setCurrentProgram (id - 1);
    };
    content.addAndMakeVisible (presetBox);

    prevBtn.setBounds (522, 18, 26, 26);
    nextBtn.setBounds (772, 18, 26, 26);
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % n); };
    content.addAndMakeVisible (prevBtn);
    content.addAndMakeVisible (nextBtn);

    saveBtn.setBounds (806, 18, 56, 26);
    loadBtn.setBounds (866, 18, 56, 26);
    saveBtn.onClick = [this]
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("NoxWave Presets");
        dir.createDirectory();
        chooser = std::make_unique<juce::FileChooser> ("Sauver le preset", dir.getChildFile (proc.presetName + ".noxpreset"), "*.noxpreset");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  f = f.withFileExtension ("noxpreset");
                                  proc.presetName = f.getFileNameWithoutExtension();
                                  juce::MemoryBlock mb;
                                  proc.getStateInformation (mb);
                                  f.replaceWithData (mb.getData(), mb.getSize());
                              });
    };
    loadBtn.onClick = [this]
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("NoxWave Presets");
        chooser = std::make_unique<juce::FileChooser> ("Charger un preset", dir, "*.noxpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (! f.existsAsFile()) return;
                                  juce::MemoryBlock mb;
                                  if (f.loadFileAsData (mb))
                                  {
                                      proc.setStateInformation (mb.getData(), (int) mb.getSize());
                                      proc.presetName = f.getFileNameWithoutExtension();
                                  }
                              });
    };
    content.addAndMakeVisible (saveBtn);
    content.addAndMakeVisible (loadBtn);

    voiceLabel.setBounds (940, 18, 130, 26);
    voiceLabel.setFont (juce::FontOptions (11.0f));
    voiceLabel.setColour (juce::Label::textColourId, col::dim);
    voiceLabel.setJustificationType (juce::Justification::centredRight);
    content.addAndMakeVisible (voiceLabel);

    // Master dans l'en-tête (visible sur tous les onglets)
    knob ("master", "MASTER", { 1092, 4, 70, 60 }, -1);
}

void NoxWaveEditor::buildOscTab()
{
    constexpr int T = 0;
    for (int o = 0; o < 2; ++o)
    {
        const juce::Rectangle<int> r (12 + o * 584, 72, 572, 322);
        panel (r, {}, T);
        toggle (oscId (o, "on"), o == 0 ? "OSC A" : "OSC B", { r.getX() + 10, r.getY() + 6, 76, 22 }, T);
        combo (oscId (o, "table"), { r.getX() + 92, r.getY() + 6, 170, 22 }, T);

        auto* load = own (std::make_unique<juce::TextButton> ("WAV..."), T);
        load->setBounds (r.getX() + 266, r.getY() + 6, 58, 22);
        load->setTooltip (juce::String::fromUTF8 ("Importer une wavetable (.wav, frames de 2048 échantillons, format Serum)"));
        load->onClick = [this, o]
        {
            chooser = std::make_unique<juce::FileChooser> ("Importer une wavetable",
                                                           juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                           "*.wav;*.aif;*.aiff;*.flac");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this, o] (const juce::FileChooser& fc)
                                  {
                                      const auto f = fc.getResult();
                                      if (! f.existsAsFile()) return;
                                      const auto err = proc.loadUserTable (o, f);
                                      if (err.isNotEmpty())
                                          juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "NoxWave", err);
                                  });
        };

        auto* wl = own (std::make_unique<juce::Label> (juce::String(), "WARP"), T);
        wl->setFont (juce::FontOptions (11.0f, juce::Font::bold));
        wl->setColour (juce::Label::textColourId, col::dim);
        wl->setBounds (r.getRight() - 196, r.getY() + 6, 46, 22);
        combo (oscId (o, "warpMode"), { r.getRight() - 150, r.getY() + 6, 140, 22 }, T);

        auto* view = own (std::make_unique<WavetableView> (proc, o), T);
        view->setBounds (r.getX() + 10, r.getY() + 34, r.getWidth() - 20, 130);

        const int kw = 76, kh = 70;
        int x = r.getX() + 12, y = r.getY() + 170;
        const std::pair<const char*, const char*> row1[] = { { "pos", "WT POS" }, { "warp", "WARP" }, { "oct", "OCT" },
                                                             { "semi", "SEMI" }, { "fine", "FINE" }, { "level", "LEVEL" }, { "pan", "PAN" } };
        for (auto& [id, lb] : row1) { knob (oscId (o, id), lb, { x, y, kw, kh }, T); x += kw + 2; }
        x = r.getX() + 12; y += kh + 4;
        const std::pair<const char*, const char*> row2[] = { { "unison", "UNISON" }, { "detune", "DETUNE" }, { "blend", "BLEND" },
                                                             { "width", "WIDTH" }, { "rand", "PHASE RND" } };
        for (auto& [id, lb] : row2) { knob (oscId (o, id), lb, { x, y, kw, kh }, T); x += kw + 2; }
    }

    // Sub / Noise / Filter / Voice
    {
        const juce::Rectangle<int> r (12, 406, 220, 170);
        panel (r, {}, T);
        toggle ("sub_on", "SUB", { r.getX() + 10, r.getY() + 6, 70, 22 }, T);
        combo ("sub_shape", { r.getX() + 90, r.getY() + 6, 120, 22 }, T);
        knob ("sub_oct", "OCT", { r.getX() + 16, r.getY() + 46, 90, 100 }, T);
        knob ("sub_level", "LEVEL", { r.getX() + 114, r.getY() + 46, 90, 100 }, T);
    }
    {
        const juce::Rectangle<int> r (244, 406, 170, 170);
        panel (r, {}, T);
        toggle ("noise_on", "NOISE", { r.getX() + 10, r.getY() + 6, 90, 22 }, T);
        knob ("noise_level", "LEVEL", { r.getX() + 6, r.getY() + 46, 78, 100 }, T);
        knob ("noise_color", "COLOR", { r.getX() + 86, r.getY() + 46, 78, 100 }, T);
    }
    {
        const juce::Rectangle<int> r (426, 406, 470, 170);
        panel (r, {}, T);
        toggle ("flt_on", "FILTER", { r.getX() + 10, r.getY() + 6, 90, 22 }, T);
        combo ("flt_type", { r.getX() + 104, r.getY() + 6, 140, 22 }, T);
        const std::pair<const char*, const char*> ks[] = { { "flt_cutoff", "CUTOFF" }, { "flt_res", "RESO" }, { "flt_drive", "DRIVE" },
                                                           { "flt_env", "ENV 2" }, { "flt_key", "KEYTRK" }, { "flt_mix", "MIX" } };
        int x = r.getX() + 8;
        for (auto& [id, lb] : ks) { knob (id, lb, { x, r.getY() + 46, 74, 100 }, T); x += 76; }
    }
    {
        const juce::Rectangle<int> r (908, 406, 260, 170);
        panel (r, "VOICE", T);
        combo ("voice_mode", { r.getX() + 120, r.getY() + 6, 130, 22 }, T);
        knob ("glide", "GLIDE", { r.getX() + 8, r.getY() + 46, 80, 100 }, T);
        knob ("bend_range", "BEND", { r.getX() + 90, r.getY() + 46, 80, 100 }, T);
        knob ("amp_vel", "VELOCITY", { r.getX() + 172, r.getY() + 46, 80, 100 }, T);
    }

    // Enveloppes
    {
        const juce::Rectangle<int> r (12, 588, 400, 170);
        panel (r, "ENV", T);
        const char* names[3] = { "1 AMP", "2", "3" };
        for (int e = 0; e < 3; ++e)
        {
            auto& b = envButtons[e];
            b.setButtonText (names[e]);
            b.setBounds (r.getX() + 60 + e * 58, r.getY() + 6, 54, 22);
            b.onClick = [this, e] { envSel = e; envView->env = e; updateVisibility(); envView->repaint(); };
            content.addAndMakeVisible (b);
            entries.push_back ({ &b, T, -1, -1 });
            const char* k[4] = { "a", "d", "s", "r" };
            const char* lb[4] = { "ATTACK", "DECAY", "SUSTAIN", "RELEASE" };
            for (int i = 0; i < 4; ++i)
                knob (envId (e, k[i]), lb[i], { r.getX() + 168 + i * 57, r.getY() + 50, 56, 90 }, T, e);
        }
        envView = own (std::make_unique<EnvView> (proc), T);
        envView->setBounds (r.getX() + 10, r.getY() + 36, 150, 124);
    }
    // LFO
    {
        const juce::Rectangle<int> r (424, 588, 440, 170);
        panel (r, "LFO", T);
        for (int l = 0; l < kNumLfos; ++l)
        {
            auto& b = lfoButtons[l];
            b.setButtonText (juce::String (l + 1));
            b.setBounds (r.getX() + 56 + l * 40, r.getY() + 6, 36, 22);
            b.onClick = [this, l] { lfoSel = l; lfoView->lfo = l; updateVisibility(); lfoView->repaint(); };
            content.addAndMakeVisible (b);
            entries.push_back ({ &b, T, -1, -1 });

            auto* sh = combo (lfoId (l, "shape"), { r.getX() + 160, r.getY() + 38, 150, 22 }, T, -1, l);
            sh->onChange = [this] { if (lfoView) lfoView->repaint(); };
            combo (lfoId (l, "div"), { r.getX() + 316, r.getY() + 38, 112, 22 }, T, -1, l);
            toggle (lfoId (l, "sync"), "SYNC", { r.getX() + 160, r.getY() + 74, 80, 22 }, T, -1, l);
            toggle (lfoId (l, "retrig"), "RETRIG", { r.getX() + 160, r.getY() + 102, 90, 22 }, T, -1, l);
            knob (lfoId (l, "rate"), "RATE (Hz)", { r.getX() + 300, r.getY() + 68, 90, 92 }, T, -1, l);
        }
        lfoView = own (std::make_unique<LfoView> (proc), T);
        lfoView->setBounds (r.getX() + 10, r.getY() + 36, 140, 124);
    }
    // Macros
    {
        const juce::Rectangle<int> r (876, 588, 292, 170);
        panel (r, "MACROS", T);
        const char* lb[4] = { "1 BRIGHT", "2 GRIM", "3 ABYSS", "4 MORPH" };
        for (int m = 0; m < 4; ++m)
            knob (macroId (m), lb[m], { r.getX() + 6 + m * 71, r.getY() + 46, 70, 100 }, T);
    }
}

void NoxWaveEditor::buildFxTab()
{
    constexpr int T = 1;
    auto rect = [] (int c, int row) { return juce::Rectangle<int> (12 + c * 389, 72 + row * 346, 378, 334); };
    const int kw = 104, kh = 120;

    { auto r = rect (0, 0); panel (r, {}, T);
      toggle ("dist_on", "DISTORTION", { r.getX() + 10, r.getY() + 8, 140, 22 }, T);
      combo ("dist_mode", { r.getX() + 16, r.getY() + 48, 160, 24 }, T);
      knob ("dist_drive", "DRIVE", { r.getX() + 20, r.getY() + 110, kw, kh }, T);
      knob ("dist_tone", "TONE", { r.getX() + 136, r.getY() + 110, kw, kh }, T);
      knob ("dist_mix", "MIX", { r.getX() + 252, r.getY() + 110, kw, kh }, T); }

    { auto r = rect (1, 0); panel (r, {}, T);
      toggle ("eq_on", "EQ", { r.getX() + 10, r.getY() + 8, 100, 22 }, T);
      knob ("eq_low", "LOW", { r.getX() + 8, r.getY() + 110, 88, kh }, T);
      knob ("eq_mid", "MID", { r.getX() + 98, r.getY() + 110, 88, kh }, T);
      knob ("eq_midfreq", "MID FREQ", { r.getX() + 188, r.getY() + 110, 88, kh }, T);
      knob ("eq_high", "HIGH", { r.getX() + 278, r.getY() + 110, 88, kh }, T); }

    { auto r = rect (2, 0); panel (r, {}, T);
      toggle ("trem_on", "TREMOLO", { r.getX() + 10, r.getY() + 8, 140, 22 }, T);
      combo ("trem_div", { r.getX() + 16, r.getY() + 48, 100, 24 }, T);
      combo ("trem_shape", { r.getX() + 124, r.getY() + 48, 160, 24 }, T);
      knob ("trem_depth", "DEPTH", { r.getX() + 137, r.getY() + 110, kw, kh }, T); }

    { auto r = rect (0, 1); panel (r, {}, T);
      toggle ("cho_on", "CHORUS", { r.getX() + 10, r.getY() + 8, 140, 22 }, T);
      knob ("cho_rate", "RATE", { r.getX() + 20, r.getY() + 110, kw, kh }, T);
      knob ("cho_depth", "DEPTH", { r.getX() + 136, r.getY() + 110, kw, kh }, T);
      knob ("cho_mix", "MIX", { r.getX() + 252, r.getY() + 110, kw, kh }, T); }

    { auto r = rect (1, 1); panel (r, {}, T);
      toggle ("dly_on", "DELAY", { r.getX() + 10, r.getY() + 8, 140, 22 }, T);
      combo ("dly_div", { r.getX() + 16, r.getY() + 48, 100, 24 }, T);
      toggle ("dly_ping", "PING-PONG", { r.getX() + 130, r.getY() + 48, 130, 24 }, T);
      knob ("dly_fb", "FEEDBACK", { r.getX() + 20, r.getY() + 110, kw, kh }, T);
      knob ("dly_tone", "TONE", { r.getX() + 136, r.getY() + 110, kw, kh }, T);
      knob ("dly_mix", "MIX", { r.getX() + 252, r.getY() + 110, kw, kh }, T); }

    { auto r = rect (2, 1); panel (r, {}, T);
      toggle ("rev_on", "REVERB", { r.getX() + 10, r.getY() + 8, 140, 22 }, T);
      knob ("rev_size", "SIZE", { r.getX() + 20, r.getY() + 70, kw, 110 }, T);
      knob ("rev_decay", "DECAY", { r.getX() + 136, r.getY() + 70, kw, 110 }, T);
      knob ("rev_damp", "DAMP", { r.getX() + 252, r.getY() + 70, kw, 110 }, T);
      knob ("rev_predelay", "PREDELAY", { r.getX() + 78, r.getY() + 196, kw, 110 }, T);
      knob ("rev_mix", "MIX", { r.getX() + 194, r.getY() + 196, kw, 110 }, T); }
}

void NoxWaveEditor::buildMatrixTab()
{
    constexpr int T = 2;
    const juce::Rectangle<int> r (12, 72, 1156, 686);
    panel (r, "MOD MATRIX", T);
    for (int m = 0; m < kNumModSlots; ++m)
    {
        const int colIdx = m / 6, row = m % 6;
        const int x = r.getX() + 20 + colIdx * 574, y = r.getY() + 80 + row * 78;
        auto* num = own (std::make_unique<juce::Label> (juce::String(), juce::String (m + 1)), T);
        num->setFont (juce::FontOptions (16.0f, juce::Font::bold));
        num->setColour (juce::Label::textColourId, m >= 8 ? col::blood.brighter (0.3f) : col::ice);
        num->setBounds (x, y, 30, 28);
        combo (modId (m, "src"), { x + 34, y, 150, 28 }, T);
        auto* arrow = own (std::make_unique<juce::Label> (juce::String(), juce::CharPointer_UTF8 ("\xe2\x86\x92")), T);
        arrow->setColour (juce::Label::textColourId, col::dim);
        arrow->setBounds (x + 188, y, 22, 28);
        combo (modId (m, "dst"), { x + 212, y, 170, 28 }, T);
        auto* s = own (std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight), T);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
        s->setColour (juce::Slider::textBoxTextColourId, col::text);
        s->setColour (juce::Slider::textBoxOutlineColourId, col::line);
        s->setBounds (x + 392, y, 150, 28);
        sliderAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, modId (m, "amt"), *s));
        s->setDoubleClickReturnValue (true, 0.0);
    }
}

//------------------------------------------------------------------------------
void NoxWaveEditor::setTab (int t)
{
    tab = t;
    for (int i = 0; i < 3; ++i) tabButtons[i].setToggleState (i == t, juce::dontSendNotification);
    updateVisibility();
    content.repaint();
}

void NoxWaveEditor::updateVisibility()
{
    for (int e = 0; e < 3; ++e) envButtons[e].setToggleState (e == envSel, juce::dontSendNotification);
    for (int l = 0; l < kNumLfos; ++l) lfoButtons[l].setToggleState (l == lfoSel, juce::dontSendNotification);
    for (auto& en : entries)
    {
        bool vis = en.tab < 0 || en.tab == tab;
        if (en.env >= 0) vis = vis && en.env == envSel;
        if (en.lfo >= 0) vis = vis && en.lfo == lfoSel;
        en.c->setVisible (vis);
    }
}

void NoxWaveEditor::timerCallback()
{
    voiceLabel.setText ("voix : " + juce::String (proc.getActiveVoices()) + " / 16", juce::dontSendNotification);
    const int prog = proc.getCurrentProgram();
    if (presetBox.getSelectedId() != prog + 1) presetBox.setSelectedId (prog + 1, juce::dontSendNotification);
    if (presetBox.getText() != proc.presetName) presetBox.setText (proc.presetName, juce::dontSendNotification);
    if (tab == 0) { envView->repaint(); lfoView->repaint(); }
}

void NoxWaveEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (col::bg);

    // grain subtil + vignette
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15151a), (float) baseW * 0.5f, 0.0f,
                                             col::bg, (float) baseW * 0.5f, (float) baseH * 0.6f, true));
    g.fillRect (0, 0, baseW, baseH);

    // en-tête
    g.setColour (col::text);
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)).withExtraKerningFactor (0.28f));
    g.drawText ("NOXWAVE", 16, 8, 240, 34, juce::Justification::centredLeft);
    g.setColour (col::blood.brighter (0.2f));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("WAVETABLE SYNTH  //  BLACK METAL EDITION", 18, 42, 260, 16, juce::Justification::centredLeft);
    g.setColour (col::line);
    g.drawHorizontalLine (66, 12.0f, (float) baseW - 12.0f);

    for (const auto& p : panels)
    {
        if (p.tab != tab) continue;
        auto r = p.r.toFloat();
        g.setColour (col::panel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (col::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (col::blood.withAlpha (0.7f));
        g.fillRect (r.getX() + 6.0f, r.getY(), r.getWidth() - 12.0f, 1.5f);
        if (p.title.isNotEmpty())
        {
            g.setColour (col::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawText (p.title, p.r.getX() + 12, p.r.getY() + 6, 120, 22, juce::Justification::centredLeft);
        }
    }

    g.setFont (juce::FontOptions (11.5f));
    g.setColour (col::dim);
    if (tab == 1)
    {
        g.drawText ("Synchro tempo. \"Chop\" imite le tremolo picking.", 12 + 2 * 389 + 16, 72 + 250, 350, 18, juce::Justification::centredLeft);
        g.drawText ("Ordre : Dist > EQ > Tremolo > Chorus > Delay > Reverb", 12, 754, 600, 14, juce::Justification::centredLeft);
    }
    if (tab == 2)
    {
        g.drawText (juce::String::fromUTF8 ("Source  >  Destination  >  Quantité.  Les destinations \"FX ...\" sont globales : elles suivent les LFO (mode libre), macros, mod wheel, aftertouch et pitch bend."),
                    32, 112, 1120, 18, juce::Justification::centredLeft);
        g.drawText (juce::String::fromUTF8 ("Slots 9 à 12 (rouges) : pré-câblés sur les 4 macros  —  1 Bright > Cutoff, 2 Grim > Distortion, 3 Abyss > Reverb, 4 Morph > Position A."),
                    32, 700, 1120, 18, juce::Justification::centredLeft);
    }
}
