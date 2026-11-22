#include "PluginEditor.h"

VocalEnhanceProAudioProcessorEditor::VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveform (p)
{
    setLookAndFeel (&modernLnF);
    setSize (720, 420);

    titleLabel.setText ("VOCAL ENHANCE PRO", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe9d5ff));
    addAndMakeVisible (titleLabel);

    setupSlider (hpfSlider, hpfLabel, "HIGH-PASS");
    setupSlider (deesserSlider, deesserLabel, "DE-ESSER");
    setupSlider (compSlider, compLabel, "COMPRESS");
    setupSlider (warmthSlider, warmthLabel, "WARMTH");
    setupSlider (airSlider, airLabel, "AIR");
    setupSlider (outputSlider, outputLabel, "OUTPUT");

    hpfAtt = std::make_unique<Attachment> (audioProcessor.getAPVTS(), "hpf", hpfSlider);
    deesserAtt = std::make_unique<Attachment> (audioProcessor.getAPVTS(), "deesser", deesserSlider);
    compAtt = std::make_unique<Attachment> (audioProcessor.getAPVTS(), "comp", compSlider);
    warmthAtt = std::make_unique<Attachment> (audioProcessor.getAPVTS(), "warmth", warmthSlider);
    airAtt = std::make_unique<Attachment> (audioProcessor.getAPVTS(), "air", airSlider);
    outputAtt = std::make_unique<Attachment> (audioProcessor.getAPVTS(), "output", outputSlider);

    bypassButton.setButtonText ("BYPASS");
    bypassButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffc4b5fd));
    bypassButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffa78bfa));
    addAndMakeVisible (bypassButton);
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.getAPVTS(), "bypass", bypassButton);

    addAndMakeVisible (waveform);
}

VocalEnhanceProAudioProcessorEditor::~VocalEnhanceProAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void VocalEnhanceProAudioProcessorEditor::setupSlider (juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffddd6fe));
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (12.5f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, juce::Colour (0xffc4b5fd));
    addAndMakeVisible (l);
}

void VocalEnhanceProAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff0a0614), 0, 0, juce::Colour (0xff1a1035), 0, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colour (0xff7c3aed).withAlpha (0.4f));
    g.fillRect (0, 0, getWidth(), 3);
}

void VocalEnhanceProAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (18);

    titleLabel.setBounds (bounds.removeFromTop (32));
    bounds.removeFromTop (8);

    waveform.setBounds (bounds.removeFromTop (110));
    bounds.removeFromTop (18);

    const int knobW = 100;
    const int knobH = 120;
    const int spacing = (bounds.getWidth() - 6 * knobW) / 5;

    auto placeKnob = [&] (juce::Slider& s, juce::Label& l)
    {
        auto area = bounds.removeFromLeft (knobW);
        l.setBounds (area.removeFromTop (18));
        s.setBounds (area.removeFromTop (knobH));
        bounds.removeFromLeft (spacing);
    };

    placeKnob (hpfSlider, hpfLabel);
    placeKnob (deesserSlider, deesserLabel);
    placeKnob (compSlider, compLabel);
    placeKnob (warmthSlider, warmthLabel);
    placeKnob (airSlider, airLabel);
    placeKnob (outputSlider, outputLabel);

    bypassButton.setBounds (getWidth() / 2 - 50, getHeight() - 42, 100, 28);
}
