/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

int DrumMachineAudioProcessor::getCurrentStepIndexForSequencer(const StepSequencer* s) const
{
    if (s == &seqBD) return curBD;
    if (s == &seqSD) return curSD;
    if (s == &seqCH) return curCH;
    if (s == &seqOH) return curOH;
    if (s == &seqClap) return curClap;
    return -1;
}

void DrumMachineAudioProcessor::setGlobalStepsMode(bool is32)
{
    seqBD.setStepsMode(is32);
    seqSD.setStepsMode(is32);
    seqCH.setStepsMode(is32);
    seqOH.setStepsMode(is32);
    seqClap.setStepsMode(is32);
}

bool DrumMachineAudioProcessor::loadSampleForLane(int laneIndex, const juce::File& file)
{
    switch (laneIndex)
    {
        case 0: return bdSampleLayer.loadFromFile(file);
        case 1: return sdSampleLayer.loadFromFile(file);
        case 2: return chSample.loadFromFile(file);
        case 3: return ohSample.loadFromFile(file);
        case 4: return clapSample.loadFromFile(file);
        default: return false;
    }
}

DrumMachineAudioProcessor::DrumMachineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

DrumMachineAudioProcessor::~DrumMachineAudioProcessor()
{
}

const juce::String DrumMachineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DrumMachineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DrumMachineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DrumMachineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DrumMachineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DrumMachineAudioProcessor::getNumPrograms()
{
    return 1;
}

int DrumMachineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DrumMachineAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String DrumMachineAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void DrumMachineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void DrumMachineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    bdVoice.prepare(sampleRate);
    sdVoice.prepare(sampleRate);
    chVoice.prepare(sampleRate);
    ohVoice.prepare(sampleRate);
    clapVoice.prepare(sampleRate);

    bdSampleLayer.prepare(sampleRate);
    sdSampleLayer.prepare(sampleRate);
    chSample.prepare(sampleRate);
    ohSample.prepare(sampleRate);
    clapSample.prepare(sampleRate);

    internalPPQ = 0.0;
    internalPlaying = true;
    curBD = curSD = curCH = curOH = curClap = -1;
}

void DrumMachineAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DrumMachineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
  #endif
}
#endif

void DrumMachineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Read parameters for all voices
    bdVoice.setParameters(*apvts.getRawParameterValue(DMParams::bdPitchId),
                          *apvts.getRawParameterValue(DMParams::bdDecayId),
                          *apvts.getRawParameterValue(DMParams::bdToneId),
                          *apvts.getRawParameterValue(DMParams::bdDriveId));

    sdVoice.setParameters(*apvts.getRawParameterValue(DMParams::sdPitchId),
                          *apvts.getRawParameterValue(DMParams::sdDecayId),
                          *apvts.getRawParameterValue(DMParams::sdToneId),
                          *apvts.getRawParameterValue(DMParams::sdDriveId));

    chVoice.setParameters(*apvts.getRawParameterValue(DMParams::chPitchId),
                          *apvts.getRawParameterValue(DMParams::chDecayId),
                          *apvts.getRawParameterValue(DMParams::chToneId),
                          *apvts.getRawParameterValue(DMParams::chDriveId));

    ohVoice.setParameters(*apvts.getRawParameterValue(DMParams::ohPitchId),
                          *apvts.getRawParameterValue(DMParams::ohDecayId),
                          *apvts.getRawParameterValue(DMParams::ohToneId),
                          *apvts.getRawParameterValue(DMParams::ohDriveId));

    clapVoice.setParameters(*apvts.getRawParameterValue(DMParams::clapPitchId),
                            *apvts.getRawParameterValue(DMParams::clapDecayId),
                            *apvts.getRawParameterValue(DMParams::clapToneId),
                            *apvts.getRawParameterValue(DMParams::clapDriveId));

    bool seqEnable = apvts.getRawParameterValue(DMParams::seqEnableId)->load() > 0.5f;
    int stepsChoice = (int)apvts.getRawParameterValue(DMParams::stepsModeId)->load();
    float swingAmount = apvts.getRawParameterValue(DMParams::swingId)->load();
    double tempo = (double) apvts.getRawParameterValue(DMParams::tempoId)->load();

    juce::Array<StepSequencer::Trigger> trBD, trSD, trCH, trOH, trClap;

    if (seqEnable)
    {
        bool usedHost = false;
        juce::AudioPlayHead::CurrentPositionInfo pos;
        if (auto* hostPlayHead = getPlayHead())
        {
            if (auto positionInfo = hostPlayHead->getPosition())
            {
                pos.isPlaying = positionInfo->getIsPlaying();
                pos.bpm = positionInfo->getBpm().orFallback(120.0);
                if (pos.isPlaying && pos.bpm > 0.0)
                    usedHost = true;
            }
        }

        if (!usedHost)
        {
            pos.isPlaying = internalPlaying;
            pos.bpm = tempo;
            pos.timeSigNumerator = 4; pos.timeSigDenominator = 4;
            const double barPPQ = (double) pos.timeSigNumerator * (4.0 / (double) pos.timeSigDenominator);
            pos.ppqPositionOfLastBarStart = std::floor(internalPPQ / barPPQ) * barPPQ;
            pos.ppqPosition = internalPPQ;
        }

        seqBD.computeTriggers(pos, getSampleRate(), buffer.getNumSamples(), stepsChoice == 1, swingAmount, trBD);
        seqSD.computeTriggers(pos, getSampleRate(), buffer.getNumSamples(), stepsChoice == 1, swingAmount, trSD);
        seqCH.computeTriggers(pos, getSampleRate(), buffer.getNumSamples(), stepsChoice == 1, swingAmount, trCH);
        seqOH.computeTriggers(pos, getSampleRate(), buffer.getNumSamples(), stepsChoice == 1, swingAmount, trOH);
        seqClap.computeTriggers(pos, getSampleRate(), buffer.getNumSamples(), stepsChoice == 1, swingAmount, trClap);

        curBD   = seqBD.computeCurrentStepIndex(pos, stepsChoice == 1);
        curSD   = seqSD.computeCurrentStepIndex(pos, stepsChoice == 1);
        curCH   = seqCH.computeCurrentStepIndex(pos, stepsChoice == 1);
        curOH   = seqOH.computeCurrentStepIndex(pos, stepsChoice == 1);
        curClap = seqClap.computeCurrentStepIndex(pos, stepsChoice == 1);

        // advance internal PPQ if used
        if (!usedHost && internalPlaying)
        {
            const double samplesPerBeat = getSampleRate() * 60.0 / tempo;
            internalPPQ += (double) buffer.getNumSamples() / samplesPerBeat;
        }

        // Trigger voices and samples
        if (trBD.size() > 0) {
            auto t = trBD.getReference(0);
            bdVoice.noteOnWithDelay(t.velocity, t.sampleOffset);
            if (bdSampleLayer.isLoaded()) {
                bdSampleLayer.setParameters(*apvts.getRawParameterValue(DMParams::bdPitchId), 0, 0.35f); // layer mix as gain
                bdSampleLayer.noteOnWithDelay(t.velocity, t.sampleOffset);
            }
        }
        if (trSD.size() > 0) {
            auto t = trSD.getReference(0);
            sdVoice.noteOnWithDelay(t.velocity, t.sampleOffset);
            if (sdSampleLayer.isLoaded()) {
                sdSampleLayer.setParameters(*apvts.getRawParameterValue(DMParams::sdPitchId), 0, 0.35f);
                sdSampleLayer.noteOnWithDelay(t.velocity, t.sampleOffset);
            }
        }
        if (trCH.size() > 0) {
            auto t = trCH.getReference(0);
            if (chSample.isLoaded()) {
                chSample.setParameters(*apvts.getRawParameterValue(DMParams::chPitchId), 0, 1.0f);
                chSample.noteOnWithDelay(t.velocity, t.sampleOffset);
            } else {
                chVoice.noteOnWithDelay(t.velocity, t.sampleOffset);
            }
        }
        if (trOH.size() > 0) {
            auto t = trOH.getReference(0);
            if (ohSample.isLoaded()) {
                ohSample.setParameters(*apvts.getRawParameterValue(DMParams::ohPitchId), 0, 1.0f);
                ohSample.noteOnWithDelay(t.velocity, t.sampleOffset);
            } else {
                ohVoice.noteOnWithDelay(t.velocity, t.sampleOffset);
            }
        }
        if (trClap.size() > 0) {
            auto t = trClap.getReference(0);
            if (clapSample.isLoaded()) {
                clapSample.setParameters(*apvts.getRawParameterValue(DMParams::clapPitchId), 0, 1.0f);
                clapSample.noteOnWithDelay(t.velocity, t.sampleOffset);
            } else {
                clapVoice.noteOnWithDelay(t.velocity, t.sampleOffset);
            }
        }
    }
    else
    {
        // MIDI mapping: C1 BD, D1 SD, F#1 CH, A#1 OH, D#1 CLAP
        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                float vel = msg.getVelocity() / 127.0f;
                switch (msg.getNoteNumber())
                {
                    case 36: bdVoice.noteOn(vel); if (bdSampleLayer.isLoaded()) { bdSampleLayer.setParameters(*apvts.getRawParameterValue(DMParams::bdPitchId), 0, 0.35f); bdSampleLayer.noteOnWithDelay(vel, 0); } break;
                    case 38: sdVoice.noteOn(vel); if (sdSampleLayer.isLoaded()) { sdSampleLayer.setParameters(*apvts.getRawParameterValue(DMParams::sdPitchId), 0, 0.35f); sdSampleLayer.noteOnWithDelay(vel, 0); } break;
                    case 42: if (chSample.isLoaded()) { chSample.setParameters(*apvts.getRawParameterValue(DMParams::chPitchId), 0, 1.0f); chSample.noteOnWithDelay(vel, 0); } else { chVoice.noteOn(vel); } break;
                    case 46: if (ohSample.isLoaded()) { ohSample.setParameters(*apvts.getRawParameterValue(DMParams::ohPitchId), 0, 1.0f); ohSample.noteOnWithDelay(vel, 0); } else { ohVoice.noteOn(vel); } break;
                    case 39: if (clapSample.isLoaded()) { clapSample.setParameters(*apvts.getRawParameterValue(DMParams::clapPitchId), 0, 1.0f); clapSample.noteOnWithDelay(vel, 0); } else { clapVoice.noteOn(vel); } break;
                    default: break;
                }
            }
        }
    }

    // Render voices and samples
    bdVoice.render(buffer, 0, buffer.getNumSamples());
    sdVoice.render(buffer, 0, buffer.getNumSamples());
    chVoice.render(buffer, 0, buffer.getNumSamples());
    ohVoice.render(buffer, 0, buffer.getNumSamples());
    clapVoice.render(buffer, 0, buffer.getNumSamples());

    bdSampleLayer.render(buffer, 0, buffer.getNumSamples());
    sdSampleLayer.render(buffer, 0, buffer.getNumSamples());
    chSample.render(buffer, 0, buffer.getNumSamples());
    ohSample.render(buffer, 0, buffer.getNumSamples());
    clapSample.render(buffer, 0, buffer.getNumSamples());
}

bool DrumMachineAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DrumMachineAudioProcessor::createEditor()
{
    return new DrumMachineAudioProcessorEditor (*this);
}

void DrumMachineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void DrumMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, (size_t) sizeInBytes);
    if (tree.isValid())
        apvts.replaceState(tree);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumMachineAudioProcessor();
}
