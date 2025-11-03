#pragma once
#include <JuceHeader.h>

class ClapVoice
{
public:
    void prepare(double sr) { sampleRate = sr; reset(); }

    void setParameters(float pitchSemi, float decaySec, float tone, float drive)
    {
        decayTime = juce::jlimit(0.05f, 1.5f, decaySec);
        toneAmount = juce::jlimit(0.0f, 1.0f, tone);
        driveAmount = juce::jlimit(0.0f, 1.0f, drive);
        ampEnvMult = std::exp(-1.0f / (decayTime * (float)sampleRate));
        float cutoff = juce::jmap(toneAmount, 1500.0f, 6000.0f);
        float x = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)sampleRate);
        lp_a = 1.0f - x; lp_b = x;
        pulses = 4; pulseGapSamples = (int) (0.008f * sampleRate);
        juce::ignoreUnused(pitchSemi);
    }

    void noteOn(float velocity)
    {
        active = true;
        ampEnv = juce::jlimit(0.0f, 1.0f, velocity);
        noiseState = 0x7654321u;
        currentPulse = 0; pulseCountdown = 0;
    }

    void noteOnWithDelay(float velocity, int delaySamples)
    {
        startDelaySamples = juce::jmax(0, delaySamples);
        noteOn(velocity);
    }

    void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        if (!active) return;
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = startSample + i;
            if (startDelaySamples > 0) { --startDelaySamples; continue; }
            if (currentPulse >= pulses && ampEnv < 1e-4f) { active = false; break; }

            float out = 0.0f;
            if (pulseCountdown <= 0 && currentPulse < pulses)
            {
                // emit short noise pulse
                for (int k = 0; k < 4; ++k)
                {
                    float n = (float)((noiseState = noiseState * 1103515245u + 12345u) & 0x00ffffff) / (float)0x00ffffff;
                    n = n * 2.0f - 1.0f;
                    lp_y = lp_a * n + lp_b * lp_y;
                    out += lp_y * ampEnv * 0.25f;
                }
                ++currentPulse;
                pulseCountdown = pulseGapSamples;
            }
            else
            {
                --pulseCountdown;
            }

            if (driveAmount > 0.001f)
                out = juce::jmap(driveAmount, out, std::tanh(out * (1.0f + 2.5f * driveAmount)));

            L[idx] += out;
            if (R) R[idx] += out;

            ampEnv *= ampEnvMult;
        }
    }

    void reset()
    {
        active = false; startDelaySamples = 0; currentPulse = 0; pulseCountdown = 0;
        ampEnv = 0.0f; ampEnvMult = 0.995f;
        lp_y = 0.0f; lp_a = 1.0f; lp_b = 0.0f;
        toneAmount = 0.5f; decayTime = 0.3f; driveAmount = 0.0f;
        pulses = 4; pulseGapSamples = 0;
    }

private:
    double sampleRate { 44100.0 };
    bool active { false };

    float decayTime { 0.3f };
    float toneAmount { 0.5f };
    float driveAmount { 0.0f };
    float ampEnv { 0.0f }, ampEnvMult { 0.995f };

    float lp_y { 0.0f }, lp_a { 1.0f }, lp_b { 0.0f };
    unsigned int noiseState { 1u };

    int startDelaySamples { 0 };
    int pulses { 4 }, currentPulse { 0 }, pulseGapSamples { 0 }, pulseCountdown { 0 };
};