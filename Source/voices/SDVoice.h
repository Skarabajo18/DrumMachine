#pragma once
#include <JuceHeader.h>

class SDVoice
{
public:
    void prepare(double sr) { sampleRate = sr; reset(); }

    void setParameters(float pitchSemi, float decaySec, float tone, float drive)
    {
        baseFreq = 180.0f * std::pow(2.0f, pitchSemi / 12.0f);
        decayTime = juce::jlimit(0.02f, 2.5f, decaySec);
        toneAmount = juce::jlimit(0.0f, 1.0f, tone);
        driveAmount = juce::jlimit(0.0f, 1.0f, drive);
        bodyEnvMult = std::exp(-1.0f / (decayTime * (float)sampleRate));
        snappyEnvMult = std::exp(-1.0f / (0.03f * (float)sampleRate));
        // bandpass coeff approx for tone: center 1k..3k
        float center = juce::jmap(toneAmount, 1000.0f, 3000.0f);
        float Q = 0.7f;
        float w0 = 2.0f * juce::MathConstants<float>::pi * center / (float)sampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);
        b0 = alpha; b1 = 0.0f; b2 = -alpha;
        a0 = 1.0f + alpha; a1 = -2.0f * std::cos(w0); a2 = 1.0f - alpha;
    }

    void noteOn(float velocity)
    {
        active = true;
        bodyEnv = juce::jlimit(0.0f, 1.0f, velocity);
        snappyEnv = bodyEnv;
        bodyPhase = 0.0f;
        noiseState = 0x1234567u;
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

            // Body: lightly inharmonic ring
            float f1 = baseFreq;
            float f2 = baseFreq * 1.5f;
            bodyPhase += f1 / (float)sampleRate;
            if (bodyPhase >= 1.0f) bodyPhase -= 1.0f;
            float s1 = std::sin(bodyPhase * juce::MathConstants<float>::twoPi);
            float s2 = std::sin(bodyPhase * juce::MathConstants<float>::twoPi * (f2/f1));
            float body = (s1 + 0.6f * s2) * bodyEnv;

            // Snappy: filtered noise burst
            float n = (float)((noiseState = noiseState * 1664525u + 1013904223u) & 0x00ffffff) / (float)0x00ffffff;
            n = n * 2.0f - 1.0f;
            float x = n * snappyEnv;
            float y = (b0/a0) * x + (b1/a0) * x1 + (b2/a0) * x2 - (a1/a0) * y1 - (a2/a0) * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;

            float out = body + 0.7f * y;

            if (driveAmount > 0.001f)
                out = juce::jmap(driveAmount, out, std::tanh(out * (1.0f + 3.0f * driveAmount)));

            L[idx] += out;
            if (R) R[idx] += out;

            bodyEnv *= bodyEnvMult;
            snappyEnv *= snappyEnvMult;
            if (bodyEnv < 1e-4f && snappyEnv < 1e-4f)
            {
                active = false;
                break;
            }
        }
    }

    void reset()
    {
        active = false; startDelaySamples = 0;
        bodyEnv = 0.0f; snappyEnv = 0.0f;
        bodyEnvMult = 0.995f; snappyEnvMult = 0.95f;
        baseFreq = 180.0f; toneAmount = 0.5f; decayTime = 0.4f; driveAmount = 0.0f;
        x1 = x2 = y1 = y2 = 0.0f;
    }

private:
    double sampleRate { 44100.0 };
    bool active { false };

    float baseFreq { 180.0f };
    float decayTime { 0.4f };
    float toneAmount { 0.5f };
    float driveAmount { 0.0f };

    float bodyEnv { 0.0f }, bodyEnvMult { 0.995f };
    float snappyEnv { 0.0f }, snappyEnvMult { 0.95f };

    float bodyPhase { 0.0f };
    unsigned int noiseState { 1u };

    // Biquad state
    float b0{0}, b1{0}, b2{0}, a0{1}, a1{0}, a2{0};
    float x1{0}, x2{0}, y1{0}, y2{0};

    int startDelaySamples { 0 };
};