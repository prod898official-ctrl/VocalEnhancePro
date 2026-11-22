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
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path bg;
        bg.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff1e1b4b));
        g.strokePath (bg, juce::PathStrokeType (4.0f));

        if (slider.isEnabled())
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour (juce::Colour (0xffa78bfa));
            g.strokePath (value, juce::PathStrokeType (4.0f));
        }

        juce::Path thumb;
        thumb.addRectangle (-2.0f, -radius + 1.0f, 4.0f, radius * 0.32f);
        g.setColour (juce::Colour (0xffe9d5ff));
        g.fillPath (thumb, juce::AffineTransform::rotation (angle).translated (centre));
    }
};

class WaveformComponent : public juce::Component, private juce::Timer
{
public:
    explicit WaveformComponent (VocalEnhanceProAudioProcessor& p) : processor (p)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);

        g.setColour (juce::Colour (0xff0f0a1e));
        g.fillRoundedRectangle (r, 6.0f);

        g.setColour (juce::Colour (0xff4c1d95).withAlpha (0.5f));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);

        const auto& buf = processor.waveformBuffer;
        if (buf.getNumSamples() < 4)
            return;

        juce::Path p;
        const float mid = r.getCentreY();
        const float h = r.getHeight() * 0.38f;
        const float w = r.getWidth() - 6.0f;
        const float x0 = r.getX() + 3.0f;

        p.startNewSubPath (x0, mid);

        for (int i = 0; i < buf.getNumSamples(); i += 2)
        {
            float x = x0 + (float) i / (float) buf.getNumSamples() * w;
            float y = mid - buf.getSample (0, i) * h;
            p.lineTo (x, y);
        }

        g.setColour (juce::Colour (0xffc084fc).withAlpha (0.9f));
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }

    void timerCallback() override { repaint(); }

private:
    VocalEnhanceProAudioProcessor& processor;
};

class VocalEnhanceProAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor&);
    ~VocalEnhanceProAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalEnhanceProAudioProcessor& processor;
    ModernLookAndFeel lnf;

    juce::Slider hpf, deesser, comp, warmth, air, output;
    juce::Label  hpfL, deesserL, compL, warmthL, airL, outputL;

    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Att> hpfA, deesserA, compA, warmthA, airA, outputA;

    juce::ToggleButton bypass;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassA;

    WaveformComponent wave;
    juce::Label title;

    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalEnhanceProAudioProcessorEditor)
};
