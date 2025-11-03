#pragma once
#include <JuceHeader.h>

class BDVoice
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        reset();
    }

    void setParameters(float pitchSemitones, float decaySeconds, float tone, float drive)
    {
        baseFreq = 55.0f * std::pow(2.0f, pitchSemitones / 12.0f);
        decayTime = juce::jlimit(0.01f, 4.0f, decaySeconds);
        toneAmount = juce::jlimit(0.0f, 1.0f, tone);
        driveAmount = juce::jlimit(0.0f, 1.0f, drive);
        ampEnvMult = std::exp(-1.0f / (decayTime * (float)sampleRate));
        float cutoff = juce::jmap(toneAmount, 400.0f, 4000.0f);
        float x = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)sampleRate);
        lp_a = 1.0f - x;
        lp_b = x;
    }

    void noteOn(float velocity)
    {
        noteOnWithDelay(velocity, 0);
    }

    void noteOnWithDelay(float velocity, int delaySamples)
    {
        startDelaySamples = juce::jmax(0, delaySamples);
        active = true;
        ampEnv = juce::jlimit(0.0f, 1.0f, velocity);
        sweepPhase = 0.0f;
        clickSamples = (int)(0.003f * sampleRate);
        sweepStartFreq = baseFreq * 1.6f;
        sweepEndFreq = baseFreq;
    }

    void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        if (!active) return;
        auto* left  = buffer.getWritePointer(0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            int idx = startSample + i;

            if (startDelaySamples > 0)
            {
                --startDelaySamples;
                continue;
            }

            float sweepDur = 0.02f;
            float sweepAlpha = juce::jlimit(0.0f, 1.0f, sweepPhase / (sweepDur * (float)sampleRate));
            float instFreq = sweepEndFreq + (sweepStartFreq - sweepEndFreq) * std::exp(-6.0f * sweepAlpha);
            phase += instFreq / (float)sampleRate;
            if (phase >= 1.0f) phase -= 1.0f;
            float s = std::sin(phase * juce::MathConstants<float>::twoPi);

            lp_y = lp_a * s + lp_b * lp_y;
            float out = lp_y * ampEnv;

            if (clickSamples > 0)
            {
                out += 0.25f * ampEnv * (float)clickSamples / (0.003f * (float)sampleRate);
                --clickSamples;
            }

            if (driveAmount > 0.001f)
                out = juce::jmap(driveAmount, out, std::tanh(out * (1.0f + 2.5f * driveAmount)));

            left[idx] += out;
            if (right) right[idx] += out;

            ampEnv *= ampEnvMult;
            sweepPhase += 1.0f;
            if (ampEnv < 1e-4f)
            {
                active = false;
                break;
            }
        }
    }

    void reset()
    {
        active = false;
        phase = 0.0f;
        ampEnv = 0.0f;
        ampEnvMult = 0.995f;
        lp_y = 0.0f; lp_a = 1.0f; lp_b = 0.0f;
        clickSamples = 0;
        startDelaySamples = 0;
        sweepPhase = 0.0f;
        baseFreq = 55.0f; decayTime = 0.5f; toneAmount = 0.5f; driveAmount = 0.0f;
        sweepStartFreq = 80.0f; sweepEndFreq = 55.0f;
    }

private:
    double sampleRate { 44100.0 };
    bool active { false };

    float phase { 0.0f };

    float ampEnv { 0.0f };
    float ampEnvMult { 0.995f };

    float lp_y { 0.0f }, lp_a { 1.0f }, lp_b { 0.0f };

    int clickSamples { 0 };
    int startDelaySamples { 0 };

    float sweepPhase { 0.0f };
    float baseFreq { 55.0f };
    float decayTime { 0.5f };
    float toneAmount { 0.5f };
    float driveAmount { 0.0f };
    float sweepStartFreq { 80.0f };
    float sweepEndFreq { 55.0f };
};