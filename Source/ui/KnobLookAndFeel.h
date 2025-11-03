#pragma once
#include <JuceHeader.h>

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(240, 170, 60));
        setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(70, 80, 90));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(255, 160, 60));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(40, 45, 50));
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(32, 38, 44));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(55, 60, 66));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 6.0f;
        const float centreX = bounds.getCentreX();
        const float centreY = bounds.getCentreY();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Base
        g.setColour(juce::Colour::fromRGB(26, 32, 38));
        g.fillEllipse(centreX - radius - 3.0f, centreY - radius - 3.0f, (radius + 3.0f) * 2.0f, (radius + 3.0f) * 2.0f);

        // Background ring
        const float ringThickness = juce::jmax(3.0f, radius * 0.18f);
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(findColour(juce::Slider::trackColourId).withAlpha(0.45f));
        g.strokePath(bgArc, juce::PathStrokeType(ringThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(valueArc, juce::PathStrokeType(ringThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Ticks (marcas) alrededor del anillo
        const int tickCount = 8; // cada 12.5%
        g.setColour(findColour(juce::Slider::trackColourId).withAlpha(0.7f));
        for (int i = 0; i <= tickCount; ++i)
        {
            const float t = (float) i / (float) tickCount;
            const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const float rOuter = radius + 0.0f;
            const float rInner = radius - juce::jmax(2.0f, ringThickness * 0.55f);
            juce::Point<float> o(centreX + std::cos(a) * rOuter, centreY + std::sin(a) * rOuter);
            juce::Point<float> in(centreX + std::cos(a) * rInner, centreY + std::sin(a) * rInner);
            g.drawLine(o.x, o.y, in.x, in.y, 1.5f);
        }

        // Indicador (cuña) alineado al ángulo
        const float wedgeWidth = juce::MathConstants<float>::pi / 48.0f; // ~3.75º
        const float rOuter = radius;
        const float rInner = radius - ringThickness * 0.85f;
        juce::Path wedge;
        juce::Point<float> pA(centreX + std::cos(angle - wedgeWidth) * rInner,
                              centreY + std::sin(angle - wedgeWidth) * rInner);
        juce::Point<float> pB(centreX + std::cos(angle + wedgeWidth) * rInner,
                              centreY + std::sin(angle + wedgeWidth) * rInner);
        juce::Point<float> pC(centreX + std::cos(angle) * rOuter,
                              centreY + std::sin(angle) * rOuter);
        wedge.startNewSubPath(pA);
        wedge.lineTo(pC);
        wedge.lineTo(pB);
        wedge.closeSubPath();
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillPath(wedge);

        // Centre cap
        g.setColour(juce::Colour::fromRGB(48, 54, 60));
        g.fillEllipse(centreX - (radius * 0.55f), centreY - (radius * 0.55f), radius * 1.1f, radius * 1.1f);
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawEllipse(centreX - (radius * 0.55f), centreY - (radius * 0.55f), radius * 1.1f, radius * 1.1f, 1.0f);

        // Value text (optional overlay)
        if (slider.getTextBoxPosition() == juce::Slider::NoTextBox)
        {
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.setFont(juce::FontOptions(12.0f));
            juce::String text = juce::String(slider.getValue(), 2);
            g.drawText(text, bounds.reduced(6.0f).toNearestInt(), juce::Justification::centred);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
        g.setColour(juce::Colour::fromRGB(42, 50, 58));
        g.fillRoundedRectangle(bounds, 6.0f);

        const auto trackColour = findColour(juce::Slider::rotarySliderFillColourId);
        const auto bgColour = findColour(juce::Slider::trackColourId).withAlpha(0.4f);
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.reduced(2.0f), 6.0f);

        auto valueRect = bounds;
        valueRect.setRight(sliderPos);
        g.setColour(trackColour);
        g.fillRoundedRectangle(valueRect.reduced(2.0f), 6.0f);

        // Thumb
        const float thumbRadius = (float) getSliderThumbRadius(slider);
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillEllipse(sliderPos - thumbRadius, bounds.getCentreY() - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);
    }

    int getSliderThumbRadius(juce::Slider& slider) override
    {
        const int base = juce::jmax(8, juce::roundToInt(juce::jmin(slider.getWidth(), slider.getHeight()) * 0.08f));
        return base;
    }
};