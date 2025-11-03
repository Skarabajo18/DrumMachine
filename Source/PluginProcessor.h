/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "params/ParameterLayout.h"
#include "voices/BDVoice.h"
#include "voices/SDVoice.h"
#include "voices/HHVoice.h"
#include "voices/ClapVoice.h"
#include "sequencer/StepSequencer.h"
#include "sampling/SampleLayer.h"

class DrumMachineAudioProcessor  : public juce::AudioProcessor
{
public:
    DrumMachineAudioProcessor();
    ~DrumMachineAudioProcessor() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Sequencers per instrument
    StepSequencer& getBDSequencer()   { return seqBD; }
    StepSequencer& getSDSequencer()   { return seqSD; }
    StepSequencer& getCHSequencer()   { return seqCH; }
    StepSequencer& getOHSequencer()   { return seqOH; }
    StepSequencer& getClapSequencer() { return seqClap; }

    int getCurrentStepIndexForSequencer(const StepSequencer* s) const;
    void setGlobalStepsMode(bool is32);

    // Internal transport controls
    void startInternalTransport() { internalPlaying = true; }
    void pauseInternalTransport() { internalPlaying = false; }
    void restartInternalTransport() { internalPPQ = 0.0; internalPlaying = true; }

    // Sample loading per lane: 0 BD layer, 1 SD layer, 2 CH, 3 OH, 4 Clap
    bool loadSampleForLane(int laneIndex, const juce::File& file);

private:
    // Voices
    BDVoice bdVoice;
    SDVoice sdVoice;
    HHVoice chVoice { HHVoice::Closed };
    HHVoice ohVoice { HHVoice::Open };
    ClapVoice clapVoice;

    // Sample layers
    SampleLayer bdSampleLayer;
    SampleLayer sdSampleLayer;
    SampleLayer chSample;
    SampleLayer ohSample;
    SampleLayer clapSample;

    // Sequencers per lane
    StepSequencer seqBD, seqSD, seqCH, seqOH, seqClap;

    // Internal clock
    double internalPPQ { 0.0 };
    bool internalPlaying { true };

    // Current step indices per lane
    int curBD { -1 }, curSD { -1 }, curCH { -1 }, curOH { -1 }, curClap { -1 };

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", DMParams::createParameterLayout() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineAudioProcessor)
};
