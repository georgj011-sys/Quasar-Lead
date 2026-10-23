#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include "PluginProcessor.h"

namespace Theme
{
    inline const juce::Colour bg        { 0xff121316 };
    inline const juce::Colour panel     { 0xff1c1e22 };
    inline const juce::Colour panelEdge { 0xff30333a };
    inline const juce::Colour text      { 0xffe6e1d6 };
    inline const juce::Colour textDim   { 0xff8a877f };
    inline const juce::Colour metal1    { 0xff3a3d42 };
    inline const juce::Colour metal2    { 0xff25272b };

    // Colour-coded sections, like on hardware front panels
    inline const juce::Colour osc     { 0xffff8a3d };
    inline const juce::Colour mix     { 0xffffb347 };
    inline const juce::Colour voice   { 0xffc792ea };
    inline const juce::Colour filter  { 0xfff2d34f };
    inline const juce::Colour env     { 0xff5aa9ff };
    inline const juce::Colour lfo     { 0xffb388ff };
    inline const juce::Colour pitch   { 0xffff6b9a };
    inline const juce::Colour colour  { 0xffff5d5d };
    inline const juce::Colour stereo  { 0xff3fd8c4 };
}

class QuasarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    QuasarLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f)); }
    juce::Font getPopupMenuFont() override               { return juce::Font (juce::FontOptions (14.0f)); }
};

class LabeledKnob : public juce::Component
{
public:
    LabeledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                 const juce::String& name, juce::Colour accent);
    void resized() override;
    void setLabelText (const juce::String& t) { label.setText (t, juce::dontSendNotification); }

private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class ParamCombo : public juce::Component
{
public:
    ParamCombo (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& name);
    void resized() override;

private:
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// Waveform scope + model selector. Also renames the Shape/Color knobs per model.
class OscTop : public juce::Component, private juce::Timer
{
public:
    OscTop (juce::AudioProcessorValueTreeState& state, int oscIndex, juce::Colour accent);
    void setModKnobs (LabeledKnob* shapeKnob, LabeledKnob* colorKnob);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    std::atomic<float>* typeParam;
    std::atomic<float>* shapeParam;
    std::atomic<float>* colorParam;
    juce::Colour accent;

    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    LabeledKnob* shapeKnob = nullptr;
    LabeledKnob* colorKnob = nullptr;

    std::array<float, 256> wave {};
    int lastType = -1;
    float lastShape = -1.0f, lastColor = -1.0f;
    juce::Rectangle<int> scopeArea;
};

class Section : public juce::Component
{
public:
    Section (juce::String title, int columns, juce::Colour accent);

    LabeledKnob* addKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& name);
    void addCombo (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& name);
    void setTop (std::unique_ptr<juce::Component> component, int height);
    juce::Component* getTop() { return top.get(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String title;
    int columns;
    juce::Colour accent;
    std::unique_ptr<juce::Component> top;
    int topHeight = 0;
    std::vector<std::unique_ptr<juce::Component>> items;
};

class QuasarEditor : public juce::AudioProcessorEditor
{
public:
    explicit QuasarEditor (QuasarProcessor&);
    ~QuasarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectPreset (int index);
    Section& makeSection (const juce::String& title, int columns, juce::Colour accent);

    static constexpr int cheekWidth = 22;
    static constexpr int headerHeight = 56;

    QuasarLookAndFeel lnf;
    QuasarProcessor& proc;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };

    std::vector<std::unique_ptr<Section>> sections;
    Section* oscA = nullptr;
    Section* oscB = nullptr;
    Section* mixSec = nullptr;
    Section* voiceSec = nullptr;
    Section* filterSec = nullptr;
    Section* fenvSec = nullptr;
    Section* ampSec = nullptr;
    Section* lfoSec = nullptr;
    Section* pitchSec = nullptr;
    Section* driveSec = nullptr;
    Section* phaserSec = nullptr;
    Section* chorusSec = nullptr;
    Section* reverbSec = nullptr;
    Section* delaySec = nullptr;
    Section* stereoSec = nullptr;
    Section* outSec = nullptr;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuasarEditor)
};
