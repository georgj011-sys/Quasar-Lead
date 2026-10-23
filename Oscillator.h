#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <cstdint>

// One oscillator with seven very different synthesis models.
// "shape" and "color" mean something different for every model:
//   Saw      shape = detune of the two side saws, color = how loud the side saws are
//   Pulse    shape = pulse width,                 color = built-in PWM depth
//   Fold     shape = wavefold drive,              color = asymmetry (bias)
//   Sync     shape = hard-sync ratio,             color = blend with the master saw
//   FM       shape = modulation index,            color = modulator ratio (stepped)
//   Formant  shape = formant position,            color = focus (window sharpness)
//   Noise    shape = sample & hold rate,          color = smoothing
class LeadOsc
{
public:
    enum Type { Saw = 0, Pulse, Fold, Sync, FM, Formant, Noise };

    void setSampleRate (double sr) noexcept { pwmInc = 0.6f / (float) sr; }

    void resetPhases (uint32_t seed) noexcept
    {
        rng = seed * 747796405u + 2891336453u;
        phase = rand01(); p2 = rand01(); p3 = rand01();
        slave = 0.0f; mod = 0.0f; sh = 0.0f; shVal = 0.0f; lp = 0.0f;
        pwm = rand01();
    }

    void resetDeterministic() noexcept
    {
        phase = p2 = p3 = slave = mod = sh = shVal = lp = pwm = 0.0f;
        rng = 12345u;
    }

    float process (int type, float dt, float shape, float color) noexcept
    {
        dt = juce::jlimit (1.0e-6f, 0.45f, dt);
        const float twoPi = juce::MathConstants<float>::twoPi;
        float out = 0.0f;

        switch (type)
        {
            case Saw:
            {
                const float det = shape * 0.025f;
                const float d2 = dt * (1.0f + det), d3 = dt * (1.0f - det);
                const float s1 = 2.0f * phase - 1.0f - blep (phase, dt);
                const float s2 = 2.0f * p2 - 1.0f - blep (p2, d2);
                const float s3 = 2.0f * p3 - 1.0f - blep (p3, d3);
                out = (s1 + color * (s2 + s3)) / (1.0f + color * 1.3f);
                advance (phase, dt); advance (p2, d2); advance (p3, d3);
                break;
            }

            case Pulse:
            {
                float w = 0.5f - 0.45f * shape + color * 0.4f * std::sin (twoPi * pwm);
                w = juce::jlimit (0.03f, 0.97f, w);
                pwm += pwmInc;
                if (pwm >= 1.0f) pwm -= 1.0f;

                float v = phase < w ? 1.0f : -1.0f;
                v += blep (phase, dt);
                float t2 = phase + 1.0f - w;
                if (t2 >= 1.0f) t2 -= 1.0f;
                v -= blep (t2, dt);
                out = v - (2.0f * w - 1.0f);
                advance (phase, dt);
                break;
            }

            case Fold:
            {
                const float base = std::sin (twoPi * phase);
                const float x = base * (1.0f + shape * 7.0f) + color * 1.5f;
                out = std::sin (x * 1.5707963f);
                advance (phase, dt);
                break;
            }

            case Sync:
            {
                const float ratio = 1.0f + shape * 7.0f;
                const float ds = juce::jmin (dt * ratio, 0.45f);
                const float sl = 2.0f * slave - 1.0f - blep (slave, ds);
                const float ms = 2.0f * phase - 1.0f - blep (phase, dt);
                out = sl * (1.0f - 0.5f * color) + ms * 0.5f * color;

                slave += ds;
                if (slave >= 1.0f) slave -= 1.0f;
                phase += dt;
                if (phase >= 1.0f)
                {
                    phase -= 1.0f;
                    slave = phase * ratio;
                    slave -= std::floor (slave);
                }
                break;
            }

            case FM:
            {
                static constexpr float ratios[12] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
                const int ri = juce::jlimit (0, 11, (int) std::lround (color * 11.0f));
                const float index = shape * shape * 8.0f;
                out = std::sin (twoPi * phase + index * std::sin (twoPi * mod));
                advance (mod, juce::jmin (dt * ratios[ri], 0.49f));
                advance (phase, dt);
                break;
            }

            case Formant:
            {
                const float fr = 1.0f + shape * 11.0f;
                const float win = std::pow (1.0f - phase, 1.0f + color * 6.0f);
                out = std::sin (twoPi * phase * fr) * win * 1.6f;
                advance (phase, dt);
                break;
            }

            case Noise:
            default:
            {
                const float mult = 0.5f + shape * shape * 24.0f;
                sh += dt * mult;
                if (sh >= 1.0f)
                {
                    sh -= std::floor (sh);
                    shVal = rand01() * 2.0f - 1.0f;
                }
                lp += (shVal - lp) * (1.0f - color * 0.97f);
                out = lp;
                advance (phase, dt);
                break;
            }
        }

        return out;
    }

    // Renders one cycle for the on-screen display.
    static void renderPreview (int type, float shape, float color, float* dest, int n)
    {
        LeadOsc o;
        o.resetDeterministic();
        const float dt = 1.0f / (float) n;

        for (int i = 0; i < n; ++i)
            o.process (type, dt, shape, color);

        for (int i = 0; i < n; ++i)
            dest[i] = o.process (type, dt, shape, color);
    }

private:
    static float blep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt;          return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    static void advance (float& p, float d) noexcept
    {
        p += d;
        if (p >= 1.0f) p -= 1.0f;
    }

    float rand01() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) (rng >> 8) / 16777216.0f;
    }

    float phase = 0.0f, p2 = 0.0f, p3 = 0.0f, slave = 0.0f, mod = 0.0f;
    float sh = 0.0f, shVal = 0.0f, lp = 0.0f, pwm = 0.0f, pwmInc = 0.0f;
    uint32_t rng = 1u;
};
