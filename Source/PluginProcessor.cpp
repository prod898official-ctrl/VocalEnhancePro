#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalEnhanceProAudioProcessor::VocalEnhanceProAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    waveformBuffer.setSize (1, 1024);
    waveformBuffer.clear();
}

VocalEnhanceProAudioProcessor::~VocalEnhanceProAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout VocalEnhanceProAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "hpf", 1 }, "High-Pass", juce::NormalisableRange<float> (20.0f, 300.0f, 1.0f, 0.4f), 80.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "deesser", 1 }, "De-Esser", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp", 1 }, "Compressor", juce::NormalisableRange<float> (0.0f, 1.0f), 0.45f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "warmth", 1 }, "Warmth", juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "air", 1 }, "Air", juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), 3.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

void VocalEnhanceProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    highPass.prepare (spec);
    compressor.prepare (spec);
    makeupGain.prepare (spec);
    saturator.prepare (spec);
    airShelf.prepare (spec);

    saturator.functionToUse = [] (float x) { return std::tanh (x * 1.3f); };

    updateFilters();
}

void VocalEnhanceProAudioProcessor::releaseResources() {}

bool VocalEnhanceProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void VocalEnhanceProAudioProcessor::updateFilters()
{
    auto hpfFreq = apvts.getRawParameterValue ("hpf")->load();
    *highPass.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, hpfFreq);

    auto airGain = apvts.getRawParameterValue ("air")->load();
    *airShelf.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        currentSampleRate, 10500.0f, 0.707f, juce::Decibels::decibelsToGain (airGain));
}

void VocalEnhanceProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f)
    {
        inputPeak.store (buffer.getMagnitude (0, 0, numSamples));
        outputPeak.store (inputPeak.load());
        return;
    }

    updateFilters();

    // Simple waveform (mono)
    {
        auto* data = buffer.getReadPointer (0);
        auto* wave = waveformBuffer.getWritePointer (0);
        const int waveSize = waveformBuffer.getNumSamples();
        for (int i = 0; i < numSamples; ++i)
            wave[i % waveSize] = data[i];
    }

    inputPeak.store (buffer.getMagnitude (0, 0, numSamples));

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    // High-Pass
    highPass.process (context);

    // Simple De-Esser
    {
        const float amount = apvts.getRawParameterValue ("deesser")->load();
        float peak = buffer.getMagnitude (0, 0, numSamples);

        deEsserEnv = (peak > 0.2f) ? deEsserEnv + (peak - deEsserEnv) * 0.03f
                                  : deEsserEnv * 0.92f;

        const float gain = juce::jlimit (0.4f, 1.0f, 1.0f - deEsserEnv * amount * 0.6f);
        buffer.applyGain (gain);
    }

    // Compressor + auto makeup
    {
        const float amount = apvts.getRawParameterValue ("comp")->load();
        compressor.setThreshold (juce::jmap (amount, 0.0f, 1.0f, -8.0f, -26.0f));
        compressor.setRatio (juce::jmap (amount, 0.0f, 1.0f, 1.8f, 5.5f));
        compressor.setAttack (10.0f);
        compressor.setRelease (100.0f);
        compressor.process (context);

        makeupGain.setGainDecibels (juce::jmap (amount, 0.0f, 1.0f, 0.0f, 7.0f));
        makeupGain.process (context);
    }

    // Warmth / Saturation
    {
        const float amount = apvts.getRawParameterValue ("warmth")->load();
        if (amount > 0.01f)
        {
            buffer.applyGain (1.0f + amount * 1.5f);
            saturator.process (context);
            buffer.applyGain (1.0f / (1.0f + amount * 0.8f));
        }
    }

    // Air
    airShelf.process (context);

    // Output
    const float outDb = apvts.getRawParameterValue ("output")->load();
    buffer.applyGain (juce::Decibels::decibelsToGain (outDb));

    outputPeak.store (buffer.getMagnitude (0, 0, numSamples));
}

void VocalEnhanceProAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalEnhanceProAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VocalEnhanceProAudioProcessor::createEditor()
{
    return new VocalEnhanceProAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalEnhanceProAudioProcessor();
}
