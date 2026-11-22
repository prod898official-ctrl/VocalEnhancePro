#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalEnhanceProAudioProcessor::VocalEnhanceProAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Params", createParameterLayout())
{
}

VocalEnhanceProAudioProcessor::~VocalEnhanceProAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VocalEnhanceProAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"hpf", 1}, "High Pass", juce::NormalisableRange<float>{20.f, 300.f, 1.f, 0.4f}, 80.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"deess", 1}, "De-Esser", juce::NormalisableRange<float>{0.f, 1.f}, 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"comp", 1}, "Compress", juce::NormalisableRange<float>{0.f, 1.f}, 0.4f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"warmth", 1}, "Warmth", juce::NormalisableRange<float>{0.f, 1.f}, 0.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"air", 1}, "Air", juce::NormalisableRange<float>{0.f, 12.f, 0.1f}, 3.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"out", 1}, "Output", juce::NormalisableRange<float>{-24.f, 12.f, 0.1f}, 0.f));

    return { params.begin(), params.end() };
}

void VocalEnhanceProAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    highPass.prepare (spec);
    compressor.prepare (spec);
    makeup.prepare (spec);
    saturator.prepare (spec);
    airFilter.prepare (spec);

    saturator.functionToUse = [](float x){ return std::tanh(x * 1.4f); };

    *highPass.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 80.0);
    *airFilter.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 10000.0, 0.7f, 1.5f);
}

void VocalEnhanceProAudioProcessor::releaseResources() {}

bool VocalEnhanceProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void VocalEnhanceProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update filters
    float hpfFreq = apvts.getRawParameterValue("hpf")->load();
    *highPass.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, hpfFreq);

    float airGain = apvts.getRawParameterValue("air")->load();
    *airFilter.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 10500.0, 0.7f, juce::Decibels::decibelsToGain(airGain));

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // 1. High Pass
    highPass.process (ctx);

    // 2. Simple De-esser (gain reduction on high content)
    float deess = apvts.getRawParameterValue("deess")->load();
    if (deess > 0.01f)
    {
        float peak = buffer.getMagnitude(0, 0, buffer.getNumSamples());
        float reduction = 1.0f - (peak * deess * 0.5f);
        buffer.applyGain (juce::jlimit(0.5f, 1.0f, reduction));
    }

    // 3. Compressor
    float compAmt = apvts.getRawParameterValue("comp")->load();
    compressor.setThreshold (juce::jmap(compAmt, 0.f, 1.f, -6.f, -24.f));
    compressor.setRatio (juce::jmap(compAmt, 0.f, 1.f, 1.5f, 6.f));
    compressor.setAttack (8.0f);
    compressor.setRelease (120.0f);
    compressor.process (ctx);

    makeup.setGainDecibels (juce::jmap(compAmt, 0.f, 1.f, 0.f, 8.f));
    makeup.process (ctx);

    // 4. Warmth
    float warmth = apvts.getRawParameterValue("warmth")->load();
    if (warmth > 0.01f)
    {
        buffer.applyGain (1.0f + warmth * 1.6f);
        saturator.process (ctx);
        buffer.applyGain (1.0f / (1.0f + warmth * 0.85f));
    }

    // 5. Air
    airFilter.process (ctx);

    // 6. Output
    float outGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue("out")->load());
    buffer.applyGain (outGain);
}

void VocalEnhanceProAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VocalEnhanceProAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* VocalEnhanceProAudioProcessor::createEditor()
{
    return new VocalEnhanceProAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalEnhanceProAudioProcessor();
}
