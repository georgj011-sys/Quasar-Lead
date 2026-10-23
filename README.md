# Quasar Lead

Two-oscillator solo synthesizer (VST3 + Standalone for Windows).

## Upload and build

1. Create a new repository on GitHub (Public).
2. Click "uploading an existing file" and drag in ALL files from this folder. Commit.
3. Move build.yml into place: click build.yml > pencil icon > click in the name field right before "build.yml" > type `.github/workflows/` > Commit changes.
4. Open the Actions tab and wait for the green check (about 10-15 minutes).
5. Download "QuasarLead-Windows" under Artifacts, extract it, and copy "Quasar Lead.vst3" to C:\Program Files\Common Files\VST3.

## Sound engine

Each oscillator has seven models with their own two character knobs
(the knob names change on the panel when you switch models):

| Model   | Shape   | Color  |
|---------|---------|--------|
| Saw     | Detune  | Stack  |
| Pulse   | Width   | PWM    |
| Fold    | Fold    | Bias   |
| Sync    | Sync    | Blend  |
| FM      | Index   | Ratio  |
| Formant | Formant | Focus  |
| Noise   | Rate    | Smooth |

Plus: sub oscillator, ring modulation, Mono / Legato / Poly with glide,
filter with LP12, LP24, band pass, high pass, notch and a tuned comb mode,
filter and amp envelopes, LFO with six waves, pitch envelope.

Stereo effects: stereo phaser, stereo chorus, ping-pong delay, reverb,
auto-pan, Haas widener, mid/side width, plus per-oscillator pan.
