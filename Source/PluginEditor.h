#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class SaturAirAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SaturAirAudioProcessorEditor (SaturAirAudioProcessor&);
    ~SaturAirAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void setupRotary (juce::Slider&, juce::Label&,
                      const juce::String& text, const juce::String& paramID);
    void setupLinear (juce::Slider&, juce::Label&,
                      const juce::String& text, const juce::String& paramID);

    SaturAirAudioProcessor& processorRef;

    juce::Slider highSlider, airSlider, outputSlider;
    juce::Slider lowMidSlider, midHighSlider;

    juce::Label titleLabel;
    juce::Label highLabel, airLabel, outputLabel;
    juce::Label lowMidLabel, midHighLabel;

    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturAirAudioProcessorEditor)
};
