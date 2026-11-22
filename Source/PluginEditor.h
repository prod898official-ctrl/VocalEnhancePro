#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay(VocalEnhanceProAudioProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.f);
        g.setColour(juce::Colour(0xff0f0a1e));
        g.fillRoundedRectangle(r, 6.f);
        g.setColour(juce::Colour(0xff4c1d95).withAlpha(0.55f));
        g.drawRoundedRectangle(r, 6.f, 1.3f);

        const auto& b = proc.waveBuffer;
        if (b.getNumSamples() < 4) return;

        juce::Path path;
        float mid = r.getCentreY();
        float h = r.getHeight() * 0.4f;
        float w = r.getWidth() - 6.f;
        float x0 = r.getX() + 3.f;

        path.startNewSubPath(x0, mid);
        for (int i = 0; i < b.getNumSamples(); i += 2)
        {
            float x = x0 + (float)i / b.getNumSamples() * w;
            path.lineTo(x, mid - b.getSample(0, i) * h);
        }
        g.setColour(juce::Colour(0xffc084fc).withAlpha(0.9f));
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }
    void timerCallback() override { repaint(); }
private:
    VocalEnhanceProAudioProcessor& proc;
};

class VocalEnhanceProAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    VocalEnhanceProAudioProcessorEditor(VocalEnhanceProAudioProcessor&);
    ~VocalEnhanceProAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VocalEnhanceProAudioProcessor& processor;
    WaveformDisplay wave;

    juce::Slider hpf, deess, comp, warm, air, out;
    juce::Label  hpfL, deessL, compL, warmL, airL, outL, title;

    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Att> hpfA, deessA, compA, warmA, airA, outA;

    juce::ToggleButton bypass;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassA;

    void setup(juce::Slider& s, juce::Label& l, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalEnhanceProAudioProcessorEditor)
};
