#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> frequencyRange (float centreHz)
    {
        juce::NormalisableRange<float> range (100.0f, 16000.0f, 1.0f);
        range.setSkewForCentre (centreHz);
        return range;
    }
}

SaturAirAudioProcessor::SaturAirAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, juce::Identifier ("PARAMETERS"), createParameterLayout())
{
    highParam    = apvts.getRawParameterValue ("high");
    airParam     = apvts.getRawParameterValue ("air");
    outputParam  = apvts.getRawParameterValue ("output");
    lowMidParam  = apvts.getRawParameterValue ("lowMidHz");
    midHighParam = apvts.getRawParameterValue ("midHighHz");
    shapeParam   = apvts.getRawParameterValue ("shape");
    powerParam   = apvts.getRawParameterValue ("power");
}

juce::AudioProcessorValueTreeState::ParameterLayout SaturAirAudioProcessor::createParameterLayout()
{
    using Attributes = juce::AudioParameterFloatAttributes;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::NormalisableRange<float> percentRange (0.0f, 100.0f, 0.1f);
    const juce::NormalisableRange<float> outputRange  (-24.0f, 24.0f, 0.1f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "high", 1 }, "High", percentRange, 0.0f,
        Attributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "air", 1 }, "Air", percentRange, 0.0f,
        Attributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "shape", 1 }, "Shape",
        juce::StringArray { "Tanh", "ArcTan", "Cubic", "Rational", "Valve", "Hard" }, 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "power", 1 }, "Power", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output", outputRange, 0.0f,
        Attributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lowMidHz", 1 }, "Low/Mid Xover",
        frequencyRange ((float) BandSplitter::defaultLowMidHz), (float) BandSplitter::defaultLowMidHz,
        Attributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "midHighHz", 1 }, "Mid/High Xover",
        frequencyRange ((float) BandSplitter::defaultMidHighHz), (float) BandSplitter::defaultMidHighHz,
        Attributes().withLabel ("Hz")));

    return layout;
}

void SaturAirAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int numChannels = juce::jmax (1, juce::jmax (getTotalNumInputChannels(),
                                                       getTotalNumOutputChannels()));

    splitter.prepare (sampleRate, numChannels, samplesPerBlock);

    const juce::dsp::ProcessSpec spec { sampleRate,
                                        (juce::uint32) juce::jmax (1, samplesPerBlock),
                                        (juce::uint32) numChannels };

    midSaturator.prepare (spec);
    airSaturator.prepare (spec);

    // Um re-prepare nao deve reiniciar as rampas do zero: sincroniza o estado
    // suavizado com os valores atuais dos parametros (evita um "pulo" de
    // volume de 20 ms ao trocar sample rate/buffer no meio da sessao).
    midSaturator.syncParameterValues (highParam->load() * 0.01f, midDrive,
                                      (SaturationStage::Shape) ((int) shapeParam->load()));
    airSaturator.syncParameterValues (airParam->load() * 0.01f, airDrive,
                                      (SaturationStage::Shape) ((int) shapeParam->load()));

    const int latency = juce::jmax (midSaturator.getLatencyInSamples(),
                                    airSaturator.getLatencyInSamples());

    dryDelay.prepare (numChannels, latency);
    dryDelay.setDelay (latency);
    setLatencySamples (latency);

    outputGain.reset (sampleRate, 0.03);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
}

void SaturAirAudioProcessor::releaseResources() {}

bool SaturAirAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void SaturAirAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples == 0)
        return;

    // Defesa: os buffers de banda tem o tamanho passado em prepareToPlay.
    // Se um host mandar mais amostras do que o prometido, processa o que
    // cabe e zera o resto em vez de estourar o buffer.
    const int n = juce::jmin (numSamples, splitter.getMaxBlockSize());

    const int numChannels = juce::jmin (buffer.getNumChannels(), splitter.getNumChannels());

    // Medidores: pico da ENTRADA, medido antes de o buffer ser reescrito.
    const int measuredIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    for (int ch = 0; ch < 2; ++ch)
        inputPeaks[ch].store (ch < measuredIn ? buffer.getMagnitude (ch, 0, n) : 0.0f,
                              std::memory_order_relaxed);

    // Atualiza os parametros do bloco atual.
    splitter.setLowMidHz  ((double) lowMidParam->load());
    splitter.setMidHighHz ((double) midHighParam->load());

    const auto shape = (SaturationStage::Shape) ((int) shapeParam->load());

    // POWER: quando desligado, os saturadores nao adicionam nada (a soma das
    // bandas continua exata, entao o sinal passa intacto e com a mesma latencia).
    const float powerMix = powerParam->load() > 0.5f ? 1.0f : 0.0f;

    midSaturator.setDrive  (midDrive);
    midSaturator.setAmount (highParam->load() * 0.01f * powerMix);
    midSaturator.setShape  (shape);

    airSaturator.setDrive  (airDrive);
    airSaturator.setAmount (airParam->load() * 0.01f * powerMix);
    airSaturator.setShape  (shape);

    // Divide em bandas e satura so as de cima.
    splitter.split (buffer);
    midSaturator.process (splitter.getMidBand(),  n);
    airSaturator.process (splitter.getHighBand(), n);

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    // Pre-computa a rampa de saida UMA vez por bloco. Consumir getNextValue()
    // dentro do loop de canais faria a rampa avancar por canal: L e R
    // receberiam ganhos diferentes durante a automacao (e a rampa correria
    // em velocidade x numero de canais).
    if ((int) outputScratch.size() < n)
        outputScratch.resize ((size_t) n, 0.0f);

    for (int i = 0; i < n; ++i)
        outputScratch[(size_t) i] = outputGain.getNextValue();

    // Soma as bandas (grave atrasada = compensacao de latencia) e aplica o ganho.
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* low  = splitter.getLowBand().getReadPointer (ch);
        const float* mid  = splitter.getMidBand().getReadPointer (ch);
        const float* high = splitter.getHighBand().getReadPointer (ch);
        float* out = buffer.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            const float lowDelayed = dryDelay.processSample (ch, low[i]);
            out[i] = (lowDelayed + mid[i] + high[i]) * outputScratch[(size_t) i];
        }

        if (n < numSamples)
            buffer.clear (ch, n, numSamples - n);
    }

    // Medidores: pico da SAIDA (apos a soma das bandas e o ganho).
    for (int ch = 0; ch < 2; ++ch)
        outputPeaks[ch].store (ch < numChannels ? buffer.getMagnitude (ch, 0, n) : 0.0f,
                               std::memory_order_relaxed);

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* SaturAirAudioProcessor::createEditor()
{
    return new SaturAirAudioProcessorEditor (*this);
}

void SaturAirAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SaturAirAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Isso cria a fabrica de plugins para o host encontrar.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturAirAudioProcessor();
}
