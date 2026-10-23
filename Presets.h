#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

// Values in real units. Unlisted parameters return to their defaults.
// Osc types: 0 Saw, 1 Pulse, 2 Fold, 3 Sync, 4 FM, 5 Formant, 6 Noise
// Filter:    0 LP12, 1 LP24, 2 BP, 3 HP, 4 Notch, 5 Comb
// LFO wave:  0 Sine, 1 Triangle, 2 Saw, 3 Square, 4 S&H, 5 Drift
// Drive:     0 Soft, 1 Hard, 2 Fold, 3 Crush
// Mode:      0 Mono, 1 Legato, 2 Poly
struct PresetDef
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};

inline const std::vector<PresetDef>& getFactoryPresets()
{
    static const std::vector<PresetDef> presets {
        { "Init", {} },

        { "Classic Saw Lead", {
            { "a_type", 0 }, { "a_shape", 0.35f }, { "a_color", 0.6f },
            { "b_type", 0 }, { "b_fine", 7 }, { "b_level", 0.5f },
            { "filter_type", 1 }, { "cutoff", 2500 }, { "reso", 0.25f }, { "fenv_amt", 0.4f },
            { "glide", 0.06f }, { "lfo_rate", 5.5f }, { "lfo_pitch", 0.08f },
            { "chorus_mix", 0.2f }, { "delay_mix", 0.2f }, { "reverb_mix", 0.15f } } },

        { "Square Hero", {
            { "a_type", 1 }, { "a_shape", 0.1f }, { "a_color", 0.2f },
            { "b_type", 1 }, { "b_octave", -1 }, { "b_shape", 0.4f }, { "b_level", 0.4f }, { "b_fine", 0 },
            { "filter_type", 0 }, { "cutoff", 3500 }, { "reso", 0.2f },
            { "mode", 1 }, { "glide", 0.12f }, { "delay_mix", 0.25f }, { "delay_time", 0.375f } } },

        { "Sync Scream", {
            { "a_type", 3 }, { "a_shape", 0.55f }, { "a_color", 0.2f },
            { "b_type", 0 }, { "b_level", 0.3f }, { "b_fine", 10 },
            { "filter_type", 1 }, { "cutoff", 5000 }, { "reso", 0.4f }, { "fenv_amt", 0.3f },
            { "lfo_wave", 1 }, { "lfo_rate", 0.4f }, { "lfo_osc", 0.5f }, { "lfo_pitch", 0.0f },
            { "dist_type", 0 }, { "dist_amt", 0.5f }, { "glide", 0.05f },
            { "delay_mix", 0.3f }, { "reverb_mix", 0.15f } } },

        { "FM Glass Bell", {
            { "a_type", 4 }, { "a_shape", 0.5f }, { "a_color", 0.545f }, { "a_pan", -0.5f },
            { "b_type", 4 }, { "b_shape", 0.25f }, { "b_color", 0.273f }, { "b_octave", 1 }, { "b_level", 0.4f },
            { "b_pan", 0.5f }, { "b_fine", 3 },
            { "filter_type", 0 }, { "cutoff", 9000 }, { "fenv_amt", 0.1f }, { "sub_level", 0 },
            { "amp_attack", 0.002f }, { "amp_decay", 1.2f }, { "amp_sustain", 0.3f }, { "amp_release", 0.9f },
            { "mode", 2 }, { "lfo_pitch", 0 }, { "chorus_mix", 0.3f }, { "reverb_mix", 0.35f }, { "reverb_size", 0.7f } } },

        { "Formant Vox", {
            { "a_type", 5 }, { "a_shape", 0.35f }, { "a_color", 0.4f },
            { "b_type", 5 }, { "b_shape", 0.6f }, { "b_color", 0.2f }, { "b_level", 0.5f }, { "b_fine", -8 },
            { "filter_type", 0 }, { "cutoff", 6000 }, { "fenv_amt", 0.1f },
            { "lfo_rate", 0.7f }, { "lfo_osc", 0.3f }, { "lfo_pitch", 0.05f },
            { "mode", 1 }, { "glide", 0.18f }, { "chorus_mix", 0.4f }, { "reverb_mix", 0.25f } } },

        { "Robot Gargle", {
            { "a_type", 6 }, { "a_shape", 0.05f }, { "a_color", 0.1f },
            { "b_type", 5 }, { "b_shape", 0.5f }, { "b_level", 0.5f },
            { "filter_type", 5 }, { "cutoff", 262 }, { "keytrack", 1.0f }, { "reso", 0.85f }, { "fenv_amt", 0 },
            { "lfo_wave", 4 }, { "lfo_rate", 8 }, { "lfo_cutoff", 0.4f }, { "lfo_pitch", 0 },
            { "dist_type", 3 }, { "dist_amt", 0.4f }, { "delay_mix", 0.2f } } },

        { "Laser Duel", {
            { "a_type", 0 }, { "a_shape", 0.2f }, { "b_type", 1 }, { "b_level", 0.45f },
            { "penv_amt", 24 }, { "penv_decay", 0.15f },
            { "filter_type", 1 }, { "cutoff", 6000 }, { "fenv_amt", 0.5f },
            { "delay_time", 0.25f }, { "delay_fb", 0.6f }, { "delay_mix", 0.45f },
            { "pan_depth", 0.6f }, { "pan_rate", 4 }, { "dist_type", 1 }, { "dist_amt", 0.3f } } },

        { "Alien Ring", {
            { "a_type", 4 }, { "a_shape", 0.3f }, { "a_color", 0.8f },
            { "b_type", 4 }, { "b_shape", 0 }, { "b_semi", 7 }, { "b_level", 0.8f },
            { "ring", 0.8f }, { "lfo_rate", 6 }, { "lfo_pitch", 0.3f },
            { "filter_type", 0 }, { "cutoff", 8000 }, { "fenv_amt", 0 },
            { "phaser_mix", 0.5f }, { "reverb_mix", 0.4f }, { "reverb_size", 0.8f } } },

        { "Bit Dragon", {
            { "a_type", 2 }, { "a_shape", 0.7f }, { "a_color", 0.4f },
            { "b_type", 0 }, { "b_octave", -1 }, { "b_level", 0.5f },
            { "dist_type", 3 }, { "dist_amt", 0.65f },
            { "filter_type", 1 }, { "cutoff", 2000 }, { "reso", 0.5f }, { "fenv_amt", 0.6f },
            { "lfo_wave", 1 }, { "lfo_rate", 3 }, { "lfo_cutoff", 0.3f }, { "lfo_pitch", 0 }, { "glide", 0.1f } } },

        { "Underwater Theremin", {
            { "a_type", 2 }, { "a_shape", 0.05f }, { "a_color", 0 }, { "b_level", 0 }, { "sub_level", 0 },
            { "mode", 1 }, { "glide", 0.35f }, { "lfo_rate", 5 }, { "lfo_pitch", 0.25f },
            { "filter_type", 0 }, { "cutoff", 1200 }, { "fenv_amt", 0 },
            { "phaser_mix", 0.6f }, { "phaser_rate", 0.2f }, { "chorus_mix", 0.5f },
            { "reverb_mix", 0.5f }, { "reverb_size", 0.85f }, { "width", 1.6f } } },

        { "Wide Haas Pluck", {
            { "a_type", 1 }, { "a_shape", 0.3f }, { "a_color", 0.5f }, { "a_pan", -1 },
            { "b_type", 0 }, { "b_octave", 1 }, { "b_level", 0.35f }, { "b_pan", 1 },
            { "amp_attack", 0.001f }, { "amp_decay", 0.35f }, { "amp_sustain", 0 }, { "amp_release", 0.3f },
            { "filter_type", 1 }, { "cutoff", 800 }, { "fenv_amt", 0.7f }, { "flt_decay", 0.25f }, { "flt_sustain", 0 },
            { "haas", 18 }, { "width", 1.5f }, { "mode", 2 }, { "lfo_pitch", 0 }, { "delay_mix", 0.3f } } },

        { "Glass Wasp", {
            { "a_type", 3 }, { "a_shape", 0.9f }, { "a_color", 0 },
            { "b_type", 1 }, { "b_shape", 0.8f }, { "b_color", 0.6f }, { "b_octave", 1 }, { "b_level", 0.4f },
            { "filter_type", 3 }, { "cutoff", 600 }, { "reso", 0.3f }, { "fenv_amt", 0 },
            { "lfo_wave", 1 }, { "lfo_rate", 9 }, { "lfo_osc", 0.6f }, { "lfo_pitch", 0 },
            { "dist_type", 2 }, { "dist_amt", 0.3f }, { "pan_depth", 0.4f }, { "pan_rate", 7 } } },

        { "Kazoo From Space", {
            { "a_type", 5 }, { "a_shape", 0.15f }, { "a_color", 0.8f },
            { "b_type", 1 }, { "b_shape", 0.6f }, { "b_level", 0.3f },
            { "filter_type", 2 }, { "cutoff", 1500 }, { "reso", 0.5f }, { "keytrack", 1 }, { "fenv_amt", 0.1f },
            { "lfo_rate", 6.5f }, { "lfo_pitch", 0.12f },
            { "phaser_mix", 0.4f }, { "delay_mix", 0.25f }, { "mode", 1 }, { "glide", 0.1f } } },

        { "Broken Radio", {
            { "a_type", 6 }, { "a_shape", 0.4f }, { "a_color", 0.6f },
            { "b_type", 4 }, { "b_shape", 0.7f }, { "b_color", 0.3f }, { "b_level", 0.5f },
            { "filter_type", 2 }, { "cutoff", 2200 }, { "reso", 0.6f }, { "fenv_amt", 0 },
            { "lfo_wave", 5 }, { "lfo_rate", 0.4f }, { "lfo_cutoff", 0.6f }, { "lfo_pitch", 0.15f },
            { "dist_type", 3 }, { "dist_amt", 0.5f }, { "chorus_mix", 0.3f } } },

        { "Acid Comb", {
            { "a_type", 0 }, { "a_shape", 0 }, { "a_color", 0 }, { "b_level", 0 },
            { "filter_type", 5 }, { "cutoff", 300 }, { "reso", 0.7f }, { "keytrack", 1 }, { "fenv_amt", 0.6f },
            { "flt_decay", 0.25f }, { "flt_sustain", 0 },
            { "dist_type", 0 }, { "dist_amt", 0.4f }, { "glide", 0.08f }, { "lfo_pitch", 0 },
            { "delay_mix", 0.3f }, { "delay_time", 0.19f } } },

        { "Neon Chorus Lead", {
            { "a_type", 0 }, { "a_shape", 0.6f }, { "a_color", 0.9f },
            { "b_type", 0 }, { "b_octave", 1 }, { "b_shape", 0.5f }, { "b_color", 0.5f }, { "b_level", 0.35f },
            { "filter_type", 0 }, { "cutoff", 4500 },
            { "chorus_mix", 0.6f }, { "chorus_depth", 0.8f }, { "reverb_mix", 0.3f },
            { "width", 1.8f }, { "pan_depth", 0.2f }, { "pan_rate", 0.5f } } },

        { "Rubber Bass Lead", {
            { "a_type", 1 }, { "a_shape", 0 }, { "a_color", 0 }, { "b_level", 0 },
            { "sub_type", 0 }, { "sub_level", 0.7f },
            { "filter_type", 1 }, { "cutoff", 400 }, { "reso", 0.5f }, { "fenv_amt", 0.7f },
            { "flt_decay", 0.2f }, { "flt_sustain", 0.1f },
            { "glide", 0.05f }, { "lfo_pitch", 0 }, { "dist_type", 0 }, { "dist_amt", 0.3f },
            { "chorus_mix", 0 }, { "delay_mix", 0 }, { "reverb_mix", 0.05f } } },

        { "Chip Arcade", {
            { "a_type", 1 }, { "a_shape", 0.5f }, { "a_color", 0 },
            { "b_type", 1 }, { "b_shape", 0.75f }, { "b_color", 0 }, { "b_octave", 1 }, { "b_level", 0.3f },
            { "lfo_wave", 3 }, { "lfo_rate", 12 }, { "lfo_pitch", 0.5f },
            { "dist_type", 3 }, { "dist_amt", 0.3f },
            { "filter_type", 0 }, { "cutoff", 12000 }, { "fenv_amt", 0 }, { "sub_level", 0 },
            { "chorus_mix", 0 }, { "delay_mix", 0.2f }, { "delay_time", 0.125f } } },
    };

    return presets;
}
