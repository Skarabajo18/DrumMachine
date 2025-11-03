#pragma once
#include <JuceHeader.h>

class SampleLayer
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        reset();
    }

    bool loadFromFile(const juce::File& file)
    {
        juce::AudioFormatManager afm;
        afm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
        if (!reader) return false;
        fileSampleRate = reader->sampleRate;
        const int channels = (int) reader->numChannels;
        const int samples = (int) reader->lengthInSamples;
        buffer.setSize(std::max(1, channels), samples);
        reader->read(&buffer, 0, samples, 0, true, true);
        loaded = true;
        reset();
        return true;
    }

    bool isLoaded() const { return loaded; }

    void setParameters(float tuneSemis, int startOffsetSamples, float gainLinear)
    {
        tune = tuneSemis;
        startOffset = juce::jmax(0, startOffsetSamples);
        gain = juce::jlimit(0.0f, 2.0f, gainLinear);
        // playback rate from semitones
        const double pitchRatio = std::pow(2.0, (double) tune / 12.0);
        playbackRate = (fileSampleRate / sampleRate) * pitchRatio;
    }

    void noteOnWithDelay(float velocity, int delaySamples)
    {
        if (!loaded) return;
        active = true;
        startDelaySamples = juce::jmax(0, delaySamples + startOffset);
        position = 0.0;
        env = juce::jlimit(0.0f, 1.0f, velocity);
    }

    void render(juce::AudioBuffer<float>& out, int startSample, int numSamples)
    {
        if (!active || !loaded) return;
        const int outChannels = out.getNumChannels();
        const int srcChannels = buffer.getNumChannels();
        const int srcSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            int idx = startSample + i;
            if (startDelaySamples > 0) { --startDelaySamples; continue; }
            int posInt = (int) position;
            if (posInt >= srcSamples)
            {
                active = false;
                break;
            }
            float frac = (float) (position - (double) posInt);
            for (int ch = 0; ch < outChannels; ++ch)
            {
                const float s0 = buffer.getSample(ch < srcChannels ? ch : 0, posInt);
                const float s1 = buffer.getSample(ch < srcChannels ? ch : 0, juce::jmin(posInt + 1, srcSamples - 1));
                float sample = s0 + (s1 - s0) * frac; // linear interp
                out.addSample(ch, idx, sample * env * gain);
            }
            position += playbackRate;
            env *= 0.9995f; // gentle decay to avoid click if long tail
            if (env < 1e-5f && position > srcSamples * 0.9) { active = false; break; }
        }
    }

    void reset()
    {
        active = false;
        startDelaySamples = 0;
        position = 0.0;
        env = 0.0f;
        gain = 1.0f;
        tune = 0.0f;
        playbackRate = fileSampleRate / sampleRate;
    }

private:
    juce::AudioBuffer<float> buffer;
    double sampleRate { 44100.0 };
    double fileSampleRate { 44100.0 };
    double playbackRate { 1.0 };
    double position { 0.0 };
    int startDelaySamples { 0 };
    int startOffset { 0 };
    float env { 0.0f };
    float gain { 1.0f };
    float tune { 0.0f };
    bool loaded { false };
    bool active { false };
};