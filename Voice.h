#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>
#include "Oscillator.h"

struct LeadParams
{
    struct Osc
    {
        int type = 0;
        float shape = 0.3f, color = 0.5f, octave = 0.0f, semi = 0.0f, fine = 0.0f, level = 0.8f, pan = 0.0f;
    };

    Osc osc[2];
    int subType = 0;
    float subLevel = 0.0f, ring = 0.0f;

    int filterType = 1;
    float cutoff = 3000.0f, reso = 0.3f, envAmt = 0.4f, keytrack = 0.5f, drive = 0.2f;

    juce::ADSR::Parameters ampEnv { 0.005f, 0.3f, 0.8f, 0.25f };
    juce::ADSR::Parameters fltEnv { 0.002f, 0.4f, 0.3f, 0.3f };

    int lfoWave = 0;
    float lfoRate = 5.0f, lfoPitch = 0.1f, lfoCutoff = 0.0f, lfoOsc = 0.0f;

    float penvAmt = 0.0f, penvDecay = 0.1f;
    float glide = 0.05f;
};

class LeadVoice
{
public:
    static constexpr int chunk = 16;

    enum FilterType { LP12 = 0, LP24, BP, HP, Notch, Comb };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        ampEnv.setSampleRate (sampleRate);
        fltEnv.setSampleRate (sampleRate);
        oscA.setSampleRate (sampleRate);
        oscB.setSampleRate (sampleRate);

        combSize = (int) (sampleRate / 15.0) + 4;
        combL.assign ((size_t) combSize, 0.0f);
        combR.assign ((size_t) combSize, 0.0f);
        combW = 0;

        resetFilters();
        kill();
    }

    void start (int note, float vel, float fromPitch, const LeadParams& p, uint32_t seed)
    {
        if (! isActive())
        {
            oscA.resetPhases (seed);
            oscB.resetPhases (seed * 7u + 3u);
            subPhase = 0.0f;
            resetFilters();
        }

        targetPitch = (float) note;
        currentPitch = fromPitch;
        currentNote = note;
        velocity = vel;
        lfoPhase = 0.0f;
        penv = 1.0f;
        gate = true;

        ampEnv.setParameters (p.ampEnv);
        fltEnv.setParameters (p.fltEnv);
        ampEnv.noteOn();
        fltEnv.noteOn();
    }

    void glideTo (int note) noexcept
    {
        targetPitch = (float) note;
        currentNote = note;
    }

    void release()
    {
        gate = false;
        ampEnv.noteOff();
        fltEnv.noteOff();
    }

    void kill()
    {
        ampEnv.reset();
        fltEnv.reset();
        gate = false;
        currentNote = -1;
    }

    bool isActive() const noexcept   { return ampEnv.isActive(); }
    bool isGateOn() const noexcept   { return gate; }
    int getNote() const noexcept     { return currentNote; }
    float getPitch() const noexcept  { return currentPitch; }

    juce::uint64 age = 0;

    void render (float* L, float* R, int start, int num, const LeadParams& p, float bend, float modWheel)
    {
        if (! isActive() || num <= 0)
            return;

        ampEnv.setParameters (p.ampEnv);
        fltEnv.setParameters (p.fltEnv);

        const float fsr = (float) sr;
        const float glideCoeff = p.glide <= 0.0005f ? 1.0f : 1.0f - std::exp (-(float) chunk / (p.glide * fsr));
        const float penvCoeff = std::exp (-(float) chunk / (juce::jmax (0.001f, p.penvDecay) * fsr));
        const float velGain = 0.45f + 0.55f * velocity;

        float gL[2], gR[2];
        for (int o = 0; o < 2; ++o)
        {
            const float pan = juce::jlimit (-1.0f, 1.0f, p.osc[o].pan);
            gL[o] = std::sqrt (0.5f * (1.0f - pan)) * p.osc[o].level;
            gR[o] = std::sqrt (0.5f * (1.0f + pan)) * p.osc[o].level;
        }

        const float ringAmt = p.ring;
        const float drv = 1.0f + p.drive * 8.0f;
        const float drvComp = 1.0f / (0.7f * std::sqrt (drv));
        const float subGain = p.subLevel * 0.8f;

        while (num > 0)
        {
            const int n = juce::jmin (chunk, num);

            currentPitch += (targetPitch - currentPitch) * glideCoeff;

            const float lfo = tickLfo (p, n);
            const float pe = penv * p.penvAmt;
            penv *= penvCoeff;

            const float vib = lfo * (p.lfoPitch * p.lfoPitch * 12.0f + modWheel * 0.6f);
            const float basePitch = currentPitch + bend + pe + vib;

            float dt[2], shp[2];
            for (int o = 0; o < 2; ++o)
            {
                const auto& op = p.osc[o];
                const float semis = basePitch + op.octave * 12.0f + op.semi + op.fine * 0.01f;
                dt[o] = 440.0f * std::exp2 ((semis - 69.0f) / 12.0f) / fsr;
                shp[o] = juce::jlimit (0.0f, 1.0f, op.shape + lfo * p.lfoOsc * 0.5f);
            }

            const float subSemis = basePitch - (p.subType == 2 ? 24.0f : 12.0f);
            const float subDt = juce::jmin (0.45f, 440.0f * std::exp2 ((subSemis - 69.0f) / 12.0f) / fsr);

            // Filter envelope and cutoff at control rate
            const float fe = fltEnv.getNextSample();
            for (int k = 1; k < n; ++k)
                fltEnv.getNextSample();

            float cutoff = p.cutoff * std::exp2 (p.envAmt * fe * 7.0f
                                                 + (basePitch - 60.0f) / 12.0f * p.keytrack
                                                 + lfo * p.lfoCutoff * 3.0f);
            cutoff = juce::jlimit (20.0f, fsr * 0.45f, cutoff);

            const float g = std::tan (juce::MathConstants<float>::pi * cutoff / fsr);
            const float k = 1.414f - 1.38f * p.reso;
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;

            const float combDelay = juce::jlimit (2.0f, (float) combSize - 3.0f, fsr / cutoff);
            const float combFb = p.reso * 0.97f;

            for (int i = 0; i < n; ++i)
            {
                const float a = oscA.process (p.osc[0].type, dt[0], shp[0], p.osc[0].color);
                const float b = oscB.process (p.osc[1].type, dt[1], shp[1], p.osc[1].color);
                const float rm = a * b * p.osc[0].level * p.osc[1].level * 1.4f;

                float l = (a * gL[0] + b * gL[1]) * (1.0f - ringAmt) + rm * ringAmt;
                float r = (a * gR[0] + b * gR[1]) * (1.0f - ringAmt) + rm * ringAmt;

                if (subGain > 0.0f)
                {
                    const float s = p.subType == 1 ? (subPhase < 0.5f ? 1.0f : -1.0f)
                                                   : std::sin (juce::MathConstants<float>::twoPi * subPhase);
                    l += s * subGain;
                    r += s * subGain;
                }
                subPhase += subDt;
                if (subPhase >= 1.0f) subPhase -= 1.0f;

                // DC blocker
                const float yl = l - dcxL + 0.995f * dcyL; dcxL = l; dcyL = yl;
                const float yr = r - dcxR + 0.995f * dcyR; dcxR = r; dcyR = yr;

                l = std::tanh (yl * 0.7f * drv) * drvComp;
                r = std::tanh (yr * 0.7f * drv) * drvComp;

                l = filterSample (l, 0, a1, a2, a3, k, p.filterType, combDelay, combFb);
                r = filterSample (r, 1, a1, a2, a3, k, p.filterType, combDelay, combFb);

                if (p.filterType == Comb)
                    combW = (combW + 1) % combSize;

                const float amp = ampEnv.getNextSample() * velGain * 0.5f;
                L[start + i] += l * amp;
                R[start + i] += r * amp;
            }

            start += n;
            num -= n;

            if (! ampEnv.isActive())
            {
                currentNote = -1;
                gate = false;
                break;
            }
        }
    }

private:
    struct SvfState { float ic1 = 0.0f, ic2 = 0.0f; };

    static void svf (SvfState& s, float x, float a1, float a2, float a3, float k,
                     float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = x - s.ic2;
        const float v1 = a1 * s.ic1 + a2 * v3;
        const float v2 = s.ic2 + a2 * s.ic1 + a3 * v3;
        s.ic1 = 2.0f * v1 - s.ic1;
        s.ic2 = 2.0f * v2 - s.ic2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    float filterSample (float x, int ch, float a1, float a2, float a3, float k,
                        int type, float combDelay, float combFb) noexcept
    {
        if (type == Comb)
        {
            auto& buf = ch == 0 ? combL : combR;
            float& damp = ch == 0 ? combLpL : combLpR;

            float pos = (float) combW - combDelay;
            if (pos < 0.0f) pos += (float) combSize;
            int i0 = (int) pos;
            const float fr = pos - (float) i0;
            i0 %= combSize;
            const int i1 = (i0 + 1) % combSize;
            const float delayed = buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]);

            damp += 0.6f * (delayed - damp);
            const float y = x * 0.5f + combFb * damp;
            buf[(size_t) combW] = y;
            return std::tanh (y * (1.0f - combFb * 0.85f) * 1.5f);
        }

        float lp, bp, hp;
        svf (svfA[ch], x, a1, a2, a3, k, lp, bp, hp);

        switch (type)
        {
            case LP12:  return lp;
            case LP24:
            {
                float lp2, bp2, hp2;
                svf (svfB[ch], lp, a1, a2, a3, k, lp2, bp2, hp2);
                return lp2;
            }
            case BP:    return bp * (k * 0.75f + 0.5f);
            case HP:    return hp;
            case Notch: return x - k * bp;
            default:    return lp;
        }
    }

    float tickLfo (const LeadParams& p, int n) noexcept
    {
        float val = 0.0f;

        switch (p.lfoWave)
        {
            case 0:  val = std::sin (juce::MathConstants<float>::twoPi * lfoPhase); break;
            case 1:  val = 4.0f * std::abs (lfoPhase - 0.5f) - 1.0f; break;
            case 2:  val = 1.0f - 2.0f * lfoPhase; break;
            case 3:  val = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
            case 4:  val = lfoHold; break;
            default: val = lfoDrift; break;
        }

        lfoPhase += p.lfoRate * (float) n / (float) sr;
        if (lfoPhase >= 1.0f)
        {
            lfoPhase -= std::floor (lfoPhase);
            lfoHold = rng.nextFloat() * 2.0f - 1.0f;
            driftTarget = rng.nextFloat() * 2.0f - 1.0f;
        }
        lfoDrift += (driftTarget - lfoDrift) * juce::jmin (1.0f, p.lfoRate * (float) n / (float) sr * 3.0f);

        return val;
    }

    void resetFilters()
    {
        svfA[0] = svfA[1] = svfB[0] = svfB[1] = SvfState();
        std::fill (combL.begin(), combL.end(), 0.0f);
        std::fill (combR.begin(), combR.end(), 0.0f);
        combLpL = combLpR = 0.0f;
        dcxL = dcyL = dcxR = dcyR = 0.0f;
    }

    double sr = 44100.0;
    LeadOsc oscA, oscB;
    juce::ADSR ampEnv, fltEnv;
    juce::Random rng;

    SvfState svfA[2], svfB[2];
    std::vector<float> combL, combR;
    int combSize = 4, combW = 0;
    float combLpL = 0.0f, combLpR = 0.0f;
    float dcxL = 0.0f, dcyL = 0.0f, dcxR = 0.0f, dcyR = 0.0f;

    float targetPitch = 60.0f, currentPitch = 60.0f, velocity = 1.0f;
    int currentNote = -1;
    bool gate = false;

    float subPhase = 0.0f;
    float lfoPhase = 0.0f, lfoHold = 0.0f, lfoDrift = 0.0f, driftTarget = 0.0f;
    float penv = 0.0f;
};
