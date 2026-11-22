#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VocalEnhanceProAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    VocalEnhanceProAudioProcessorEditor (VocalEnhanceProAudioProcessor&);
    ~VocalEnhanceProAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalEnhanceProAudioProcessor& processor;

    juce::Slider hpfSlider, deessSlider, compSlider, warmthSlider, airSlider, outSlider;
    juce::Label  hpfLabel, deessLabel, compLabel, warmthLabel, airLabel, outLabel, titleLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> hpfAtt, deessAtt, compAtt, warmthAtt, airAtt, outAtt;

    void addKnob (juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalEnhanceProAudioProcessorEditor)
};
