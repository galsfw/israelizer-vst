#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Biquad RBJ (Audio EQ Cookbook) em forma direta II transposed.
//
// Duas cascatas identicas com Q = 1/sqrt(2) formam um corte
// Linkwitz-Riley de 4a ordem (24 dB/oct), que e o que usamos aqui
// para dividir as bandas sem buracos na resposta.
class Biquad
{
public:
    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setLowPass (double sampleRate, double frequency, double q = 0.7071067811865476) noexcept
    {
        const double w0      = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
        const double cosW0   = std::cos (w0);
        const double sinW0   = std::sin (w0);
        const double alpha   = sinW0 / (2.0 * q);

        const double b0 = (1.0 - cosW0) * 0.5;
        const double b1 = 1.0 - cosW0;
        const double b2 = (1.0 - cosW0) * 0.5;
        const double a0 = 1.0 + alpha;
        const double a1 = -2.0 * cosW0;
        const double a2 = 1.0 - alpha;

        this->b0 = (float) (b0 / a0);
        this->b1 = (float) (b1 / a0);
        this->b2 = (float) (b2 / a0);
        this->a1 = (float) (a1 / a0);
        this->a2 = (float) (a2 / a0);
    }

    [[nodiscard]] float processSample (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};
