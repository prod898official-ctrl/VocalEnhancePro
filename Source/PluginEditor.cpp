#include "PluginEditor.h"

VocalEnhanceProAudioProcessorEditor::VocalEnhanceProAudioProcessorEditor(VocalEnhanceProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), wave(p)
{
    setSize(720, 420);

    title.setText("VOCAL ENHANCE PRO", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, juce::Colour(0xffe9d5ff));
    addAndMakeVisible(title);

    setup(hpf, hpfL, "HIGH-PASS");
    setup(deess, deessL, "DE-ESSER");
    setup(comp, compL, "COMPRESS");
    setup(warm, warmL, "WARMTH");
    setup(air, airL, "AIR");
    setup(out, outL, "OUTPUT");

    hpfA   = std::make_unique<Att>(processor.getAPVTS(), "hpf", hpf);
    deessA = std::make_unique<Att>(processor.getAPVTS(), "deess", deess);
    compA  = std::make_unique<Att>(processor.getAPVTS(), "comp", comp);
    warmA  = std::make_unique<Att>(processor.getAPVTS(), "warm", warm);
    airA   = std::make_unique<Att>(processor.getAPVTS(), "air", air);
    outA   = std::make_unique<Att>(processor.getAPVTS(), "out", out);

    bypass.setButtonText("BYPASS");
    bypass.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffc4b5fd));
    addAndMakeVisible(bypass);
    bypassA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getAPVTS(), "bypass", bypass);

    addAndMakeVisible(wave);
}

VocalEnhanceProAudioProcessorEditor::~VocalEnhanceProAudioProcessorEditor() {}

void VocalEnhanceProAudioProcessorEditor::setup(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 16);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff7c3aed));
    s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffc084fc));
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(0xffc4b5fd));
    addAndMakeVisible(l);
}

void VocalEnhanceProAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0614));
    juce::ColourGradient gr(juce::Colour(0xff0a0614), 0, 0, juce::Colour(0xff1a1035), 0, (float)getHeight(), false);
    g.setGradientFill(gr);
    g.fillAll();
    g.setColour(juce::Colour(0xff7c3aed).withAlpha(0.4f));
    g.fillRect(0, 0, getWidth(), 3);
}

void VocalEnhanceProAudioProcessorEditor::resized()
{
    auto a = getLocalBounds().reduced(18);
    title.setBounds(a.removeFromTop(30));
    a.removeFromTop(8);
    wave.setBounds(a.removeFromTop(105));
    a.removeFromTop(16);

    const int kw = 100;
    const int kh = 120;
    int gap = (a.getWidth() - 6 * kw) / 5;

    auto place = [&](juce::Slider& s, juce::Label& l)
    {
        auto r = a.removeFromLeft(kw);
        l.setBounds(r.removeFromTop(18));
        s.setBounds(r.removeFromTop(kh));
        a.removeFromLeft(gap);
    };

    place(hpf, hpfL);
    place(deess, deessL);
    place(comp, compL);
    place(warm, warmL);
    place(air, airL);
    place(out, outL);

    bypass.setBounds(getWidth()/2 - 48, getHeight() - 40, 96, 26);
}
