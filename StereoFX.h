#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

struct FxParams
{
    int distType = 0;
    float distAmt = 0.0f;
    float phaserRate = 0.4f, phaserDepth = 0.7f, phaserMix = 0.0f;
    float chorusRate = 0.6f, chorusDepth = 0.5f, chorusMix = 0.0f;
    float delayTime = 0.33f, delayFb = 0.35f, delayTone = 0.5f, delayMix = 0.0f;
    float reverbSize = 0.5f, reverbMix = 0.0f;
    float panRate = 1.0f, panDepth = 0.0f;
    float haasMs = 0.0f, width = 1.0f, master = 0.7f;
};

// Chain: drive > stereo phaser > stereo chorus > ping-pong delay > reverb
//        > auto-pan > Haas widener > mid/side width > master
class StereoFX
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;

        chorusL.assign ((size_t) (sr * 0.05) + 4, 0.0f);
        chorusR.assign ((size_t) (sr * 0.05) + 4, 0.0f);
        chorusW = 0;

        delayL.assign ((size_t) (sr * 1.6) + 4, 0.0f);
        delayR.assign ((size_t) (sr * 1.6) + 4, 0.0f);
        delayW = 0;
        dampL = dampR = 0.0f;

        haasBuf.assign ((size_t) (sr * 0.04) + 4, 0.0f);
        haasW = 0;

        delaySmooth.reset (sr, 0.25);
        delaySmooth.setCurrentAndTargetValue (0.33f * (float) sr);
        haasSmooth.reset (sr, 0.1);
        haasSmooth.setCurrentAndTargetValue (0.0f);

        reverb.prepare ({ sr, (juce::uint32) juce::jmax (1, maxBlock), 2 });
        reverb.reset();

        for (int ch = 0; ch < 2; ++ch)
        {
            phFb[ch] = 0.0f;
            phA[ch] = 0.0f;
            for (auto& z : phZ[ch]) z = 0.0f;
        }

        phLfo = chLfo = panLfo = 0.0f;
        phCount = 0;
        crushCount = 0;
        heldL = heldR = 0.0f;
        lastMaster = -1.0f;
    }

    void process (juce::AudioBuffer<float>& buffer, const FxParams& p)
    {
        if (buffer.getNumChannels() < 2 || delayL.empty())
            return;

        const int n = buffer.getNumSamples();
        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);
        const float fs = (float) sr;
        const float twoPi = juce::MathConstants<float>::twoPi;

        // 1. Drive
        if (p.distAmt > 0.001f)
        {
            if (p.distType == 3)
            {
                const float levels = std::exp2 (16.0f - p.distAmt * 13.0f);
                const int hold = 1 + (int) (p.distAmt * p.distAmt * 24.0f);

                for (int i = 0; i < n; ++i)
                {
                    if (++crushCount >= hold)
                    {
                        crushCount = 0;
                        heldL = std::round (L[i] * levels) / levels;
                        heldR = std::round (R[i] * levels) / levels;
                    }
                    L[i] = heldL;
                    R[i] = heldR;
                }
            }
            else
            {
                const float g = 1.0f + p.distAmt * 12.0f;
                const float comp = 1.0f / (1.0f + p.distAmt * 1.5f);

                for (int i = 0; i < n; ++i)
                {
                    L[i] = drive (L[i], g, comp, p.distType);
                    R[i] = drive (R[i], g, comp, p.distType);
                }
            }
        }

        // 2. Stereo phaser (left and right sweep 90 degrees apart)
        if (p.phaserMix > 0.001f)
        {
            for (int i = 0; i < n; ++i)
            {
                if (phCount <= 0)
                {
                    phCount = 16;
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        const float lfo = std::sin (twoPi * (phLfo + 0.25f * (float) ch));
                        const float f = juce::jmin (fs * 0.45f, 250.0f * std::exp2 ((0.5f + 0.5f * lfo) * p.phaserDepth * 4.5f));
                        const float t = std::tan (juce::MathConstants<float>::pi * f / fs);
                        phA[ch] = (t - 1.0f) / (t + 1.0f);
                    }
                }
                --phCount;

                phLfo += p.phaserRate / fs;
                if (phLfo >= 1.0f) phLfo -= 1.0f;

                for (int ch = 0; ch < 2; ++ch)
                {
                    float* s = ch == 0 ? L : R;
                    const float x = s[i];
                    float v = x + phFb[ch] * 0.55f;

                    for (int st = 0; st < 6; ++st)
                    {
                        const float y = phA[ch] * v + phZ[ch][st];
                        phZ[ch][st] = v - phA[ch] * y;
                        v = y;
                    }

                    phFb[ch] = v;
                    s[i] = x * (1.0f - 0.5f * p.phaserMix) + v * 0.5f * p.phaserMix;
                }
            }
        }

        // 3. Stereo chorus (two delay lines, modulated in quadrature)
        if (p.chorusMix > 0.001f)
        {
            const int size = (int) chorusL.size();

            for (int i = 0; i < n; ++i)
            {
                chorusL[(size_t) chorusW] = L[i];
                chorusR[(size_t) chorusW] = R[i];

                const float lfoL = std::sin (twoPi * chLfo);
                const float lfoR = std::sin (twoPi * (chLfo + 0.25f));
                chLfo += p.chorusRate / fs;
                if (chLfo >= 1.0f) chLfo -= 1.0f;

                const float dL = (7.0f + p.chorusDepth * 6.0f * (1.0f + lfoL)) * 0.001f * fs;
                const float dR = (7.0f + p.chorusDepth * 6.0f * (1.0f + lfoR)) * 0.001f * fs;

                const float wetL = readInterp (chorusL, chorusW, dL);
                const float wetR = readInterp (chorusR, chorusW, dR);

                L[i] = L[i] * (1.0f - 0.5f * p.chorusMix) + wetL * 0.7f * p.chorusMix;
                R[i] = R[i] * (1.0f - 0.5f * p.chorusMix) + wetR * 0.7f * p.chorusMix;

                chorusW = (chorusW + 1) % size;
            }
        }

        // 4. Ping-pong delay
        {
            const int size = (int) delayL.size();
            delaySmooth.setTargetValue (juce::jlimit (1.0f, (float) size - 3.0f, p.delayTime * fs));
            const float damp = 0.1f + 0.85f * p.delayTone;

            for (int i = 0; i < n; ++i)
            {
                const float d = delaySmooth.getNextValue();
                const float dl = readInterp (delayL, delayW, d);
                const float dr = readInterp (delayR, delayW, d);

                dampL += damp * (dl - dampL);
                dampR += damp * (dr - dampR);

                const float inL = L[i], inR = R[i];
                delayL[(size_t) delayW] = 0.5f * (inL + inR) + dampR * p.delayFb;
                delayR[(size_t) delayW] = dampL * p.delayFb;

                L[i] = inL + dl * p.delayMix;
                R[i] = inR + dr * p.delayMix;

                delayW = (delayW + 1) % size;
            }
        }

        // 5. Reverb
        if (p.reverbMix > 0.001f)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize   = 0.2f + 0.78f * p.reverbSize;
            rp.damping    = 0.4f;
            rp.wetLevel   = p.reverbMix;
            rp.dryLevel   = 1.0f - p.reverbMix * 0.5f;
            rp.width      = 1.0f;
            rp.freezeMode = 0.0f;
            reverb.setParameters (rp);

            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            reverb.process (ctx);
        }

        // 6. Auto-pan, Haas, width, master
        const int hsize = (int) haasBuf.size();
        haasSmooth.setTargetValue (juce::jlimit (0.0f, (float) hsize - 3.0f, p.haasMs * 0.001f * fs));
        if (lastMaster < 0.0f)
            lastMaster = p.master;

        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];

            if (p.panDepth > 0.001f)
            {
                const float pv = p.panDepth * std::sin (twoPi * panLfo);
                l *= pv > 0.0f ? 1.0f - pv : 1.0f;
                r *= pv < 0.0f ? 1.0f + pv : 1.0f;
            }
            panLfo += p.panRate / fs;
            if (panLfo >= 1.0f) panLfo -= 1.0f;

            const float hd = haasSmooth.getNextValue();
            haasBuf[(size_t) haasW] = r;
            if (hd > 1.0f)
                r = readInterp (haasBuf, haasW, hd);
            haasW = (haasW + 1) % hsize;

            const float m = 0.5f * (l + r);
            const float s = 0.5f * (l - r) * p.width;
            l = m + s;
            r = m - s;

            const float g = lastMaster + (p.master - lastMaster) * (float) i / (float) n;
            L[i] = l * g;
            R[i] = r * g;
        }
        lastMaster = p.master;
    }

private:
    static float drive (float x, float g, float comp, int type) noexcept
    {
        switch (type)
        {
            case 0:  return std::tanh (x * g) * comp;
            case 1:  return juce::jlimit (-1.0f, 1.0f, x * g) * comp;
            default: return std::sin (x * g * 1.2f) * 0.8f;
        }
    }

    static float readInterp (const std::vector<float>& buf, int writePos, float delay) noexcept
    {
        const int size = (int) buf.size();
        float pos = (float) writePos - delay;
        while (pos < 0.0f) pos += (float) size;
        int i0 = (int) pos;
        const float fr = pos - (float) i0;
        i0 %= size;
        const int i1 = (i0 + 1) % size;
        return buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]);
    }

    double sr = 44100.0;

    std::vector<float> chorusL, chorusR, delayL, delayR, haasBuf;
    int chorusW = 0, delayW = 0, haasW = 0;
    float dampL = 0.0f, dampR = 0.0f;

    juce::SmoothedValue<float> delaySmooth, haasSmooth;
    juce::dsp::Reverb reverb;

    float phZ[2][6] {};
    float phFb[2] {}, phA[2] {};
    float phLfo = 0.0f, chLfo = 0.0f, panLfo = 0.0f;
    int phCount = 0;

    int crushCount = 0;
    float heldL = 0.0f, heldR = 0.0f;
    float lastMaster = -1.0f;
};
