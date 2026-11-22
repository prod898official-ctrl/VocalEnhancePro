#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalEnhanceProAudioProcessor::VocalEnhanceProAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("In",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    waveBuffer.setSize (1, 512);
    waveBuffer.clear();
}

VocalEnhanceProAudioProcessor::~VocalEnhanceProAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VocalEnhanceProAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{"hpf",1}, "High-Pass", juce::NormalisableRange<float>(20.f,300.f,1.f,0.4f), 80.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{"deess",1}, "De-Esser", juce::NormalisableRange<float>(0.f,1.f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{"comp",1}, "Compress", juce::NormalisableRange<float>(0.f,1.f), 0.45f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{"warm",1}, "Warmth", juce::NormalisableRange<float>(0.f,1.f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{"air",1}, "Air", juce::NormalisableRange<float>(0.f,12.f,0.1f), 3.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID{"out",1}, "Output", juce::NormalisableRange<float>(-24.f,12.f,0.1f), 0.f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID{"bypass",1}, "Bypass", false));

    return { p.begin(), p.end() };
}

void VocalEnhanceProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };

    hpf.prepare (spec);
    comp.prepare (spec);
    makeup.prepare (spec);
    sat.prepare (spec);
    air.prepare (spec);

    sat.functionToUse = [](float x){ return std::tanh(x * 1.35f); };

    updateCoeffs();
}

void VocalEnhanceProAudioProcessor::releaseResources() {}

bool VocalEnhanceProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void VocalEnhanceProAudioProcessor::updateCoeffs()
{
    float f = apvts.getRawParameterValue("hpf")->load();
    *hpf.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, f);

    float g = apvts.getRawParameterValue("air")->load();
    *air.state.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 10500.0, 0.707f, juce::Decibels::decibelsToGain(g));
}

void VocalEnhanceProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, n);

    if (apvts.getRawParameterValue("bypass")->load() > 0.5f)
    {
        inPeak = outPeak = buffer.getMagnitude(0, 0, n);
        return;
    }

    updateCoeffs();

    // waveform
    {
        auto* src = buffer.getReadPointer(0);
        auto* dst = waveBuffer.getWritePointer(0);
        int ws = waveBuffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            dst[i % ws] = src[i];
    }

    inPeak = buffer.getMagnitude(0, 0, n);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    // High-Pass
    hpf.process(ctx);

    // De-Esser
    {
        float amt = apvts.getRawParameterValue("deess")->load();
        float pk = buffer.getMagnitude(0, 0, n);
        deessEnv = (pk > 0.22f) ? deessEnv + (pk - deessEnv) * 0.025f : deessEnv * 0.9f;
        float gr = juce::jlimit(0.35f, 1.0f, 1.0f - deessEnv * amt * 0.65f);
        buffer.applyGain(gr);
    }

    // Compressor + makeup
    {
        float a = apvts.getRawParameterValue("comp")->load();
        comp.setThreshold(juce::jmap(a, 0.f, 1.f, -7.f, -26.f));
        comp.setRatio(juce::jmap(a, 0.f, 1.f, 1.6f, 5.5f));
        comp.setAttack(9.f);
        comp.setRelease(110.f);
        comp.process(ctx);

        makeup.setGainDecibels(juce::jmap(a, 0.f, 1.f, 0.f, 7.5f));
        makeup.process(ctx);
    }

    // Warmth
    {
        float w = apvts.getRawParameterValue("warm")->load();
        if (w > 0.01f)
        {
            buffer.applyGain(1.f + w * 1.55f);
            sat.process(ctx);
            buffer.applyGain(1.f / (1.f + w * 0.82f));
        }
    }

    // Air
    air.process(ctx);

    // Output
    buffer.applyGain(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("out")->load()));

    outPeak = buffer.getMagnitude(0, 0, n);
}

void VocalEnhanceProAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void VocalEnhanceProAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* VocalEnhanceProAudioProcessor::createEditor()
{
    return new VocalEnhanceProAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalEnhanceProAudioProcessor();
}
