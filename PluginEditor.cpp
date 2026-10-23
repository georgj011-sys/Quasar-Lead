#include "PluginEditor.h"
#include "Params.h"
#include "Oscillator.h"

//==============================================================================
QuasarLookAndFeel::QuasarLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Theme::bg);
    setColour (juce::Label::textColourId, Theme::text);

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff16171a));
    setColour (juce::ComboBox::outlineColourId, Theme::panelEdge);
    setColour (juce::ComboBox::textColourId, Theme::text);
    setColour (juce::ComboBox::arrowColourId, Theme::textDim);
    setColour (juce::ComboBox::focusedOutlineColourId, Theme::text);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a1b1f));
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff34373e));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff26282d));
    setColour (juce::TextButton::textColourOffId, Theme::text);

    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff0f1012));
    setColour (juce::BubbleComponent::outlineColourId, Theme::panelEdge);
    setColour (juce::TooltipWindow::textColourId, Theme::text);
}

void QuasarLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                          float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path track;
    track.addCentredArc (cx, cy, radius - 2.0f, radius - 2.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (Theme::panelEdge);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float arcStart = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    if (std::abs (angle - arcStart) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (cx, cy, radius - 2.0f, radius - 2.0f, 0.0f,
                             juce::jmin (arcStart, angle), juce::jmax (arcStart, angle), true);
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = radius - 7.0f;
    if (kr <= 2.0f)
        return;

    // Black hardware knob with a skirt
    g.setColour (juce::Colour (0xff0c0d0f));
    g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);

    const float capR = kr * 0.78f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3e45), cx, cy - capR,
                                             juce::Colour (0xff17181b), cx, cy + capR, false));
    g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -kr + 2.0f, 3.0f, kr * 0.5f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (slider.isMouseOverOrDragging() ? accent : Theme::text);
    g.fillPath (pointer);
}

//==============================================================================
LabeledKnob::LabeledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                          const juce::String& name, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));
    label.setColour (juce::Label::textColourId, Theme::textDim);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);

    if (auto* p = state.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void LabeledKnob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromBottom (16));
    slider.setBounds (b);
}

//==============================================================================
ParamCombo::ParamCombo (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& name)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramID)))
        box.addItemList (choice->choices, 1);
    addAndMakeVisible (box);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));
    label.setColour (juce::Label::textColourId, Theme::textDim);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, paramID, box);
}

void ParamCombo::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromBottom (16));
    box.setBounds (b.withSizeKeepingCentre (b.getWidth() - 4, juce::jmin (26, b.getHeight())));
}

//==============================================================================
OscTop::OscTop (juce::AudioProcessorValueTreeState& state, int i, juce::Colour c)
    : typeParam (state.getRawParameterValue (Params::osc (i, "type"))),
      shapeParam (state.getRawParameterValue (Params::osc (i, "shape"))),
      colorParam (state.getRawParameterValue (Params::osc (i, "color"))),
      accent (c)
{
    box.addItemList (Params::oscTypes(), 1);
    addAndMakeVisible (box);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, Params::osc (i, "type"), box);

    startTimerHz (20);
}

void OscTop::setModKnobs (LabeledKnob* s, LabeledKnob* c)
{
    shapeKnob = s;
    colorKnob = c;
    lastType = -1;
}

void OscTop::timerCallback()
{
    const int type = (int) std::lround (typeParam->load());
    const float shape = shapeParam->load();
    const float color = colorParam->load();

    if (type != lastType)
    {
        if (shapeKnob != nullptr) shapeKnob->setLabelText (Params::shapeName (type));
        if (colorKnob != nullptr) colorKnob->setLabelText (Params::colorName (type));
    }

    if (type != lastType || shape != lastShape || color != lastColor)
    {
        lastType = type;
        lastShape = shape;
        lastColor = color;

        LeadOsc::renderPreview (type, shape, color, wave.data(), (int) wave.size());

        float peak = 0.0f;
        for (auto s : wave)
            peak = juce::jmax (peak, std::abs (s));
        if (peak > 1.0e-6f)
            for (auto& s : wave)
                s /= peak;

        repaint();
    }
}

void OscTop::resized()
{
    auto b = getLocalBounds();
    box.setBounds (b.removeFromBottom (24));
    b.removeFromBottom (6);
    scopeArea = b;
}

void OscTop::paint (juce::Graphics& g)
{
    const auto b = scopeArea.toFloat();
    g.setColour (juce::Colour (0xff0b0c0e));
    g.fillRoundedRectangle (b, 4.0f);

    g.setColour (accent.withAlpha (0.08f));
    g.drawHorizontalLine ((int) b.getCentreY(), b.getX() + 4.0f, b.getRight() - 4.0f);

    if (lastType < 0)
        return;

    const auto area = b.reduced (6.0f, 8.0f);
    const int n = (int) wave.size();
    juce::Path path;

    for (int i = 0; i < n; ++i)
    {
        const float px = area.getX() + area.getWidth() * (float) i / (float) (n - 1);
        const float py = area.getCentreY() - wave[(size_t) i] * area.getHeight() * 0.48f;
        if (i == 0) path.startNewSubPath (px, py);
        else        path.lineTo (px, py);
    }

    g.setColour (accent.withAlpha (0.2f));
    g.strokePath (path, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
Section::Section (juce::String t, int cols, juce::Colour c)
    : title (std::move (t)), columns (juce::jmax (1, cols)), accent (c) {}

LabeledKnob* Section::addKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& name)
{
    auto k = std::make_unique<LabeledKnob> (state, id, name, accent);
    auto* raw = k.get();
    addAndMakeVisible (*k);
    items.push_back (std::move (k));
    return raw;
}

void Section::addCombo (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& name)
{
    auto c = std::make_unique<ParamCombo> (state, id, name);
    addAndMakeVisible (*c);
    items.push_back (std::move (c));
}

void Section::setTop (std::unique_ptr<juce::Component> component, int height)
{
    top = std::move (component);
    topHeight = height;
    addAndMakeVisible (*top);
}

void Section::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (Theme::panelEdge);
    g.drawRoundedRectangle (b, 5.0f, 1.0f);

    // Coloured strip along the top edge marks the section's group
    g.setColour (accent);
    g.fillRoundedRectangle (b.getX() + 10.0f, b.getY() + 1.0f, b.getWidth() - 20.0f, 2.0f, 1.0f);

    auto titleArea = b.removeFromTop (28.0f).reduced (12.0f, 0.0f);
    g.setColour (Theme::text);
    g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
    g.drawText (title, titleArea, juce::Justification::centredLeft);
}

void Section::resized()
{
    auto b = getLocalBounds().reduced (8);
    b.removeFromTop (22);

    if (top != nullptr)
    {
        top->setBounds (b.removeFromTop (topHeight));
        b.removeFromTop (6);
    }

    const int n = (int) items.size();
    if (n == 0)
        return;

    const int rows = (n + columns - 1) / columns;
    const int cellW = b.getWidth() / columns;
    const int cellH = b.getHeight() / rows;

    for (int i = 0; i < n; ++i)
    {
        const int r = i / columns, c = i % columns;
        items[(size_t) i]->setBounds (juce::Rectangle<int> (b.getX() + c * cellW, b.getY() + r * cellH, cellW, cellH).reduced (2));
    }
}

//==============================================================================
Section& QuasarEditor::makeSection (const juce::String& title, int columns, juce::Colour accent)
{
    sections.push_back (std::make_unique<Section> (title, columns, accent));
    auto& s = *sections.back();
    addAndMakeVisible (s);
    return s;
}

QuasarEditor::QuasarEditor (QuasarProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    auto& s = p.apvts;

    for (int i = 0; i < 2; ++i)
    {
        auto& sec = makeSection (i == 0 ? "Oscillator A" : "Oscillator B", 4, Theme::osc);
        auto top = std::make_unique<OscTop> (s, i, Theme::osc);
        auto* topRaw = top.get();
        sec.setTop (std::move (top), 96);

        auto* shapeK = sec.addKnob (s, Params::osc (i, "shape"), "Shape");
        auto* colorK = sec.addKnob (s, Params::osc (i, "color"), "Color");
        sec.addKnob (s, Params::osc (i, "level"), "Level");
        sec.addKnob (s, Params::osc (i, "pan"), "Pan");
        sec.addKnob (s, Params::osc (i, "octave"), "Octave");
        sec.addKnob (s, Params::osc (i, "semi"), "Semi");
        sec.addKnob (s, Params::osc (i, "fine"), "Fine");
        topRaw->setModKnobs (shapeK, colorK);

        (i == 0 ? oscA : oscB) = &sec;
    }

    mixSec = &makeSection ("Mix", 1, Theme::mix);
    mixSec->addCombo (s, "sub_type", "Sub oscillator");
    mixSec->addKnob (s, "sub_level", "Sub level");
    mixSec->addKnob (s, "ring", "Ring mod");

    voiceSec = &makeSection ("Voice", 1, Theme::voice);
    voiceSec->addCombo (s, "mode", "Mode");
    voiceSec->addKnob (s, "glide", "Glide");
    voiceSec->addKnob (s, "bend", "Bend range");

    filterSec = &makeSection ("Filter", 3, Theme::filter);
    filterSec->addCombo (s, "filter_type", "Type");
    filterSec->addKnob (s, "cutoff", "Cutoff");
    filterSec->addKnob (s, "reso", "Resonance");
    filterSec->addKnob (s, "fenv_amt", "Env amount");
    filterSec->addKnob (s, "keytrack", "Key track");
    filterSec->addKnob (s, "filter_drive", "Drive");

    fenvSec = &makeSection ("Filter envelope", 2, Theme::env);
    fenvSec->addKnob (s, "flt_attack", "Attack");
    fenvSec->addKnob (s, "flt_decay", "Decay");
    fenvSec->addKnob (s, "flt_sustain", "Sustain");
    fenvSec->addKnob (s, "flt_release", "Release");

    ampSec = &makeSection ("Amp envelope", 2, Theme::env);
    ampSec->addKnob (s, "amp_attack", "Attack");
    ampSec->addKnob (s, "amp_decay", "Decay");
    ampSec->addKnob (s, "amp_sustain", "Sustain");
    ampSec->addKnob (s, "amp_release", "Release");

    lfoSec = &makeSection ("LFO", 3, Theme::lfo);
    lfoSec->addCombo (s, "lfo_wave", "Wave");
    lfoSec->addKnob (s, "lfo_rate", "Rate");
    lfoSec->addKnob (s, "lfo_pitch", "To pitch");
    lfoSec->addKnob (s, "lfo_cutoff", "To cutoff");
    lfoSec->addKnob (s, "lfo_osc", "To shape");

    pitchSec = &makeSection ("Pitch envelope", 1, Theme::pitch);
    pitchSec->addKnob (s, "penv_amt", "Amount");
    pitchSec->addKnob (s, "penv_decay", "Decay");

    driveSec = &makeSection ("Drive", 2, Theme::colour);
    driveSec->addCombo (s, "dist_type", "Type");
    driveSec->addKnob (s, "dist_amt", "Amount");

    phaserSec = &makeSection ("Stereo phaser", 3, Theme::stereo);
    phaserSec->addKnob (s, "phaser_rate", "Rate");
    phaserSec->addKnob (s, "phaser_depth", "Depth");
    phaserSec->addKnob (s, "phaser_mix", "Mix");

    chorusSec = &makeSection ("Stereo chorus", 3, Theme::stereo);
    chorusSec->addKnob (s, "chorus_rate", "Rate");
    chorusSec->addKnob (s, "chorus_depth", "Depth");
    chorusSec->addKnob (s, "chorus_mix", "Mix");

    reverbSec = &makeSection ("Reverb", 2, Theme::stereo);
    reverbSec->addKnob (s, "reverb_size", "Size");
    reverbSec->addKnob (s, "reverb_mix", "Mix");

    delaySec = &makeSection ("Ping-pong delay", 4, Theme::stereo);
    delaySec->addKnob (s, "delay_time", "Time");
    delaySec->addKnob (s, "delay_fb", "Feedback");
    delaySec->addKnob (s, "delay_tone", "Tone");
    delaySec->addKnob (s, "delay_mix", "Mix");

    stereoSec = &makeSection ("Stereo field", 4, Theme::stereo);
    stereoSec->addKnob (s, "pan_rate", "Pan rate");
    stereoSec->addKnob (s, "pan_depth", "Pan depth");
    stereoSec->addKnob (s, "haas", "Haas");
    stereoSec->addKnob (s, "width", "Width");

    outSec = &makeSection ("Output", 1, Theme::text);
    outSec->addKnob (s, "master", "Master");

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevButton.onClick = [this]
    {
        const int n = proc.getNumPrograms();
        selectPreset ((proc.getCurrentProgram() - 1 + n) % n);
    };
    nextButton.onClick = [this]
    {
        selectPreset ((proc.getCurrentProgram() + 1) % proc.getNumPrograms());
    };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    keyboard.setLowestVisibleKey (36);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::osc.withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::osc.withAlpha (0.25f));
    addAndMakeVisible (keyboard);

    setSize (1240, 880);
}

QuasarEditor::~QuasarEditor()
{
    setLookAndFeel (nullptr);
}

void QuasarEditor::selectPreset (int index)
{
    if (index < 0 || index >= proc.getNumPrograms())
        return;

    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

void QuasarEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);

    // Brushed-metal end cheeks with screws
    auto drawCheek = [&g] (juce::Rectangle<float> r, int seedValue)
    {
        g.setGradientFill (juce::ColourGradient (Theme::metal1, r.getX(), 0.0f, Theme::metal2, r.getRight(), 0.0f, false));
        g.fillRect (r);

        juce::Random grain (seedValue);
        for (int i = 0; i < 60; ++i)
        {
            const float gy = r.getY() + grain.nextFloat() * r.getHeight();
            g.setColour (juce::Colours::white.withAlpha (0.02f + grain.nextFloat() * 0.03f));
            g.drawHorizontalLine ((int) gy, r.getX(), r.getRight());
        }

        for (float sy : { r.getY() + 18.0f, r.getBottom() - 18.0f })
        {
            const float sx = r.getCentreX();
            g.setColour (juce::Colour (0xff15161a));
            g.fillEllipse (sx - 5.0f, sy - 5.0f, 10.0f, 10.0f);
            g.setColour (juce::Colour (0xff55585f));
            g.drawLine (sx - 3.0f, sy, sx + 3.0f, sy, 1.2f);
        }
    };

    const auto full = getLocalBounds().toFloat();
    drawCheek (full.withWidth ((float) cheekWidth), 7);
    drawCheek (full.withLeft (full.getRight() - (float) cheekWidth), 13);

    auto header = getLocalBounds().reduced (cheekWidth, 0).removeFromTop (headerHeight).toFloat().reduced (18.0f, 0.0f);
    g.setColour (Theme::text);
    g.setFont (juce::Font (juce::FontOptions (26.0f).withStyle ("Bold")));
    g.drawText ("Quasar Lead", header, juce::Justification::centredLeft);

    g.setColour (Theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Two-oscillator solo synthesizer with stereo effects", header.withTrimmedLeft (170.0f),
                juce::Justification::centredLeft);
}

void QuasarEditor::resized()
{
    auto b = getLocalBounds().reduced (cheekWidth, 0);

    auto header = b.removeFromTop (headerHeight).reduced (16, 14);
    nextButton.setBounds (header.removeFromRight (30));
    header.removeFromRight (4);
    presetBox.setBounds (header.removeFromRight (240));
    header.removeFromRight (4);
    prevButton.setBounds (header.removeFromRight (30));

    auto content = b.reduced (10, 6);
    keyboard.setBounds (content.removeFromBottom (70));
    content.removeFromBottom (8);

    const int gap = 8;

    auto layoutRow = [gap] (juce::Rectangle<int> row, std::initializer_list<std::pair<Section*, float>> parts)
    {
        const int usable = row.getWidth() - gap * ((int) parts.size() - 1);
        int index = 0;
        for (const auto& part : parts)
        {
            const bool last = index == (int) parts.size() - 1;
            const int w = last ? row.getWidth() : (int) ((float) usable * part.second);
            part.first->setBounds (row.removeFromLeft (w));
            if (! last)
                row.removeFromLeft (gap);
            ++index;
        }
    };

    layoutRow (content.removeFromTop (262), { { oscA, 0.31f }, { oscB, 0.31f }, { mixSec, 0.16f }, { voiceSec, 0.22f } });
    content.removeFromTop (gap);
    layoutRow (content.removeFromTop (190), { { filterSec, 0.29f }, { fenvSec, 0.15f }, { ampSec, 0.15f },
                                              { lfoSec, 0.27f }, { pitchSec, 0.14f } });
    content.removeFromTop (gap);

    const int fxRowHeight = (content.getHeight() - gap) / 2;
    layoutRow (content.removeFromTop (fxRowHeight), { { driveSec, 0.2f }, { phaserSec, 0.3f }, { chorusSec, 0.3f }, { reverbSec, 0.2f } });
    content.removeFromTop (gap);
    layoutRow (content, { { delaySec, 0.44f }, { stereoSec, 0.44f }, { outSec, 0.12f } });
}
