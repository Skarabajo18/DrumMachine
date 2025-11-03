#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../params/ParameterLayout.h"

class MultiStepGridComponent : public juce::Component, private juce::Timer
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    MultiStepGridComponent(DrumMachineAudioProcessor& proc)
        : processor(proc)
    {
        setInterceptsMouseClicks(true, true);
        startTimerHz(30);
        lanes.add({ &proc.getBDSequencer(),  "BD"   });
        lanes.add({ &proc.getSDSequencer(),  "SD"   });
        lanes.add({ &proc.getCHSequencer(),  "CH"   });
        lanes.add({ &proc.getOHSequencer(),  "OH"   });
        lanes.add({ &proc.getClapSequencer(),"Clap" });

        int rows = lanes.size();

        // Create per-lane mini controls (Pitch/Decay)
        for (int i = 0; i < rows; ++i)
        {
            auto* sp = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            sp->setTextBoxIsEditable(false);
            sp->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            sp->setRange(-12.0, 12.0, 0.0);
            sp->setName("Pitch");
            pitchSliders.add(sp);
            addAndMakeVisible(sp);

            auto* sd = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
            sd->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            sd->setRange(0.05, 2.0, 0.0);
            sd->setName("Decay");
            decaySliders.add(sd);
            addAndMakeVisible(sd);
        }

        // Attach sliders to APVTS params per lane
        auto& apvts = processor.getAPVTS();
        for (int i = 0; i < rows; ++i)
        {
            auto ids = getPitchDecayParamIdsForRow(i);
            pitchAttach.add(new SliderAttachment(apvts, ids.first, *pitchSliders[i]));
            decayAttach.add(new SliderAttachment(apvts, ids.second, *decaySliders[i]));
        }

        for (int i = 0; i < rows; ++i)
        {
            auto* ld = new juce::DrawableButton("Load", juce::DrawableButton::ImageOnButtonBackground);
            auto icon = makeLoadIcon();
            ld->setImages(icon.get(), icon.get(), icon.get(), nullptr, nullptr, nullptr, nullptr, nullptr);
            loadButtons.add(ld);
            addAndMakeVisible(ld);
            ld->onClick = [this, i]() { chooseFileForLane(i); };
        }

        // Create per-lane buttons
        for (int i = 0; i < rows; ++i)
        {
            auto* cb = new juce::DrawableButton("Clear", juce::DrawableButton::ImageOnButtonBackground);
            auto* cp = new juce::DrawableButton("Copy", juce::DrawableButton::ImageOnButtonBackground);
            auto* ps = new juce::DrawableButton("Paste", juce::DrawableButton::ImageOnButtonBackground);
            auto iconClear = makeClearIcon();
            auto iconCopy  = makeCopyIcon();
            auto iconPaste = makePasteIcon();
            cb->setImages(iconClear.get(), iconClear.get(), iconClear.get(), nullptr, nullptr, nullptr, nullptr, nullptr);
            cp->setImages(iconCopy.get(),  iconCopy.get(),  iconCopy.get(),  nullptr, nullptr, nullptr, nullptr, nullptr);
            ps->setImages(iconPaste.get(), iconPaste.get(), iconPaste.get(), nullptr, nullptr, nullptr, nullptr, nullptr);
             clearButtons.add(cb); copyButtons.add(cp); pasteButtons.add(ps);
             addAndMakeVisible(cb); addAndMakeVisible(cp); addAndMakeVisible(ps);

             cb->onClick = [this, i]() { clearLane(i); };
             cp->onClick = [this, i]() { copyLane(i); };
             ps->onClick = [this, i]() { pasteLane(i); };
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        int rows = lanes.size();
        if (rows == 0) return;
        float rowGap = 6.0f;
        float rowH = ((float)area.getHeight() - rowGap * (rows - 1)) / (float)rows;

        int btnW = 48; int btnH = 20; int btnGap = 4;
        int ctrlH = 18; int ctrlGap = 6;
        for (int r = 0; r < rows; ++r)
        {
            int y = area.getY() + (int)(r * (rowH + rowGap));
            int x = area.getX();
            loadButtons[r]->setBounds(x, y + 2, btnW, btnH);
            clearButtons[r]->setBounds(x + btnW + btnGap, y + 2, btnW, btnH);
            copyButtons[r]->setBounds(x + 2*btnW + 2*btnGap, y + 2, btnW, btnH);
            pasteButtons[r]->setBounds(x + 3*btnW + 3*btnGap, y + 2, btnW, btnH);

            // Place mini controls inside the label area (left of the grid)
            int ctrlX = x + 3*btnW + 3*btnGap + 8;
            int ctrlW = (int)labelW - (3*btnW + 3*btnGap) - 16;
            pitchSliders[r]->setBounds(ctrlX, y + 2, ctrlW, ctrlH);
            decaySliders[r]->setBounds(ctrlX, y + 2 + ctrlH + ctrlGap, ctrlW, ctrlH);
        }
        controlW = 0.0f;
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.fillAll(juce::Colour::fromRGB(30, 38, 44));
        auto padArea = area.reduced(8);

        int rows = lanes.size();
        if (rows == 0) return;
        int steps = lanes[0].seq->getNumSteps();
        float rowGap = 6.0f;
        float padGap = 4.0f;
        float rowH = ((float)padArea.getHeight() - rowGap * (rows - 1)) / (float)rows;
        float padW = ((float)padArea.getWidth() - labelW - controlW) / (float)steps - padGap;

        bool seqEnabled = processor.getAPVTS().getRawParameterValue(DMParams::seqEnableId)->load() > 0.5f;
        auto baseA = juce::Colour::fromRGB(50, 60, 70);
        auto baseB = juce::Colour::fromRGB(45, 55, 65);
        auto onCol = juce::Colour::fromRGB(0, 180, 140);
        auto accentCol = juce::Colour::fromRGB(255, 120, 0);
        auto playCol = juce::Colour::fromRGB(255, 220, 90);

        if (!seqEnabled)
        {
            baseA = baseA.withAlpha(0.7f);
            baseB = baseB.withAlpha(0.7f);
            onCol = onCol.withAlpha(0.75f);
            accentCol = accentCol.withAlpha(0.8f);
            playCol = playCol.withAlpha(0.8f);
        }

        // Beat numbers per row (1–4 or 1–8)
        int beats = steps / 4;
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(11.0f));

        for (int r = 0; r < rows; ++r)
        {
            float y = (float)padArea.getY() + r * (rowH + rowGap);
            // label area includes buttons and label; shift drawing start after labelW
            for (int b = 0; b < beats; ++b)
            {
                float bx = (float)padArea.getX() + labelW + b * 4 * (padW + padGap);
                float bw = 4 * (padW + padGap);
                g.drawText(juce::String(b + 1), (int)bx, (int)(y - 14), (int)bw, 12, juce::Justification::centred);
            }
        }

        for (int r = 0; r < rows; ++r)
        {
            float y = (float)padArea.getY() + r * (rowH + rowGap);
            // label text
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.setFont(juce::FontOptions(12.0f));
            g.drawFittedText(lanes[r].label, juce::Rectangle<int>(padArea.getX(), (int)y, (int)labelW, (int)rowH), juce::Justification::centredLeft, 1);

            for (int i = 0; i < steps; ++i)
            {
                float x = (float)padArea.getX() + labelW + i * (padW + padGap);
                juce::Rectangle<float> rct{ x, y, padW, rowH };

                // Alternating background every 4 steps (4/4 beat groups)
                int group = (i / 4) % 2;
                g.setColour(group == 0 ? baseA : baseB);
                g.fillRoundedRectangle(rct, 6.0f);

                // Beat grid vertical line every 4 steps
                if (i % 4 == 0)
                {
                    g.setColour(juce::Colours::black.withAlpha(0.35f));
                    g.fillRect((int) (x - padGap * 0.5f), (int) y, 1, (int) rowH);
                }

                // Pad on/off
                bool on = lanes[r].seq->getStepOn(i);
                if (on)
                {
                    g.setColour(onCol);
                    g.fillRoundedRectangle(rct.reduced(2.0f), 6.0f);
                }

                // Accent overlay (small bar at top)
                bool acc = lanes[r].seq->getAccent(i);
                if (acc)
                {
                    g.setColour(accentCol);
                    auto accRect = rct.reduced(2.0f);
                    accRect.setHeight(4.0f);
                    g.fillRoundedRectangle(accRect, 2.0f);
                }

                // Current step highlight per lane
                int cur = currentSteps.size() > r ? currentSteps[r] : -1;
                if (i == cur)
                {
                    g.setColour(playCol);
                    g.drawRoundedRectangle(rct.reduced(1.0f), 6.0f, 2.0f);
                }

                // Strong separator at bar boundary (step 0 and 16)
                if (i == 0 || (steps == 32 && i == 16))
                {
                    g.setColour(juce::Colours::black.withAlpha(0.4f));
                    g.fillRect((int) (x - padGap * 0.5f), (int) y, 2, (int) rowH);
                }
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override { toggleFromMouse(e); }
    void mouseDrag(const juce::MouseEvent& e) override { toggleFromMouse(e); }

private:
    struct Lane { StepSequencer* seq; juce::String label; };

    // Map pitch/decay parameter IDs per row
    std::pair<juce::String, juce::String> getPitchDecayParamIdsForRow(int row)
    {
        switch (row)
        {
            case 0: return { DMParams::bdPitchId, DMParams::bdDecayId }; // BD
            case 1: return { DMParams::sdPitchId, DMParams::sdDecayId }; // SD
            case 2: return { DMParams::chPitchId, DMParams::chDecayId }; // CH
            case 3: return { DMParams::ohPitchId, DMParams::ohDecayId }; // OH
            case 4: return { DMParams::clapPitchId, DMParams::clapDecayId }; // Clap
            default: return { DMParams::bdPitchId, DMParams::bdDecayId };
        }
    }

    std::unique_ptr<juce::Drawable> makeLoadIcon()
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        juce::Path p; // simple folder icon
        p.addRoundedRectangle(2, 10, 20, 12, 3);
        p.addTriangle(6, 10, 12, 4, 18, 10);
        dp->setPath(p);
        dp->setFill(juce::Colour::fromRGB(240, 170, 60));
        return dp;
    }
    std::unique_ptr<juce::Drawable> makeClearIcon()
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        juce::Path p; // trash bin
        p.addRectangle(6, 6, 12, 2);
        p.addRoundedRectangle(6, 8, 12, 12, 2);
        dp->setPath(p);
        dp->setFill(juce::Colour::fromRGB(220, 90, 80));
        return dp;
    }
    std::unique_ptr<juce::Drawable> makeCopyIcon()
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        juce::Path p; // two sheets
        p.addRoundedRectangle(4, 6, 14, 12, 2);
        p.addRoundedRectangle(8, 10, 14, 12, 2);
        dp->setPath(p);
        dp->setFill(juce::Colour::fromRGB(120, 180, 240));
        return dp;
    }
    std::unique_ptr<juce::Drawable> makePasteIcon()
    {
        auto dp = std::make_unique<juce::DrawablePath>();
        juce::Path p; // clipboard
        p.addRoundedRectangle(6, 6, 16, 16, 3);
        p.addRectangle(10, 4, 8, 4);
        dp->setPath(p);
        dp->setFill(juce::Colour::fromRGB(150, 210, 120));
        return dp;
    }

    void chooseFileForLane(int lane)
    {
        int rows = lanes.size();
        if (lane < 0 || lane >= rows) return;
        juce::File initialDir = juce::File::getCurrentWorkingDirectory().getChildFile("LoFi Drums");
        if (! initialDir.exists()) initialDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        
        auto chooser = std::make_shared<juce::FileChooser>("Choose a sample", initialDir, "*.wav;*.aiff;*.aif");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, lane, chooser](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (! file.existsAsFile())
                    return;

                const bool ok = processor.loadSampleForLane(lane, file);
                if (! ok)
                {
                    juce::NativeMessageBox::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                                "Sample load failed",
                                                                "No se pudo cargar el archivo: " + file.getFileName());
                }
                else
                {
                    // Simple feedback so the user knows which lane has a sample loaded
                    lanes[lane].label = lanes[lane].label + " \u2022"; // add dot marker
                    repaint();
                }
            });
    }

    void toggleFromMouse(const juce::MouseEvent& e)
    {
        auto padArea = getLocalBounds().reduced(8);
        int rows = lanes.size();
        if (rows == 0) return;
        int steps = lanes[0].seq->getNumSteps();
        float rowGap = 6.0f;
        float padGap = 4.0f;
        float rowH = ((float)padArea.getHeight() - rowGap * (rows - 1)) / (float)rows;
        float padW = ((float)padArea.getWidth() - labelW) / (float)steps - padGap;

        float localY = (float)e.position.getY() - (float)padArea.getY();
        int row = (int) ((localY) / (rowH + rowGap));
        if (row < 0 || row >= rows) return;

        float localX = (float)e.position.getX() - (float)padArea.getX() - labelW;
        int col = (int) (localX / (padW + padGap));
        if (col < 0 || col >= steps) return;

        if (e.mods.isRightButtonDown() || e.mods.isAltDown())
        {
            bool acc = lanes[row].seq->getAccent(col);
            lanes[row].seq->setAccent(col, !acc);
        }
        else
        {
            bool current = lanes[row].seq->getStepOn(col);
            lanes[row].seq->setStepOn(col, !current);
        }
        repaint();
    }

    void clearLane(int lane)
    {
        if (lane < 0 || lane >= lanes.size()) return;
        int steps = lanes[lane].seq->getNumSteps();
        for (int i = 0; i < steps; ++i)
        {
            lanes[lane].seq->setStepOn(i, false);
            lanes[lane].seq->setAccent(i, false);
        }
        repaint();
    }

    void copyLane(int lane)
    {
        if (lane < 0 || lane >= lanes.size()) return;
        int steps = lanes[lane].seq->getNumSteps();
        clipboardOn.resize(steps);
        clipboardAccent.resize(steps);
        for (int i = 0; i < steps; ++i)
        {
            clipboardOn[i] = lanes[lane].seq->getStepOn(i);
            clipboardAccent[i] = lanes[lane].seq->getAccent(i);
        }
    }

    void pasteLane(int lane)
    {
        if (lane < 0 || lane >= lanes.size()) return;
        int steps = lanes[lane].seq->getNumSteps();
        int n = (int) std::min<size_t>(steps, clipboardOn.size());
        for (int i = 0; i < n; ++i)
        {
            lanes[lane].seq->setStepOn(i, clipboardOn[i]);
            lanes[lane].seq->setAccent(i, clipboardAccent.size() > (size_t) i ? clipboardAccent[i] : false);
        }
        repaint();
    }

    void timerCallback() override
    {
        currentSteps.clear();
        for (int i = 0; i < lanes.size(); ++i)
            currentSteps.add(processor.getCurrentStepIndexForSequencer(lanes[i].seq));
        repaint();
    }

    DrumMachineAudioProcessor& processor;
    juce::Array<Lane> lanes;
    juce::OwnedArray<juce::DrawableButton> loadButtons;
    juce::OwnedArray<juce::DrawableButton> clearButtons, copyButtons, pasteButtons;

    // Per-row mini controls
    juce::OwnedArray<juce::Slider> pitchSliders;
    juce::OwnedArray<juce::Slider> decaySliders;
    juce::OwnedArray<SliderAttachment> pitchAttach;
    juce::OwnedArray<SliderAttachment> decayAttach;

    juce::Array<int> currentSteps;
    float labelW { 220.0f }; // widened to fit buttons
    float controlW { 0.0f }; // reserved space at right for mini controls

    std::vector<bool> clipboardOn;
    std::vector<bool> clipboardAccent;
};