#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Biquad.h"
#include <vector>

// Divisor de bandas em 3 vias (mesma logica do Fresh Air):
//
//   low  : abaixo de lowMidHz            -> fica "seco" (soma direta)
//   mid  : lowMidHz .. midHighHz         -> saturado, controla o "High"
//   high : acima de midHighHz            -> saturado, controla o "Air"
//
// A divisao e complementar por construcao:
//
//   low  = LP4(x, lowMidHz)
//   mid  = LP4(x - low, midHighHz)
//   high = (x - low) - mid
//
// logo  low + mid + high == x  sempre (sem buracos de fase na soma).
// LP4 = duas cascatas de Butterworth (Q = 1/sqrt(2)) => Linkwitz-Riley 4a ordem.
class BandSplitter
{
public:
    static constexpr double defaultLowMidHz  = 2500.0;
    static constexpr double defaultMidHighHz = 8000.0;

    void prepare (double newSampleRate, int numberOfChannels, int maximumBlockSize)
    {
        sampleRate    = newSampleRate;
        numChannels   = juce::jmax (1, numberOfChannels);
        maxBlockSize  = juce::jmax (1, maximumBlockSize);

        lowBand.       setSize (numChannels, maxBlockSize);
        midHighScratch.setSize (numChannels, maxBlockSize);
        midBand.       setSize (numChannels, maxBlockSize);
        highBand.      setSize (numChannels, maxBlockSize);

        lowMidStageA.resize  ((size_t) numChannels);
        lowMidStageB.resize  ((size_t) numChannels);
        midHighStageA.resize ((size_t) numChannels);
        midHighStageB.resize ((size_t) numChannels);

        lowMidHz  = clampFrequency (lowMidHz);
        midHighHz = clampFrequency (midHighHz);

        updateFilters();
        reset();
    }

    void setLowMidHz (double hz)
    {
        const double v = clampFrequency (hz);
        if (v == lowMidHz)
            return;

        lowMidHz = v;
        updateFilters();
    }

    void setMidHighHz (double hz)
    {
        const double v = clampFrequency (hz);
        if (v == midHighHz)
            return;

        midHighHz = v;
        updateFilters();
    }

    void reset()
    {
        for (auto* stage : { &lowMidStageA, &lowMidStageB, &midHighStageA, &midHighStageB })
            for (auto& f : **stage)
                f.reset();
    }

    // Copia `input` para os buffers de banda (low / mid / high).
    void split (const juce::AudioBuffer<float>& input)
    {
        const int numSamples = juce::jmin (input.getNumSamples(), maxBlockSize);
        const int channels   = juce::jmin (input.getNumChannels(), numChannels);

        jassert (input.getNumSamples() <= maxBlockSize);

        for (int ch = 0; ch < channels; ++ch)
        {
            const float* src  = input.getReadPointer (ch);
            float*       low  = lowBand.getWritePointer (ch);
            float*       scr  = midHighScratch.getWritePointer (ch);
            float*       mid  = midBand.getWritePointer (ch);
            float*       high = highBand.getWritePointer (ch);

            auto& lp1a = lowMidStageA[(size_t) ch];
            auto& lp1b = lowMidStageB[(size_t) ch];
            auto& lp2a = midHighStageA[(size_t) ch];
            auto& lp2b = midHighStageB[(size_t) ch];

            for (int i = 0; i < numSamples; ++i)
            {
                const float x = src[i];

                low[i]  = lp1b.processSample (lp1a.processSample (x));
                scr[i]  = x - low[i];
                mid[i]  = lp2b.processSample (lp2a.processSample (scr[i]));
                high[i] = scr[i] - mid[i];
            }
        }

        // Canais nao usados: zera para a soma nao pegar lixo.
        for (int ch = channels; ch < numChannels; ++ch)
        {
            lowBand.clear  (ch, 0, numSamples);
            midBand.clear  (ch, 0, numSamples);
            highBand.clear (ch, 0, numSamples);
        }
    }

    [[nodiscard]] int getNumChannels() const noexcept          { return numChannels; }
    [[nodiscard]] int getMaxBlockSize() const noexcept         { return maxBlockSize; }
    [[nodiscard]] juce::AudioBuffer<float>& getLowBand()       { return lowBand; }
    [[nodiscard]] juce::AudioBuffer<float>& getMidBand()       { return midBand; }
    [[nodiscard]] juce::AudioBuffer<float>& getHighBand()      { return highBand; }

private:
    [[nodiscard]] double clampFrequency (double hz) const noexcept
    {
        const double hi = sampleRate > 0.0 ? sampleRate * 0.45 : 20000.0;
        return juce::jlimit (20.0, hi, hz);
    }

    void updateFilters()
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            lowMidStageA[(size_t) ch].setLowPass (sampleRate, lowMidHz);
            lowMidStageB[(size_t) ch].setLowPass (sampleRate, lowMidHz);
            midHighStageA[(size_t) ch].setLowPass (sampleRate, midHighHz);
            midHighStageB[(size_t) ch].setLowPass (sampleRate, midHighHz);
        }
    }

    double sampleRate   = 44100.0;
    int numChannels     = 0;
    int maxBlockSize    = 0;

    double lowMidHz     = defaultLowMidHz;
    double midHighHz    = defaultMidHighHz;

    juce::AudioBuffer<float> lowBand, midHighScratch, midBand, highBand;

    std::vector<Biquad> lowMidStageA, lowMidStageB;
    std::vector<Biquad> midHighStageA, midHighStageB;
};
