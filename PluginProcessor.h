#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include "Voice.h"
#include "StereoFX.h"

class QuasarProcessor : public juce::AudioProcessor
{
public:
    QuasarProcessor();
    ~QuasarProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Quasar Lead"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    enum Mode { Mono = 0, Legato, Poly };

    void readParams (LeadParams& lp, FxParams& fp) const;
    void renderVoices (float* L, float* R, int start, int num, const LeadParams& lp);
    void handleMidi (const juce::MidiMessage& msg, const LeadParams& lp);
    void noteOn (int note, float velocity, const LeadParams& lp);
    void noteOff (int note);
    void releaseAll();

    float v (juce::StringRef id) const  { return apvts.getRawParameterValue (id)->load(); }
    int vi (juce::StringRef id) const   { return (int) std::lround (v (id)); }

    std::array<LeadVoice, 6> voices;
    StereoFX fx;

    juce::Array<int> noteStack;
    int lastNote = -1;
    int mode = Mono, lastMode = -1;
    float bend = 0.0f, modWheel = 0.0f;
    juce::uint32 seed = 1;
    juce::uint64 ageCounter = 0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuasarProcessor)
};
