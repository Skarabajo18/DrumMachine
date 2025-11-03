#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class StepGridComponent : public juce::Component, private juce::Timer
{
public:
    StepGridComponent(DrumMachineAudioProcessor& proc)
        : processor(proc)
    {
        setInterceptsMouseClicks(true, true);
        startTimerHz(30);
        activeSequencer = &processor.getBDSequencer();
    }

    void setStepsMode(bool is32)
    {
        if (activeSequencer) activeSequencer->setStepsMode(is32);
        repaint();
    }

    void setSequencer(StepSequencer* seq)
    {
        activeSequencer = seq;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        auto steps = activeSequencer ? activeSequencer->getNumSteps() : 16;
        const int padCount = steps;

        g.fillAll(juce::Colour::fromRGB(30, 38, 44));
        auto padArea = area.reduced(8);

        float padGap = 6.0f;
        float padW = (float)padArea.getWidth() / (float)padCount - padGap;
        float padH = (float)padArea.getHeight();

        for (int i = 0; i < padCount; ++i)
        {
            float x = (float)padArea.getX() + i * (padW + padGap);
            juce::Rectangle<float> r{x, (float)padArea.getY(), padW, padH};

            bool on = activeSequencer ? activeSequencer->getStepOn(i) : false;
            auto base = juce::Colour::fromRGB(50, 60, 70);
            auto onCol = juce::Colour::fromRGB(0, 180, 140);
            auto playCol = juce::Colour::fromRGB(255, 220, 90);

            g.setColour(base);
            g.fillRoundedRectangle(r, 6.0f);

            if (on)
            {
                g.setColour(onCol.withAlpha(0.85f));
                g.fillRoundedRectangle(r.reduced(2.0f), 6.0f);
            }

            if (i == currentStep)
            {
                g.setColour(playCol.withAlpha(0.9f));
                g.drawRoundedRectangle(r.reduced(1.0f), 6.0f, 2.0f);
            }

            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.setFont(juce::Font(12.0f));
            g.drawText(juce::String(i + 1), r.reduced(2.0f).toNearestInt(), juce::Justification::centredBottom, false);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override { toggleFromMouse(e); }
    void mouseDrag(const juce::MouseEvent& e) override { toggleFromMouse(e); }

private:
    void toggleFromMouse(const juce::MouseEvent& e)
    {
        if (!activeSequencer) return;
        auto area = getLocalBounds().reduced(8);
        auto steps = activeSequencer->getNumSteps();
        float padGap = 6.0f;
        float padW = (float)area.getWidth() / (float)steps - padGap;
        float localX = (float)e.position.getX() - (float)area.getX();
        int index = (int)(localX / (padW + padGap));
        if (index >= 0 && index < steps)
        {
            bool current = activeSequencer->getStepOn(index);
            activeSequencer->setStepOn(index, !current);
            repaint();
        }
    }

    void timerCallback() override
    {
        if (!activeSequencer) return;
        int idx = processor.getCurrentStepIndexForSequencer(activeSequencer);
        if (idx != currentStep)
        {
            currentStep = idx;
            repaint();
        }
    }

    DrumMachineAudioProcessor& processor;
    StepSequencer* activeSequencer { nullptr };
    int currentStep { -1 };
};