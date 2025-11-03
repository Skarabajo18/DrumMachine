/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

DrumMachineAudioProcessorEditor::DrumMachineAudioProcessorEditor (DrumMachineAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), multiGrid(p)
{
    setSize (940, 560);

    auto& apvts = audioProcessor.getAPVTS();

    addAndMakeVisible(seqEnableButton);
    seqEnableAttach = std::make_unique<ButtonAttachment>(apvts, DMParams::seqEnableId, seqEnableButton);

    stepsCombo.addItem("16", 1);
    stepsCombo.addItem("32", 2);
    stepsCombo.setSelectedId(1);
    addAndMakeVisible(stepsCombo);
    stepsModeAttach = std::make_unique<ComboBoxAttachment>(apvts, DMParams::stepsModeId, stepsCombo);
    stepsCombo.onChange = [this]() {
        bool is32 = stepsCombo.getSelectedId() == 2;
        audioProcessor.setGlobalStepsMode(is32);
        // multi grid reads from sequencers directly
        repaint();
    };

    swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    swingSlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(swingSlider);
    swingAttach = std::make_unique<SliderAttachment>(apvts, DMParams::swingId, swingSlider);

    tempoSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    tempoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tempoSlider.setName("Tempo");
    tempoSlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(tempoSlider);
    tempoAttach = std::make_unique<SliderAttachment>(apvts, DMParams::tempoId, tempoSlider);

    addAndMakeVisible(startButton);
    addAndMakeVisible(pauseButton);
    addAndMakeVisible(restartButton);
    startButton.onClick   = [this]() { audioProcessor.startInternalTransport(); };
    pauseButton.onClick   = [this]() { audioProcessor.pauseInternalTransport(); };
    restartButton.onClick = [this]() { audioProcessor.restartInternalTransport(); };

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&knobLNF);
    };
    setupKnob(bdPitchSlider); bdPitchSlider.setName("Pitch"); addAndMakeVisible(bdPitchSlider);
    setupKnob(bdDecaySlider); bdDecaySlider.setName("Decay"); addAndMakeVisible(bdDecaySlider);
    setupKnob(bdToneSlider);  bdToneSlider.setName("Tone");  addAndMakeVisible(bdToneSlider);
    setupKnob(bdDriveSlider); bdDriveSlider.setName("Drive"); addAndMakeVisible(bdDriveSlider);

    bdPitchAttach = std::make_unique<SliderAttachment>(apvts, DMParams::bdPitchId, bdPitchSlider);
    bdDecayAttach = std::make_unique<SliderAttachment>(apvts, DMParams::bdDecayId, bdDecaySlider);
    bdToneAttach  = std::make_unique<SliderAttachment>(apvts, DMParams::bdToneId, bdToneSlider);
    bdDriveAttach = std::make_unique<SliderAttachment>(apvts, DMParams::bdDriveId, bdDriveSlider);

    // SD (Snare)
    setupKnob(sdPitchSlider); sdPitchSlider.setName("Pitch"); addAndMakeVisible(sdPitchSlider);
    setupKnob(sdDecaySlider); sdDecaySlider.setName("Decay"); addAndMakeVisible(sdDecaySlider);
    setupKnob(sdToneSlider);  sdToneSlider.setName("Tone");  addAndMakeVisible(sdToneSlider);
    setupKnob(sdDriveSlider); sdDriveSlider.setName("Drive"); addAndMakeVisible(sdDriveSlider);
    sdPitchAttach = std::make_unique<SliderAttachment>(apvts, DMParams::sdPitchId, sdPitchSlider);
    sdDecayAttach = std::make_unique<SliderAttachment>(apvts, DMParams::sdDecayId, sdDecaySlider);
    sdToneAttach  = std::make_unique<SliderAttachment>(apvts, DMParams::sdToneId, sdToneSlider);
    sdDriveAttach = std::make_unique<SliderAttachment>(apvts, DMParams::sdDriveId, sdDriveSlider);

    // CH (Closed Hat)
    setupKnob(chPitchSlider); chPitchSlider.setName("Pitch"); addAndMakeVisible(chPitchSlider);
    setupKnob(chDecaySlider); chDecaySlider.setName("Decay"); addAndMakeVisible(chDecaySlider);
    setupKnob(chToneSlider);  chToneSlider.setName("Tone");  addAndMakeVisible(chToneSlider);
    setupKnob(chDriveSlider); chDriveSlider.setName("Drive"); addAndMakeVisible(chDriveSlider);
    // Per-track colors
    bdPitchSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(255,160,60));
    bdDecaySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(255,160,60));
    bdToneSlider .setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(255,160,60));
    bdDriveSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(255,160,60));

    sdPitchSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(120,180,240));
    sdDecaySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(120,180,240));
    sdToneSlider .setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(120,180,240));
    sdDriveSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(120,180,240));

    chPitchSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(140,220,140));
    chDecaySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(140,220,140));
    chToneSlider .setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(140,220,140));
    chDriveSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(140,220,140));
    chPitchAttach = std::make_unique<SliderAttachment>(apvts, DMParams::chPitchId, chPitchSlider);
    chDecayAttach = std::make_unique<SliderAttachment>(apvts, DMParams::chDecayId, chDecaySlider);
    chToneAttach  = std::make_unique<SliderAttachment>(apvts, DMParams::chToneId, chToneSlider);
    chDriveAttach = std::make_unique<SliderAttachment>(apvts, DMParams::chDriveId, chDriveSlider);

    // OH (Open Hat)
    setupKnob(ohPitchSlider); ohPitchSlider.setName("Pitch"); addAndMakeVisible(ohPitchSlider);
    setupKnob(ohDecaySlider); ohDecaySlider.setName("Decay"); addAndMakeVisible(ohDecaySlider);
    setupKnob(ohToneSlider);  ohToneSlider.setName("Tone");  addAndMakeVisible(ohToneSlider);
    setupKnob(ohDriveSlider); ohDriveSlider.setName("Drive"); addAndMakeVisible(ohDriveSlider);
    ohPitchSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(90,200,200));
    ohDecaySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(90,200,200));
    ohToneSlider .setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(90,200,200));
    ohDriveSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(90,200,200));
    ohPitchAttach = std::make_unique<SliderAttachment>(apvts, DMParams::ohPitchId, ohPitchSlider);
    ohDecayAttach = std::make_unique<SliderAttachment>(apvts, DMParams::ohDecayId, ohDecaySlider);
    ohToneAttach  = std::make_unique<SliderAttachment>(apvts, DMParams::ohToneId, ohToneSlider);
    ohDriveAttach = std::make_unique<SliderAttachment>(apvts, DMParams::ohDriveId, ohDriveSlider);

    // Clap
    setupKnob(clapPitchSlider); clapPitchSlider.setName("Pitch"); addAndMakeVisible(clapPitchSlider);
    setupKnob(clapDecaySlider); clapDecaySlider.setName("Decay"); addAndMakeVisible(clapDecaySlider);
    setupKnob(clapToneSlider);  clapToneSlider.setName("Tone");  addAndMakeVisible(clapToneSlider);
    setupKnob(clapDriveSlider); clapDriveSlider.setName("Drive"); addAndMakeVisible(clapDriveSlider);
    clapPitchSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(220,140,220));
    clapDecaySlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(220,140,220));
    clapToneSlider .setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(220,140,220));
    clapDriveSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(220,140,220));
    clapPitchAttach = std::make_unique<SliderAttachment>(apvts, DMParams::clapPitchId, clapPitchSlider);
    clapDecayAttach = std::make_unique<SliderAttachment>(apvts, DMParams::clapDecayId, clapDecaySlider);
    clapToneAttach  = std::make_unique<SliderAttachment>(apvts, DMParams::clapToneId, clapToneSlider);
    clapDriveAttach = std::make_unique<SliderAttachment>(apvts, DMParams::clapDriveId, clapDriveSlider);

    addAndMakeVisible(multiGrid);
}

DrumMachineAudioProcessorEditor::~DrumMachineAudioProcessorEditor()
{
}

void DrumMachineAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB(28, 34, 40));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (16.0f));
    g.drawFittedText ("DrumMachine", getLocalBounds().removeFromTop(24), juce::Justification::centred, 1);
}

void DrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto top = area.removeFromTop(64);

    seqEnableButton.setBounds(top.removeFromLeft(110));
    stepsCombo.setBounds(top.removeFromLeft(80));
    swingSlider.setBounds(top.removeFromLeft(260));

    auto tb = top.removeFromLeft(240);
    startButton.setBounds(tb.removeFromLeft(70).reduced(5));
    pauseButton.setBounds(tb.removeFromLeft(80).reduced(5));
    restartButton.setBounds(tb.removeFromLeft(80).reduced(5));

    tempoSlider.setBounds(top.removeFromRight(120));

    auto gridArea = area.removeFromTop(area.getHeight() - 240);
    multiGrid.setBounds(gridArea.reduced(6));

    // Bottom knob area: 5 groups (BD, SD, CH, OH, Clap). Each group 2x2 grid of knobs.
    auto knobs = area;
    const int groups = 5;
    const int colGap = 10;
    int groupW = (knobs.getWidth() - colGap * (groups - 1)) / groups;
    
    auto layoutGroup = [](juce::Rectangle<int> r, juce::Slider& s1, juce::Slider& s2, juce::Slider& s3, juce::Slider& s4)
    {
        r = r.reduced(8);
        const int cellW = r.getWidth() / 2;
        const int cellH = r.getHeight() / 2;
        juce::Rectangle<int> leftTop   (r.getX(),          r.getY(),          cellW, cellH);
        juce::Rectangle<int> rightTop  (r.getX() + cellW,  r.getY(),          cellW, cellH);
        juce::Rectangle<int> leftBottom(r.getX(),          r.getY() + cellH,  cellW, cellH);
        juce::Rectangle<int> rightBottom(r.getX() + cellW, r.getY() + cellH,  cellW, cellH);
        s1.setBounds(leftTop.reduced(6));
        s2.setBounds(rightTop.reduced(6));
        s3.setBounds(leftBottom.reduced(6));
        s4.setBounds(rightBottom.reduced(6));
    };

    // BD
    auto gBD = knobs.removeFromLeft(groupW);
    layoutGroup(gBD, bdPitchSlider, bdDecaySlider, bdToneSlider, bdDriveSlider);
    knobs.removeFromLeft(colGap);
    // SD
    auto gSD = knobs.removeFromLeft(groupW);
    layoutGroup(gSD, sdPitchSlider, sdDecaySlider, sdToneSlider, sdDriveSlider);
    knobs.removeFromLeft(colGap);
    // CH
    auto gCH = knobs.removeFromLeft(groupW);
    layoutGroup(gCH, chPitchSlider, chDecaySlider, chToneSlider, chDriveSlider);
    knobs.removeFromLeft(colGap);
    // OH
    auto gOH = knobs.removeFromLeft(groupW);
    layoutGroup(gOH, ohPitchSlider, ohDecaySlider, ohToneSlider, ohDriveSlider);
    knobs.removeFromLeft(colGap);
    // Clap
    auto gClap = knobs.removeFromLeft(knobs.getWidth());
    layoutGroup(gClap, clapPitchSlider, clapDecaySlider, clapToneSlider, clapDriveSlider);
}
