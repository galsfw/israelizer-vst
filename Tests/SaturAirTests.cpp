// Testes unitarios do SaturAir (Catch2 v3).
//
// Cobrem as garantias centrais do esqueleto:
//   * o divisor de bandas e complementar (low + mid + high == entrada)
//   * o divisor separa de fato (sino grave -> low; sino agudo -> high)
//   * o atraso seco desloca exatamente N amostras
//   * a latencia reportada bate com o atraso real do sobreamostrador
//   * amount = 0 deixa o sinal praticamente intacto
//   * todas as formas de onda tem ganho ~1 em sinais fracos e sao limitadas

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdint>

#include "Dsp/BandSplitter.h"
#include "Dsp/Biquad.h"
#include "Dsp/SampleDelay.h"
#include "Dsp/SaturationStage.h"

using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 44100.0;
    constexpr int    kBlockSize  = 256;

    float rms (const juce::AudioBuffer<float>& b, int ch, int start, int count)
    {
        double acc = 0.0;
        for (int i = 0; i < count; ++i)
        {
            const double v = b.getSample (ch, start + i);
            acc += v * v;
        }
        return (float) std::sqrt (acc / juce::jmax (1, count));
    }

    // Ruido deterministico (LCG) - mesmo padrao em toda execucao.
    struct Noise
    {
        std::uint32_t state = 0x12345678u;

        float next()
        {
            state = state * 1664525u + 1013904223u;
            return ((float) (state >> 8) / 8388608.0f) - 1.0f;  // [-1, 1)
        }
    };

    float sineSample (double hz, int i)
    {
        return (float) std::sin (2.0 * juce::MathConstants<double>::pi * hz * i / kSampleRate);
    }
}

//==============================================================================
TEST_CASE ("BandSplitter: as tres bandas somam exatamente a entrada")
{
    BandSplitter splitter;
    splitter.prepare (kSampleRate, 2, kBlockSize);

    juce::AudioBuffer<float> input (2, kBlockSize);
    Noise noise;

    // Varios blocos seguidos (com estado dos filtros ja exercitado).
    for (int block = 0; block < 8; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            const float s = 0.4f * sineSample (440.0, i);
            input.setSample (0, i, s + 0.3f * noise.next());
            input.setSample (1, i, s - 0.3f * noise.next());
        }

        splitter.split (input);

        const auto& low  = splitter.getLowBand();
        const auto& mid  = splitter.getMidBand();
        const auto& high = splitter.getHighBand();

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kBlockSize; ++i)
            {
                const float sum = low.getSample (ch, i) + mid.getSample (ch, i) + high.getSample (ch, i);
                REQUIRE (std::abs (sum - input.getSample (ch, i)) < 1.0e-5f);
            }
    }
}

TEST_CASE ("BandSplitter: separa de fato as bandas")
{
    BandSplitter splitter;
    splitter.prepare (kSampleRate, 1, kBlockSize);

    juce::AudioBuffer<float> input (1, kBlockSize);

    SECTION ("sino grave (200 Hz) fica na banda low")
    {
        for (int block = 0; block < 8; ++block)
        {
            for (int i = 0; i < kBlockSize; ++i)
                input.setSample (0, i, 0.8f * sineSample (200.0, block * kBlockSize + i));
            splitter.split (input);
        }

        const float lowRms  = rms (splitter.getLowBand(),  0, 0, kBlockSize);
        const float midRms  = rms (splitter.getMidBand(),  0, 0, kBlockSize);
        const float highRms = rms (splitter.getHighBand(), 0, 0, kBlockSize);

        REQUIRE (lowRms > 4.0f * midRms);
        REQUIRE (lowRms > 4.0f * highRms);
    }

    SECTION ("sino agudo (12 kHz) fica na banda high")
    {
        for (int block = 0; block < 8; ++block)
        {
            for (int i = 0; i < kBlockSize; ++i)
                input.setSample (0, i, 0.8f * sineSample (12000.0, block * kBlockSize + i));
            splitter.split (input);
        }

        const float lowRms  = rms (splitter.getLowBand(),  0, 0, kBlockSize);
        const float midRms  = rms (splitter.getMidBand(),  0, 0, kBlockSize);
        const float highRms = rms (splitter.getHighBand(), 0, 0, kBlockSize);

        REQUIRE (highRms > 2.0f * midRms);
        REQUIRE (highRms > 4.0f * lowRms);
    }
}

TEST_CASE ("Biquad: ganho DC = 1 e atenua bem acima do corte")
{
    Biquad lp;
    lp.setLowPass (kSampleRate, 500.0);

    float y = 0.0f;
    for (int i = 0; i < 4000; ++i)
        y = lp.processSample (1.0f);

    REQUIRE (y == Approx (1.0f).margin (1.0e-3f));

    Biquad lp2;
    lp2.setLowPass (kSampleRate, 500.0);

    double inAcc = 0.0, outAcc = 0.0;
    for (int i = 0; i < 4000; ++i)
    {
        const float x = sineSample (8000.0, i);
        const float o = lp2.processSample (x);

        if (i > 1000)  // ignora o transitorio inicial
        {
            inAcc  += (double) x * x;
            outAcc += (double) o * o;
        }
    }

    REQUIRE (std::sqrt (outAcc) < 0.05 * std::sqrt (inAcc));
}

//==============================================================================
TEST_CASE ("SampleDelay: delay 0 = passa direto; delay N = atrasa exatamente N")
{
    SampleDelay d;
    d.prepare (1, 16);

    d.setDelay (0);
    for (int i = 0; i < 8; ++i)
    {
        const float v = (float) (i + 1);
        REQUIRE (d.processSample (0, v) == v);
    }

    SampleDelay d2;
    d2.prepare (1, 16);
    d2.setDelay (5);

    // Impulso na amostra 0 deve sair exatamente na amostra 5.
    for (int t = 0; t < 5; ++t)
        REQUIRE (d2.processSample (0, t == 0 ? 1.0f : 0.0f) == 0.0f);

    REQUIRE (d2.processSample (0, 0.0f) == 1.0f);
}

//==============================================================================
TEST_CASE ("SaturationStage: a latencia reportada = atraso real do sobreamostrador")
{
    SaturationStage stage;
    const juce::dsp::ProcessSpec spec { kSampleRate, (juce::uint32) kBlockSize, 1 };
    stage.prepare (spec);
    stage.syncParameterValues (0.5f, 6.0f, SaturationStage::Shape::tanh);
    stage.reset();

    const int latency = stage.getLatencyInSamples();
    REQUIRE (latency >= 0);

    juce::AudioBuffer<float> buf (1, kBlockSize);
    buf.clear();
    buf.setSample (0, 0, 1.0f);

    stage.process (buf, kBlockSize);

    int   peakIdx = 0;
    float peak    = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        const float a = std::abs (buf.getSample (0, i));
        if (a > peak)
        {
            peak    = a;
            peakIdx = i;
        }
    }

    REQUIRE (peak > 0.01f);                                   // o impulso atravessa
    REQUIRE (std::abs (peakIdx - latency) <= 3);              // e chega na hora certa
}

TEST_CASE ("SaturationStage: amount = 0 deixa o sinal intacto")
{
    SaturationStage stage;
    const juce::dsp::ProcessSpec spec { kSampleRate, (juce::uint32) kBlockSize, 1 };
    stage.prepare (spec);
    stage.syncParameterValues (0.0f, 6.0f, SaturationStage::Shape::tanh);
    stage.reset();

    juce::AudioBuffer<float> buf (1, kBlockSize);

    double inAcc = 0.0, outAcc = 0.0;

    for (int block = 0; block < 8; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
            buf.setSample (0, i, 0.5f * sineSample (1000.0, block * kBlockSize + i));

        // mede a entrada ANTES do processamento (in-place)
        const float inRms  = rms (buf, 0, 0, kBlockSize);
        stage.process (buf, kBlockSize);
        const float outRms = rms (buf, 0, 0, kBlockSize);

        if (block >= 4)  // depois do transitorio do sobreamostrador
        {
            inAcc  += (double) inRms * inRms;
            outAcc += (double) outRms * outRms;
        }
    }

    const double inLevel  = std::sqrt (inAcc / 4.0);
    const double outLevel = std::sqrt (outAcc / 4.0);

    REQUIRE (outLevel == Approx (inLevel).margin (0.1 * inLevel));
}

//==============================================================================
TEST_CASE ("applyShape: ganho ~1 para sinais fracos em todas as formas")
{
    using S = SaturationStage::Shape;

    const S shapes[] = { S::tanh, S::arctan, S::cubic, S::rational, S::valve, S::hard };
    const float xs[] = { 1.0e-5f, -1.0e-5f, 1.0e-3f, -1.0e-3f };
    const float gs[] = { 1.0f, 4.0f, 6.0f };

    for (auto shape : shapes)
        for (float x : xs)
            for (float g : gs)
                REQUIRE (SaturationStage::applyShape (shape, x, g) == Approx (x).margin (1.0e-5f));
}

TEST_CASE ("applyShape: saida limitada mesmo para sinais gigantes")
{
    using S = SaturationStage::Shape;

    const S shapes[] = { S::tanh, S::arctan, S::cubic, S::rational, S::valve, S::hard };

    for (auto shape : shapes)
        for (float x : { 1.0e6f, -1.0e6f, 1.0e3f })
            REQUIRE (std::abs (SaturationStage::applyShape (shape, x, 4.0f)) <= 1.0f);
}

TEST_CASE ("applyShape: simetricas sao impares; valve e assimetrica")
{
    using S = SaturationStage::Shape;

    // Formas impares: f(x) == -f(-x)
    for (auto shape : { S::tanh, S::arctan, S::cubic, S::rational, S::hard })
    {
        const float pos = SaturationStage::applyShape (shape, 0.5f, 4.0f);
        const float neg = SaturationStage::applyShape (shape, -0.5f, 4.0f);
        REQUIRE (std::abs (pos + neg) < 1.0e-6f);
    }

    // Valve quebra a simetria (gera harmonicos pares)
    const float vPos = SaturationStage::applyShape (S::valve, 0.5f, 4.0f);
    const float vNeg = SaturationStage::applyShape (S::valve, -0.5f, 4.0f);
    REQUIRE (std::abs (vPos + vNeg) > 1.0e-3f);
}
