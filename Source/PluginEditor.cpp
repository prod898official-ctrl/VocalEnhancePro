#include "PluginEditor.h"

VocalEnhanceProAudioProcessorEditor::VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), wave (p)
{
    setLookAndFeel (&lnf);
    setSize (700, 400);

    title.setText ("VOCAL ENHANCE PRO", juce::dontSendNotification);
    title.setFont (juce::FontOptions (21.0f, juce::Font::bold));
    title.setJustificationType (juce::Justification::centred);
    title.setColour (juce::Label::textColourId, juce::Colour (0xffe9d5ff));
    addAndMakeVisible (title);

    setupKnob (hpf,     hpfL,     "HIGH-PASS");
    setupKnob (deesser, deesserL, "DE-ESSER");
    setupKnob (comp,    compL,    "COMPRESS");
    setupKnob (warmth,  warmthL,  "WARMTH");
    setupKnob (air,     airL,     "AIR");
    setupKnob (output,  outputL,  "OUTPUT");

    hpfA     = std::make_unique<Att> (processor.getAPVTS(), "hpf",     hpf);
    deesserA = std::make_unique<Att> (processor.getAPVTS(), "deesser", deesser);
    compA    = std::make_unique<Att> (processor.getAPVTS(), "comp",    comp);
    warmthA  = std::make_unique<Att> (processor.getAPVTS(), "warmth",  warmth);
    airA     = std::make_unique<Att> (processor.getAPVTS(), "air",     air);
    outputA  = std::make_unique<Att> (processor.getAPVTS(), "output",  output);

    bypass.setButtonText ("BYPASS");
    bypass.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffc4b5fd));
    addAndMakeVisible (bypass);
    bypassA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.getAPVTS(), "bypass", bypass);

    addAndMakeVisible (wave);
}

VocalEnhanceProAudioProcessorEditor::~VocalEnhanceProAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void VocalEnhanceProAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 16);
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, juce::Colour (0xffc4b5fd));
    addAndMakeVisible (l);
}

void VocalEnhanceProAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0a0614));

    juce::ColourGradient grad (juce::Colour (0xff0a0614), 0, 0,
                               juce::Colour (0xff1a1035), 0, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();

    g.setColour (juce::Colour (0xff7c3aed).withAlpha (0.35f));
    g.fillRect (0, 0, getWidth(), 3);
}

void VocalEnhanceProAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);

    title.setBounds (area.removeFromTop (28));
    area.removeFromTop (6);

    wave.setBounds (area.removeFromTop (100));
    area.removeFromTop (14);

    const int kw = 95;
    const int kh = 110;
    const int gap = (area.getWidth() - 6 * kw) / 5;

    auto place = [&] (juce::Slider& s, juce::Label& l)
    {
        auto a = area.removeFromLeft (kw);
        l.setBounds (a.removeFromTop (16));
        s.setBounds (a.removeFromTop (kh));
        area.removeFromLeft (gap);
    };

    place (hpf, hpfL);
    place (deesser, deesserL);
    place (comp, compL);
    place (warmth, warmthL);
    place (air, airL);
    place (output, outputL);

    bypass.setBounds (getWidth() / 2 - 45, getHeight() - 38, 90, 26);
}
