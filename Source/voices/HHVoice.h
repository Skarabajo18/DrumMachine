#pragma once
#include <JuceHeader.h>

class HHVoice
{
public:
    enum Type { Closed, Open };

    HHVoice(Type t) : type(t) {}

    void prepare(double sr) { sampleRate = sr; reset(); }

    void setParameters(float pitchSemi, float decaySec, float tone, float drive)
    {
        baseFreq = 8000.0f * std::pow(2.0f, pitchSemi / 12.0f);
        decayTime = juce::jlimit(0.01f, 2.0f, decaySec);
        toneAmount = juce::jlimit(0.0f, 1.0f, tone);
        driveAmount = juce::jlimit(0.0f, 1.0f, drive);
        ampEnvMult = std::exp(-1.0f / (decayTime * (float)sampleRate));
        // simple HP/BP filter
        float cutoff = juce::jmap(toneAmount, 3000.0f, 10000.0f);
        float x = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)sampleRate);
        hp_a = 1.0f - x; hp_b = x;
    }

    void noteOn(float velocity)
    {
        active = true;
        ampEnv = juce::jlimit(0.0f, 1.0f, velocity);
        noiseState = 0xabcdefu;
        remainingSamples = type == Closed ? (int)(0.03 * sampleRate) : (int)(0.25 * sampleRate);
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
            if (remainingSamples <= 0) { active = false; break; }

            float n = (float)((noiseState = noiseState * 1103515245u + 12345u) & 0x00ffffff) / (float)0x00ffffff;
            n = n * 2.0f - 1.0f;
            hp_y = hp_a * n + hp_b * hp_y;
            float out = hp_y * ampEnv;

            if (driveAmount > 0.001f)
                out = juce::jmap(driveAmount, out, std::tanh(out * (1.0f + 3.0f * driveAmount)));

            L[idx] += out;
            if (R) R[idx] += out;

            ampEnv *= ampEnvMult;
            --remainingSamples;
        }
    }

    void reset()
    {
        active = false; startDelaySamples = 0; remainingSamples = 0;
        ampEnv = 0.0f; ampEnvMult = 0.995f; hp_y = 0.0f; hp_a = 1.0f; hp_b = 0.0f;
        baseFreq = 8000.0f; toneAmount = 0.5f; decayTime = 0.1f; driveAmount = 0.0f;
    }

private:
    Type type { Closed };
    double sampleRate { 44100.0 };
    bool active { false };

    float baseFreq { 8000.0f };
    float decayTime { 0.1f };
    float toneAmount { 0.5f };
    float driveAmount { 0.0f };

    float ampEnv { 0.0f }, ampEnvMult { 0.995f };
    float hp_y { 0.0f }, hp_a { 1.0f }, hp_b { 0.0f };
    int startDelaySamples { 0 };
    int remainingSamples { 0 };
    unsigned int noiseState { 1u };
};