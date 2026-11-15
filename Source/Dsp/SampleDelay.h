#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <vector>

// Atraso inteiro por canal.
//
// Usado para compensar a latencia dos sobreamostradores: a banda grave nao
// passa pelos saturadores, entao ela e atrasada pela mesma quantidade de
// amostras para que low + mid + high continuem alinhados no tempo.
class SampleDelay
{
public:
    void prepare (int numberOfChannels, int maxDelaySamples)
    {
        numChannels = juce::jmax (1, numberOfChannels);
        maxDelay    = juce::jmax (0, maxDelaySamples);

        const int size = maxDelay + 1;

        buffers.resize ((size_t) numChannels);
        for (auto& b : buffers)
            b.assign ((size_t) size, 0.0f);

        writePos.assign ((size_t) numChannels, 0);
        delay = 0;
    }

    void setDelay (int samples) noexcept
    {
        delay = juce::jlimit (0, maxDelay, samples);
    }

    void reset() noexcept
    {
        for (auto& b : buffers)
            std::fill (b.begin(), b.end(), 0.0f);

        std::fill (writePos.begin(), writePos.end(), 0);
    }

    // Escreve a amostra e devolve a amostra correspondente, atrasada em
    // `delay` amostras (delay == 0 devolve a propria amostra).
    [[nodiscard]] float processSample (int channel, float sample) noexcept
    {
        auto& buf = buffers[(size_t) channel];
        auto& pos = writePos[(size_t) channel];

        const int size = (int) buf.size();

        buf[(size_t) pos] = sample;

        int readPos = pos - delay;
        if (readPos < 0)
            readPos += size;

        const float out = buf[(size_t) readPos];

        pos = (pos + 1) % size;
        return out;
    }

private:
    std::vector<std::vector<float>> buffers;
    std::vector<int> writePos;

    int numChannels = 0;
    int maxDelay    = 0;
    int delay       = 0;
};
