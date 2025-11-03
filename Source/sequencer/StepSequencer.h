#pragma once
#include <JuceHeader.h>

class StepSequencer
{
public:
    void setStepsMode(bool is32)
    {
        steps = is32 ? 32 : 16;
        if ((int)on.size() != steps)
        {
            on.assign(steps, false);
            accent.assign(steps, false);
            setDefaultPattern();
        }
    }

    void setDefaultPattern()
    {
        // Start with all steps off and no accents
        std::fill(on.begin(), on.end(), false);
        std::fill(accent.begin(), accent.end(), false);
    }

    void setStepOn(int index, bool enabled)
    {
        if (index >= 0 && index < (int) on.size()) on[(size_t) index] = enabled;
    }
    void setAccent(int index, bool enabled)
    {
        if (index >= 0 && index < (int) accent.size()) accent[(size_t) index] = enabled;
    }
    bool getStepOn(int index) const
    {
        return (index >= 0 && index < (int) on.size()) ? on[(size_t) index] : false;
    }
    bool getAccent(int index) const
    {
        return (index >= 0 && index < (int) accent.size()) ? accent[(size_t) index] : false;
    }
    int getNumSteps() const { return steps; }

    struct Trigger { int sampleOffset; float velocity; };

    void computeTriggers(const juce::AudioPlayHead::CurrentPositionInfo& pos,
                         double sampleRate,
                         int numSamples,
                         bool is32Mode,
                         float swingAmount,
                         juce::Array<Trigger>& out)
    {
        out.clear();
        if (!pos.isPlaying || pos.bpm <= 0.0)
            return;

        setStepsMode(is32Mode);

        const double samplesPerBeat = sampleRate * 60.0 / pos.bpm;
        const double ppqPerStep = 0.25; // 1/16
        const double sequenceLengthPPQ = steps * ppqPerStep;

        const double startPPQ = pos.ppqPosition;
        const double endPPQ   = startPPQ + (double)numSamples / samplesPerBeat;

        const double barPPQ = getBarLengthPPQ(pos);
        double anchorPPQ = pos.ppqPositionOfLastBarStart;
        if (anchorPPQ <= 0.0)
            anchorPPQ = std::floor(startPPQ / barPPQ) * barPPQ;

        const double relStart = startPPQ - anchorPPQ;
        const double relEnd   = endPPQ   - anchorPPQ;
        int firstStep = (int) std::floor(relStart / ppqPerStep);
        int lastStep  = (int) std::floor(relEnd   / ppqPerStep);
        firstStep = juce::jlimit(0, steps - 1, firstStep);
        lastStep  = juce::jlimit(0, steps - 1, lastStep);

        for (int k = firstStep; k <= lastStep; ++k)
        {
            if (!on[k]) continue;

            double swingPPQ = 0.0;
            if ((k % 2) == 1)
                swingPPQ = juce::jlimit(0.0, 1.0, (double)swingAmount) * ppqPerStep * 0.5;

            const double stepPPQ = anchorPPQ + (double)k * ppqPerStep + swingPPQ;

            if (stepPPQ >= startPPQ && stepPPQ < endPPQ)
            {
                const double offsetSamplesD = (stepPPQ - startPPQ) * samplesPerBeat;
                int offsetSamples = (int)std::round(offsetSamplesD);
                float vel = accent[k] ? 1.0f : 0.8f;
                out.add({ juce::jlimit(0, numSamples - 1, offsetSamples), vel });
            }
        }

        if (relEnd >= sequenceLengthPPQ)
        {
            double nextAnchor = anchorPPQ + barPPQ;
            const double relStart2 = startPPQ - nextAnchor;
            const double relEnd2   = endPPQ   - nextAnchor;
            int firstStep2 = juce::jlimit(0, steps - 1, (int) std::floor(relStart2 / ppqPerStep));
            int lastStep2  = juce::jlimit(0, steps - 1, (int) std::floor(relEnd2   / ppqPerStep));
            for (int k = firstStep2; k <= lastStep2; ++k)
            {
                if (!on[k]) continue;
                double swingPPQ = ((k % 2) == 1) ? juce::jlimit(0.0, 1.0, (double)swingAmount) * ppqPerStep * 0.5 : 0.0;
                const double stepPPQ = nextAnchor + (double)k * ppqPerStep + swingPPQ;
                if (stepPPQ >= startPPQ && stepPPQ < endPPQ)
                {
                    const double offsetSamplesD = (stepPPQ - startPPQ) * samplesPerBeat;
                    int offsetSamples = (int)std::round(offsetSamplesD);
                    float vel = accent[k] ? 1.0f : 0.8f;
                    out.add({ juce::jlimit(0, numSamples - 1, offsetSamples), vel });
                }
            }
        }
    }

    int computeCurrentStepIndex(const juce::AudioPlayHead::CurrentPositionInfo& pos,
                                bool is32Mode) const
    {
        if (!pos.isPlaying || pos.bpm <= 0.0) return -1;
        int s = is32Mode ? 32 : 16;
        const double ppqPerStep = 0.25;
        const double barPPQ = getBarLengthPPQ(pos);
        double anchorPPQ = pos.ppqPositionOfLastBarStart;
        if (anchorPPQ <= 0.0) anchorPPQ = std::floor(pos.ppqPosition / barPPQ) * barPPQ;
        double rel = pos.ppqPosition - anchorPPQ;
        if (rel < 0.0) rel = 0.0;
        int idx = (int) std::floor(std::fmod(rel, s * ppqPerStep) / ppqPerStep);
        if (idx < 0) idx = 0;
        if (idx >= s) idx = s - 1;
        return idx;
    }

private:
    static double getBarLengthPPQ(const juce::AudioPlayHead::CurrentPositionInfo& pos)
    {
        int num = pos.timeSigNumerator > 0 ? pos.timeSigNumerator : 4;
        int den = pos.timeSigDenominator > 0 ? pos.timeSigDenominator : 4;
        return (double) num * (4.0 / (double) den);
    }

    int steps { 16 };
    std::vector<bool> on { std::vector<bool>(16, false) };
    std::vector<bool> accent { std::vector<bool>(16, false) };
};