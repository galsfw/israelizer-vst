#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <memory>
#include <vector>

// Estagio de saturacao "aditiva" (estilo exciter) com sobreamostragem 4x.
//
//   y = x + amount * tanh (drive * x) / drive
//
// O ramo saturado tem ganho ~1 para sinais fracos e comprime/distorce
// conforme a amplitude cresce - e exatamente isso que gera os harmonicos.
// `amount` controla quanto desse ramo e somado no sinal original.
//
// Tudo passa pelo sobreamostrador (mesmo com amount = 0) para que a
// latencia seja constante e a banda grave possa ser atrasada em
// compensacao, mantendo as tres bandas alinhadas no tempo.
class SaturationStage
{
public:
    // Formas de onda disponiveis. Todas usam `u = drive * x` e dividem o
    // resultado por `drive`, de modo que o ganho para sinais fracos fica ~1
    // (a saturacao so "pega" conforme a amplitude cresce).
    enum class Shape
    {
        tanh = 0,   // classica: tanh(g*x)/g - simetrica, suave
        arctan,     // atan(g*x)/g - "redonda", cai mais devagar que o tanh
        cubic,      // soft-clip cubico por tracos - suave, quadra no pico
        rational,   // u/(1+|u|)/g - sem exp(), timbre medio
        valve,      // exponencial ASSIMETRICA - gera harmonicos pares ("quente")
        hard        // corte duro - agressivo (ainda 4x sobreamostrado)
    };

    static constexpr int numShapes = 6;

    // Aplica a forma de onda escolhida. Funcao pura -> testavel sem audio.
    [[nodiscard]] static float applyShape (Shape shape, float x, float drive) noexcept
    {
        const float u = drive * x;

        switch (shape)
        {
            case Shape::arctan:
                return std::atan (u) / drive;

            case Shape::cubic:
                if (u > 1.0f)       return (2.0f / 3.0f) / drive;
                if (u < -1.0f)      return -(2.0f / 3.0f) / drive;
                return (u - u * u * u * (1.0f / 3.0f)) / drive;

            case Shape::rational:
                return (u / (1.0f + std::abs (u))) / drive;

            case Shape::valve:
                // ASSIMETRICA: tanh + termo par (leve retificacao) -> gera
                // harmonicos pares, o "calor" tipico de valvula/transfer macia
                return (std::tanh (u) + 0.15f * (u * u) / (1.0f + u * u)) / drive;

            case Shape::hard:
                return juce::jlimit (-1.0f, 1.0f, u) / drive;

            case Shape::tanh:
            default:
                return std::tanh (u) / drive;
        }
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        // factor = expoente: 2 -> 4x sobreamostragem
        oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) spec.numChannels,
            2,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true,   // qualidade maxima
            true);  // latencia inteira (facilita a compensacao)

        oversampling->initProcessing ((size_t) spec.maximumBlockSize);
        oversampling->reset();

        // As rampas avancam uma vez por amostra do dominio sobreamostrado
        // (4x), por isso o sampleRate * 4: o tempo real do rampa fica 0.02 s.
        amount.reset (spec.sampleRate * 4.0, 0.02);
        drive.reset  (spec.sampleRate * 4.0, 0.02);
        shapeMix.reset (spec.sampleRate * 4.0, 0.01);  // morph de 10 ms
        amount.setCurrentAndTargetValue (0.0f);
        drive.setCurrentAndTargetValue (1.0f);
        shapeMix.setCurrentAndTargetValue (1.0f);

        scratchAmount.assign ((size_t) spec.maximumBlockSize * 4, 0.0f);
        scratchDrive.assign  ((size_t) spec.maximumBlockSize * 4, 1.0f);
        scratchShapeMix.assign ((size_t) spec.maximumBlockSize * 4, 1.0f);
    }

    void reset()
    {
        if (oversampling != nullptr)
            oversampling->reset();
    }

    void setAmount (float newAmount) noexcept
    {
        amount.setTargetValue (juce::jlimit (0.0f, 1.0f, newAmount));
    }

    void setDrive (float newDrive) noexcept
    {
        drive.setTargetValue (juce::jmax (1.0f, newDrive));
    }

    // Sincroniza o estado suavizado (current + target) com os valores atuais
    // dos parametros. Chamado logo apos prepare(): sem isso, um re-prepare
    // reiniciaria as rampas no zero e causaria um "pulo" de 20 ms no volume.
    void syncParameterValues (float newAmount, float newDrive, Shape newShape) noexcept
    {
        amount.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, newAmount));
        drive.setCurrentAndTargetValue  (juce::jmax (1.0f, newDrive));
        previousShape = newShape;
        activeShape   = newShape;
        shapeMix.setCurrentAndTargetValue (1.0f);
    }

    // Troca a forma de onda com um morph continuo de 10 ms (evita estalo).
    void setShape (Shape newShape) noexcept
    {
        if (newShape == activeShape && ! shapeMix.isSmoothing())
            return;

        previousShape = activeShape;
        activeShape   = newShape;
        shapeMix.setCurrentAndTargetValue (0.0f);
        shapeMix.setTargetValue (1.0f);
    }

    [[nodiscard]] int getLatencyInSamples() const noexcept
    {
        return oversampling != nullptr ? (int) std::ceil (oversampling->getLatencyInSamples()) : 0;
    }

    // Aplica a saturacao aditiva in-place nos primeiros `numSamples` amostras.
    void process (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (oversampling == nullptr || numSamples <= 0)
            return;

        juce::dsp::AudioBlock<float> fullBlock (buffer);
        auto block = fullBlock.getSubBlock (0, (size_t) numSamples);

        auto oversampled = oversampling->processSamplesUp (block);

        const size_t n = oversampled.getNumSamples();

        if (scratchAmount.size() < n)
        {
            scratchAmount.resize (n, 0.0f);
            scratchDrive.resize  (n, 1.0f);
            scratchShapeMix.resize (n, 1.0f);
        }

        for (size_t i = 0; i < n; ++i)
        {
            scratchAmount[i]   = amount.getNextValue();
            scratchDrive[i]    = drive.getNextValue();
            scratchShapeMix[i] = shapeMix.getNextValue();
        }

        const bool morphing = shapeMix.isSmoothing()
                              || (n > 0 && scratchShapeMix[n - 1] < 1.0f);

        for (size_t ch = 0; ch < oversampled.getNumChannels(); ++ch)
        {
            auto* data = oversampled.getChannelPointer (ch);

            for (size_t i = 0; i < n; ++i)
            {
                const float x = data[i];
                const float g = scratchDrive[i];

                float saturated;
                if (! morphing)
                {
                    saturated = applyShape (activeShape, x, g);
                }
                else
                {
                    // morph entre a forma anterior e a atual
                    const float a = applyShape (previousShape, x, g);
                    const float b = applyShape (activeShape, x, g);
                    const float t = scratchShapeMix[i];
                    saturated = a + (b - a) * t;
                }

                data[i] = x + scratchAmount[i] * saturated;
            }
        }

        oversampling->processSamplesDown (block);
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    juce::SmoothedValue<float> amount  { 0.0f };
    juce::SmoothedValue<float> drive   { 1.0f };
    juce::SmoothedValue<float> shapeMix { 1.0f };  // 0 = forma anterior, 1 = atual

    Shape activeShape   = Shape::tanh;
    Shape previousShape = Shape::tanh;

    std::vector<float> scratchAmount, scratchDrive, scratchShapeMix;
};
