/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/MultiStepGridComponent.h"
#include "ui/KnobLookAndFeel.h"

class DrumMachineAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DrumMachineAudioProcessorEditor (DrumMachineAudioProcessor&);
    ~DrumMachineAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DrumMachineAudioProcessor& audioProcessor;

    juce::ToggleButton seqEnableButton { "Sequencer" };
    juce::ComboBox stepsCombo;
    juce::Slider swingSlider;
    juce::Slider tempoSlider;
    juce::TextButton startButton { "Start" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton restartButton { "Restart" };

    // BD (Kick)
    juce::Slider bdPitchSlider, bdDecaySlider, bdToneSlider, bdDriveSlider;
    // SD (Snare)
    juce::Slider sdPitchSlider, sdDecaySlider, sdToneSlider, sdDriveSlider;
    // CH (Closed Hat)
    juce::Slider chPitchSlider, chDecaySlider, chToneSlider, chDriveSlider;
    // OH (Open Hat)
    juce::Slider ohPitchSlider, ohDecaySlider, ohToneSlider, ohDriveSlider;
    // Clap
    juce::Slider clapPitchSlider, clapDecaySlider, clapToneSlider, clapDriveSlider;

    MultiStepGridComponent multiGrid;
    KnobLookAndFeel knobLNF;

    using SliderAttachment  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment= juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment  = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment>   seqEnableAttach;
    std::unique_ptr<ComboBoxAttachment> stepsModeAttach;
    std::unique_ptr<SliderAttachment>   swingAttach;
    std::unique_ptr<SliderAttachment>   tempoAttach;
    std::unique_ptr<SliderAttachment>   bdPitchAttach, bdDecayAttach, bdToneAttach, bdDriveAttach;
    std::unique_ptr<SliderAttachment>   sdPitchAttach, sdDecayAttach, sdToneAttach, sdDriveAttach;
    std::unique_ptr<SliderAttachment>   chPitchAttach, chDecayAttach, chToneAttach, chDriveAttach;
    std::unique_ptr<SliderAttachment>   ohPitchAttach, ohDecayAttach, ohToneAttach, ohDriveAttach;
    std::unique_ptr<SliderAttachment>   clapPitchAttach, clapDecayAttach, clapToneAttach, clapDriveAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineAudioProcessorEditor)
};
