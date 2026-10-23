#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Params
{
    inline const juce::StringArray& oscTypes()
    {
        static const juce::StringArray s { "Saw", "Pulse", "Fold", "Sync", "FM", "Formant", "Noise" };
        return s;
    }

    // Per-type names for the two character knobs, shown on the panel.
    inline const char* shapeName (int type)
    {
        static const char* names[] = { "Detune", "Width", "Fold", "Sync", "Index", "Formant", "Rate" };
        return names[juce::jlimit (0, 6, type)];
    }

    inline const char* colorName (int type)
    {
        static const char* names[] = { "Stack", "PWM", "Bias", "Blend", "Ratio", "Focus", "Smooth" };
        return names[juce::jlimit (0, 6, type)];
    }

    inline juce::String osc (int index, const char* name)
    {
        return juce::String (index == 0 ? "a_" : "b_") + name;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        auto skewed = [] (float lo, float hi, float centre)
        {
            NormalisableRange<float> r (lo, hi);
            r.setSkewForCentre (centre);
            return r;
        };

        auto addFloat = [&layout] (const String& id, const String& name,
                                   NormalisableRange<float> range, float def, const String& label = {})
        {
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def,
                                                               AudioParameterFloatAttributes().withLabel (label)));
        };

        auto addInt = [&layout] (const String& id, const String& name, int lo, int hi, int def)
        {
            layout.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def));
        };

        auto addChoice = [&layout] (const String& id, const String& name, const StringArray& items, int def)
        {
            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
        };

        const NormalisableRange<float> unit (0.0f, 1.0f);
        const NormalisableRange<float> bipolar (-1.0f, 1.0f);

        // Voice
        addChoice ("mode", "Voice Mode", StringArray { "Mono", "Legato", "Poly" }, 0);
        addFloat ("glide", "Glide", skewed (0.0f, 2.0f, 0.2f), 0.05f, "s");
        addInt ("bend", "Bend Range", 1, 24, 2);

        // Oscillators
        const float defLevel[2] = { 0.8f, 0.5f };
        const float defPan[2]   = { -0.2f, 0.2f };
        const float defFine[2]  = { 0.0f, 8.0f };

        for (int i = 0; i < 2; ++i)
        {
            const String n = i == 0 ? "Osc A " : "Osc B ";
            addChoice (osc (i, "type"), n + "Type", oscTypes(), 0);
            addFloat (osc (i, "shape"), n + "Shape", unit, 0.3f);
            addFloat (osc (i, "color"), n + "Color", unit, 0.5f);
            addInt (osc (i, "octave"), n + "Octave", -3, 3, 0);
            addInt (osc (i, "semi"), n + "Semitones", -12, 12, 0);
            addFloat (osc (i, "fine"), n + "Fine", NormalisableRange<float> (-100.0f, 100.0f), defFine[i], "ct");
            addFloat (osc (i, "level"), n + "Level", unit, defLevel[i]);
            addFloat (osc (i, "pan"), n + "Pan", bipolar, defPan[i]);
        }

        // Mix
        addChoice ("sub_type", "Sub Type", StringArray { "Sine -1 oct", "Square -1 oct", "Sine -2 oct" }, 0);
        addFloat ("sub_level", "Sub Level", unit, 0.15f);
        addFloat ("ring", "Ring Mod", unit, 0.0f);

        // Filter
        addChoice ("filter_type", "Filter Type", StringArray { "Low pass 12", "Low pass 24", "Band pass", "High pass", "Notch", "Comb" }, 1);
        addFloat ("cutoff", "Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 3000.0f, "Hz");
        addFloat ("reso", "Resonance", unit, 0.3f);
        addFloat ("fenv_amt", "Filter Env Amount", bipolar, 0.4f);
        addFloat ("keytrack", "Key Tracking", unit, 0.5f);
        addFloat ("filter_drive", "Filter Drive", unit, 0.2f);

        // Envelopes
        const auto timeRange = skewed (0.001f, 10.0f, 0.5f);
        addFloat ("amp_attack", "Amp Attack", timeRange, 0.005f, "s");
        addFloat ("amp_decay", "Amp Decay", timeRange, 0.3f, "s");
        addFloat ("amp_sustain", "Amp Sustain", unit, 0.8f);
        addFloat ("amp_release", "Amp Release", timeRange, 0.25f, "s");
        addFloat ("flt_attack", "Filter Attack", timeRange, 0.002f, "s");
        addFloat ("flt_decay", "Filter Decay", timeRange, 0.4f, "s");
        addFloat ("flt_sustain", "Filter Sustain", unit, 0.3f);
        addFloat ("flt_release", "Filter Release", timeRange, 0.3f, "s");

        // LFO
        addChoice ("lfo_wave", "LFO Wave", StringArray { "Sine", "Triangle", "Saw", "Square", "Sample & hold", "Drift" }, 0);
        addFloat ("lfo_rate", "LFO Rate", skewed (0.05f, 30.0f, 3.0f), 5.0f, "Hz");
        addFloat ("lfo_pitch", "LFO to Pitch", unit, 0.1f);
        addFloat ("lfo_cutoff", "LFO to Cutoff", unit, 0.0f);
        addFloat ("lfo_osc", "LFO to Shape", unit, 0.0f);

        // Pitch envelope
        addFloat ("penv_amt", "Pitch Env Amount", NormalisableRange<float> (-24.0f, 24.0f), 0.0f, "st");
        addFloat ("penv_decay", "Pitch Env Decay", skewed (0.005f, 2.0f, 0.2f), 0.1f, "s");

        // Effects
        addChoice ("dist_type", "Drive Type", StringArray { "Soft", "Hard", "Fold", "Crush" }, 0);
        addFloat ("dist_amt", "Drive Amount", unit, 0.0f);
        addFloat ("phaser_rate", "Phaser Rate", skewed (0.02f, 10.0f, 1.0f), 0.4f, "Hz");
        addFloat ("phaser_depth", "Phaser Depth", unit, 0.7f);
        addFloat ("phaser_mix", "Phaser Mix", unit, 0.0f);
        addFloat ("chorus_rate", "Chorus Rate", skewed (0.05f, 8.0f, 1.0f), 0.6f, "Hz");
        addFloat ("chorus_depth", "Chorus Depth", unit, 0.5f);
        addFloat ("chorus_mix", "Chorus Mix", unit, 0.2f);
        addFloat ("delay_time", "Delay Time", skewed (0.02f, 1.5f, 0.35f), 0.33f, "s");
        addFloat ("delay_fb", "Delay Feedback", NormalisableRange<float> (0.0f, 0.95f), 0.35f);
        addFloat ("delay_tone", "Delay Tone", unit, 0.5f);
        addFloat ("delay_mix", "Delay Mix", unit, 0.15f);
        addFloat ("reverb_size", "Reverb Size", unit, 0.5f);
        addFloat ("reverb_mix", "Reverb Mix", unit, 0.12f);
        addFloat ("pan_rate", "Auto-Pan Rate", skewed (0.05f, 15.0f, 1.5f), 1.0f, "Hz");
        addFloat ("pan_depth", "Auto-Pan Depth", unit, 0.0f);
        addFloat ("haas", "Haas Delay", NormalisableRange<float> (0.0f, 30.0f), 0.0f, "ms");
        addFloat ("width", "Stereo Width", NormalisableRange<float> (0.0f, 2.0f), 1.0f);
        addFloat ("master", "Master", unit, 0.7f);

        return layout;
    }
}
