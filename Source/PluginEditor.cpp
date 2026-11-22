#include "PluginEditor.h"

VocalEnhanceProAudioProcessorEditor::VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (680, 320);

    titleLabel.setText ("VOCAL ENHANCE PRO", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe9d5ff));
    addAndMakeVisible (titleLabel);

    addKnob (hpfSlider,    hpfLabel,    "HIGH-PASS");
    addKnob (deessSlider,  deessLabel,  "DE-ESSER");
    addKnob (compSlider,   compLabel,   "COMPRESS");
    addKnob (warmthSlider, warmthLabel, "WARMTH");
    addKnob (airSlider,    airLabel,    "AIR");
    addKnob (outSlider,    outLabel,    "OUTPUT");

    hpfAtt    = std::make_unique<SliderAttachment> (processor.apvts, "hpf",    hpfSlider);
    deessAtt  = std::make_unique<SliderAttachment> (processor.apvts, "deess",  deessSlider);
    compAtt   = std::make_unique<SliderAttachment> (processor.apvts, "comp",   compSlider);
    warmthAtt = std::make_unique<SliderAttachment> (processor.apvts, "warmth", warmthSlider);
    airAtt    = std::make_unique<SliderAttachment> (processor.apvts, "air",    airSlider);
    outAtt    = std::make_unique<SliderAttachment> (processor.apvts, "out",    outSlider);
}

VocalEnhanceProAudioProcessorEditor::~VocalEnhanceProAudioProcessorEditor() {}

void VocalEnhanceProAudioProcessorEditor::addKnob (juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff7c3aed));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffc084fc));
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffc4b5fd));
    addAndMakeVisible (label);
}

void VocalEnhanceProAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0a14));

    juce::ColourGradient grad (juce::Colour (0xff0c0a14), 0, 0,
                               juce::Colour (0xff1a1030), 0, (float)getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();
}

void VocalEnhanceProAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    titleLabel.setBounds (area.removeFromTop (30));
    area.removeFromTop (15);

    const int knobWidth = 95;
    const int knobHeight = 130;
    int spacing = (area.getWidth() - 6 * knobWidth) / 5;

    auto place = [&](juce::Slider& s, juce::Label& l)
    {
        auto r = area.removeFromLeft (knobWidth);
        l.setBounds (r.removeFromTop (18));
        s.setBounds (r.removeFromTop (knobHeight));
        area.removeFromLeft (spacing);
    };

    place (hpfSlider, hpfLabel);
    place (deessSlider, deessLabel);
    place (compSlider, compLabel);
    place (warmthSlider, warmthLabel);
    place (airSlider, airLabel);
    place (outSlider, outLabel);
}
