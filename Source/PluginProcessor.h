#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Dsp/BandSplitter.h"
#include "Dsp/SampleDelay.h"
#include "Dsp/SaturationStage.h"

// SaturAir - exciter/saturador multibanda no espirito do Fresh Air.
//
// Logica do sinal:
//
//   entrada ──┬─ LP 2.5k ──────────────────────────────┬─ atraso(L) ─┐
//             │                                        │             │
//             └─ (x - low) ─ LP 8k ── banda mid ── sat ── "High" ──► (+) ─ gain ─ saida
//                     │                                    │         ▲
//                     └─ (x - low) - mid ── banda high ── sat ── "Air" ─┘
//
// * Só as bandas de cima sao saturadas (sobreamostradas 4x) e somadas
//   de forma aditiva - e isso que cria os harmonicos de "brilho/ar".
// * A banda grave fica seca e recebe um atraso igual a latencia dos
//   sobreamostradores, para nao haver desalinhamento na soma.
// * Latencia reportada ao host = latencia do sobreamostrador.
class SaturAirAudioProcessor : public juce::AudioProcessor
{
public:
    SaturAirAudioProcessor();
    ~SaturAirAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true; }

    const juce::String getName() const override            { return JucePlugin_Name; }
    bool acceptsMidi() const override                      { return false; }
    bool producesMidi() const override                     { return false; }
    bool isMidiEffect() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0.0; }

    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override       { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* highParam    = nullptr;  // "High": excitacao da banda mid
    std::atomic<float>* airParam     = nullptr;  // "Air":  excitacao da banda alta
    std::atomic<float>* outputParam  = nullptr;  // ganho de saida (dB)
    std::atomic<float>* lowMidParam  = nullptr;  // 1o crossover (Hz)
    std::atomic<float>* midHighParam = nullptr;  // 2o crossover (Hz)

    // --- DSP ---
    BandSplitter   splitter;
    SaturationStage midSaturator;  // banda mid  -> parametro "High"
    SaturationStage airSaturator;  // banda alta -> parametro "Air"
    SampleDelay    dryDelay;       // compensa a latencia da banda grave

    juce::SmoothedValue<float> outputGain { 1.0f };

    // Caracteristica de cada banda - ajustar ao gosto.
    static constexpr float midDrive = 6.0f;
    static constexpr float airDrive = 4.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturAirAudioProcessor)
};
