#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Editor inspirado no visual do Fresh Air: painel escuro em cima com um
// arco de medidor de saida e o titulo, painel claro embaixo com dois
// knobs grandes (High/Air), pill do Shape, POWER e TRIM.
class SaturAirAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit SaturAirAudioProcessorEditor (SaturAirAudioProcessor&);
    ~SaturAirAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;  // picos do arco + medidores

    // Visual proprio: knobs claros com valor dentro, pill, etc.
    struct AirLookAndFeel : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPos, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider&) override;

        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;

        void drawComboBox (juce::Graphics&, int width, int height, bool,
                           int, int, int, int, juce::ComboBox&) override;

        void drawButtonBackground (juce::Graphics&, juce::Button&,
                                   const juce::Colour& backgroundColour,
                                   bool highlighted, bool down) override;

        void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                               bool highlighted, bool down) override;
    };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void setupKnob (juce::Slider&, juce::Label&, const juce::String& text,
                    const juce::String& paramID);
    void setupCrossSlider (juce::Slider&, juce::Label&,
                           const juce::String& text, const juce::String& paramID);

    SaturAirAudioProcessor& processorRef;

    AirLookAndFeel airLnf;

    // Painel claro: dois knobs grandes
    juce::Slider highSlider, airSlider;
    juce::Label  highLabel, airLabel;
    juce::Label  highRangeMin, highRangeMax;   // "0" / "100"
    juce::Label  airRangeMin,  airRangeMax;

    // Crossovers (linha fina acima do rodape)
    juce::Slider lowMidSlider, midHighSlider;
    juce::Label  lowMidLabel, midHighLabel;

    // Pill central = seletor de Shape
    juce::ComboBox shapeBox;
    juce::Label    shapeLabel;
    std::unique_ptr<ComboBoxAttachment> shapeAttachment;

    // POWER (toggle) e TRIM (knob pequeno de saida)
    juce::ToggleButton powerButton;
    juce::Label        powerLabel;
    std::unique_ptr<ButtonAttachment> powerAttachment;

    juce::Slider outputSlider;   // TRIM
    juce::Label  outputLabel;

    juce::Label titleLabel;      // "S A T U R  A I R" no painel escuro

    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    // Retangulos calculados no resized() e usados no paint()
    juce::Rectangle<int> darkPanel, lightPanel, arcArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturAirAudioProcessorEditor)
};
