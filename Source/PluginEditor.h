#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ModernLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId, juce::Colour (0xffc084fc));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff7c3aed));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff1e1b4b));
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (0xffe0e7ff));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff1e1b4b));
        g.strokePath (backgroundArc, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
            juce::ColourGradient grad (juce::Colour (0xffa78bfa), centre.x - radius, centre.y, juce::Colour (0xff7c3aed), centre.x + radius, centre.y, false);
            g.setGradientFill (grad);
            g.strokePath (valueArc, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        juce::Path thumb;
        thumb.addRectangle (-1.75f, -radius + 2.0f, 3.5f, radius * 0.35f);
        g.setColour (juce::Colour (0xffe9d5ff));
        g.fillPath (thumb, juce::AffineTransform::rotation (angle).translated (centre));
    }
};

class WaveformComponent : public juce::Component, private juce::Timer
{
public:
    WaveformComponent (VocalEnhanceProAudioProcessor& p) : processor (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff0f0a1e));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (juce::Colour (0xff4c1d95).withAlpha (0.6f));
        g.drawRoundedRectangle (bounds, 8.0f, 1.5f);

        const auto& buffer = processor.waveformBuffer;
        if (buffer.getNumSamples() < 2) return;

        juce::Path path;
        const float midY = bounds.getCentreY();
        const float height = bounds.getHeight() * 0.4f;
        const float width = bounds.getWidth() - 8.0f;
        const float startX = bounds.getX() + 4.0f;

        path.startNewSubPath (startX, midY);
        for (int i = 0; i < buffer.getNumSamples(); i += 4)
        {
            float x = startX + (float(i) / buffer.getNumSamples()) * width;
            float y = midY - buffer.getSample (0, i) * height;
            path.lineTo (x, y);
        }

        g.setColour (juce::Colour (0xffc084fc).withAlpha (0.85f));
        g.strokePath (path, juce::PathStrokeType (1.6f));
    }

    void timerCallback() override { repaint(); }

private:
    VocalEnhanceProAudioProcessor& processor;
};

class VocalEnhanceProAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor&);
    ~VocalEnhanceProAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalEnhanceProAudioProcessor& audioProcessor;
    ModernLookAndFeel modernLnF;

    juce::Slider hpfSlider, deesserSlider, compSlider, warmthSlider, airSlider, outputSlider;
    juce::Label hpfLabel, deesserLabel, compLabel, warmthLabel, airLabel, outputLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> hpfAtt, deesserAtt, compAtt, warmthAtt, airAtt, outputAtt;

    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt;

    WaveformComponent waveform;
    juce::Label titleLabel;

    void setupSlider (juce::Slider& s, juce::Label& l, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalEnhanceProAudioProcessorEditor)
};
