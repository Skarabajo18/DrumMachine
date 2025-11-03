#pragma once
#include <JuceHeader.h>

namespace DMParams
{
    // BD
    static constexpr const char* bdPitchId   = "bdPitch";
    static constexpr const char* bdDecayId   = "bdDecay";
    static constexpr const char* bdToneId    = "bdTone";
    static constexpr const char* bdDriveId   = "bdDrive";

    // SD
    static constexpr const char* sdPitchId   = "sdPitch";
    static constexpr const char* sdDecayId   = "sdDecay";
    static constexpr const char* sdToneId    = "sdTone";
    static constexpr const char* sdDriveId   = "sdDrive";

    // CH
    static constexpr const char* chPitchId   = "chPitch";
    static constexpr const char* chDecayId   = "chDecay";
    static constexpr const char* chToneId    = "chTone";
    static constexpr const char* chDriveId   = "chDrive";

    // OH
    static constexpr const char* ohPitchId   = "ohPitch";
    static constexpr const char* ohDecayId   = "ohDecay";
    static constexpr const char* ohToneId    = "ohTone";
    static constexpr const char* ohDriveId   = "ohDrive";

    // Clap
    static constexpr const char* clapPitchId = "clapPitch";
    static constexpr const char* clapDecayId = "clapDecay";
    static constexpr const char* clapToneId  = "clapTone";
    static constexpr const char* clapDriveId = "clapDrive";

    // Sequencer globals
    static constexpr const char* swingId     = "swing";
    static constexpr const char* stepsModeId = "stepsMode";
    static constexpr const char* seqEnableId = "seqEnable";
    static constexpr const char* tempoId     = "tempo";

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        auto addFloat = [&params](const char* id, const char* name, float min, float max, float def, float skew = 1.0f)
        {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(id, name,
                juce::NormalisableRange<float>(min, max, 0.0f, skew), def));
        };

        // BD
        addFloat(bdPitchId, "BD Pitch", -12.0f, 12.0f, 0.0f, 1.0f);
        addFloat(bdDecayId, "BD Decay", 0.05f, 2.0f, 0.5f, 0.4f);
        addFloat(bdToneId,  "BD Tone",  0.0f, 1.0f, 0.5f, 1.0f);
        addFloat(bdDriveId, "BD Drive", 0.0f, 1.0f, 0.0f, 1.0f);

        // SD
        addFloat(sdPitchId, "SD Pitch", -12.0f, 12.0f, 0.0f, 1.0f);
        addFloat(sdDecayId, "SD Decay", 0.05f, 2.5f, 0.4f, 0.4f);
        addFloat(sdToneId,  "SD Tone",  0.0f, 1.0f, 0.5f, 1.0f);
        addFloat(sdDriveId, "SD Drive", 0.0f, 1.0f, 0.0f, 1.0f);

        // CH
        addFloat(chPitchId, "CH Pitch", -12.0f, 12.0f, 0.0f, 1.0f);
        addFloat(chDecayId, "CH Decay", 0.01f, 0.3f, 0.08f, 0.6f);
        addFloat(chToneId,  "CH Tone",  0.0f, 1.0f, 0.5f, 1.0f);
        addFloat(chDriveId, "CH Drive", 0.0f, 1.0f, 0.0f, 1.0f);

        // OH
        addFloat(ohPitchId, "OH Pitch", -12.0f, 12.0f, 0.0f, 1.0f);
        addFloat(ohDecayId, "OH Decay", 0.1f, 2.0f, 0.4f, 0.6f);
        addFloat(ohToneId,  "OH Tone",  0.0f, 1.0f, 0.5f, 1.0f);
        addFloat(ohDriveId, "OH Drive", 0.0f, 1.0f, 0.0f, 1.0f);

        // Clap
        addFloat(clapPitchId, "Clap Pitch", -12.0f, 12.0f, 0.0f, 1.0f);
        addFloat(clapDecayId, "Clap Decay", 0.05f, 1.5f, 0.3f, 0.5f);
        addFloat(clapToneId,  "Clap Tone",  0.0f, 1.0f, 0.5f, 1.0f);
        addFloat(clapDriveId, "Clap Drive", 0.0f, 1.0f, 0.0f, 1.0f);

        // Sequencer globals
        addFloat(swingId, "Swing", 0.0f, 0.6f, 0.0f, 1.0f);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            stepsModeId, "Steps Mode", juce::StringArray{"16", "32"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            seqEnableId, "Seq Enable", false));
        addFloat(tempoId, "Tempo", 60.0f, 200.0f, 125.0f, 1.0f);

        return { params.begin(), params.end() };
    }
}