#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <sound_meter/sound_meter.h>
#include "PluginProcessor.h"

class SaturAirAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit SaturAirAudioProcessorEditor (SaturAirAudioProcessor&);
    ~SaturAirAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;  // faz poll dos picos -> medidores

    using SliderAttachment    = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void setupRotary (juce::Slider&, juce::Label&,
                      const juce::String& text, const juce::String& paramID);
    void setupLinear (juce::Slider&, juce::Label&,
                      const juce::String& text, const juce::String& paramID);

    SaturAirAudioProcessor& processorRef;

    juce::Slider highSlider, airSlider, outputSlider;
    juce::Slider lowMidSlider, midHighSlider;

    juce::ComboBox shapeBox;
    juce::Label shapeLabel;
    std::unique_ptr<ComboBoxAttachment> shapeAttachment;

    juce::Label titleLabel;
    juce::Label highLabel, airLabel, outputLabel;
    juce::Label lowMidLabel, midHighLabel;

    sd::SoundMeter::MetersComponent inMeters, outMeters;

    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturAirAudioProcessorEditor)
};
