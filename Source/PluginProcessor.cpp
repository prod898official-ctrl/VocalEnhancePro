#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalEnhanceProAudioProcessor::VocalEnhanceProAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    waveformBuffer.setSize (2, 2048);
    waveformBuffer.clear();
}

VocalEnhanceProAudioProcessor::~VocalEnhanceProAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VocalEnhanceProAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("hpf", "High-Pass", juce::NormalisableRange<float> (20.0f, 300.0f, 1.0f, 0.4f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("deesser", "De-Esser", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("comp", "Compressor", juce::NormalisableRange<float> (0.0f, 1.0f), 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("warmth", "Warmth", juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("air", "Air Boost", juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), 3.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("bypass", "Bypass", false));

    return { params.begin(), params.end() };
}

void VocalEnhanceProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    highPass.prepare (spec);
    compressor.prepare (spec);
    makeupGain.prepare (spec);
    saturator.prepare (spec);
    airShelf.prepare (spec);
    deEsserFilter.prepare (spec);

    saturator.functionToUse = [] (float x) { return std::tanh (x * 1.4f) * 0.85f; };

    updateDSP();
}

void VocalEnhanceProAudioProcessor::releaseResources() {}

bool VocalEnhanceProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void VocalEnhanceProAudioProcessor::updateDSP()
{
    const float hpfFreq = apvts.getRawParameterValue ("hpf")->load();
    *highPass.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, hpfFreq);

    const float airGain = apvts.getRawParameterValue ("air")->load();
    *airShelf.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 10000.0f, 0.7f, juce::Decibels::decibelsToGain (airGain));

    *deEsserFilter.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 6500.0f, 0.7f, 1.0f);
}

void VocalEnhanceProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    const bool bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    if (bypass)
    {
        float peakIn = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peakIn = juce::jmax (peakIn, buffer.getMagnitude (ch, 0, numSamples));
        inputPeak.store (peakIn);
        outputPeak.store (peakIn);
        return;
    }

    updateDSP();

    float peakIn = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        peakIn = juce::jmax (peakIn, buffer.getMagnitude (ch, 0, numSamples));
        auto* src = buffer.getReadPointer (ch);
        auto* dst = waveformBuffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
            dst[(i + currentBlockSize) % waveformBuffer.getNumSamples()] = src[i] * 0.5f;
    }
    inputPeak.store (peakIn);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    highPass.process (context);

    {
        const float amount = apvts.getRawParameterValue ("deesser")->load();
        float maxHF = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                maxHF = juce::jmax (maxHF, std::abs (data[i]));
        }

        if (maxHF > 0.25f)
            deEsserEnv = deEsserEnv + (maxHF - deEsserEnv) * 0.02f;
        else
            deEsserEnv = deEsserEnv * 0.85f;

        deEsserGain = juce::jlimit (0.3f, 1.0f, 1.0f - (deEsserEnv * amount * 0.7f));
        buffer.applyGain (deEsserGain);
    }

    {
        const float amount = apvts.getRawParameterValue ("comp")->load();
        compressor.setThreshold (juce::jmap (amount, 0.0f, 1.0f, -6.0f, -28.0f));
        compressor.setRatio (juce::jmap (amount, 0.0f, 1.0f, 1.5f, 6.0f));
        compressor.setAttack (8.0f);
        compressor.setRelease (120.0f);
        compressor.process (context);

        makeupGain.setGainDecibels (juce::jmap (amount, 0.0f, 1.0f, 0.0f, 8.0f));
        makeupGain.process (context);
    }

    {
        const float amount = apvts.getRawParameterValue ("warmth")->load();
        if (amount > 0.01f)
        {
            buffer.applyGain (1.0f + amount * 1.8f);
            saturator.process (context);
            buffer.applyGain (1.0f / (1.0f + amount * 0.9f));
        }
    }

    airShelf.process (context);

    buffer.applyGain (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));

    float peakOut = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        peakOut = juce::jmax (peakOut, buffer.getMagnitude (ch, 0, numSamples));
    outputPeak.store (peakOut);
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
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
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
