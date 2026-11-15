#include "PluginEditor.h"

SaturAirAudioProcessorEditor::SaturAirAudioProcessorEditor (SaturAirAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    titleLabel.setText ("SaturAir - exciter multibanda com saturação",
                        juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe8eef7));
    addAndMakeVisible (titleLabel);

    setupRotary (highSlider,   highLabel,   "High",   "high");
    setupRotary (airSlider,    airLabel,    "Air",    "air");
    setupRotary (outputSlider, outputLabel, "Output", "output");

    setupLinear (lowMidSlider,  lowMidLabel,  "Low/Mid Xover",  "lowMidHz");
    setupLinear (midHighSlider, midHighLabel, "Mid/High Xover", "midHighHz");

    setResizable (true, true);
    setResizeLimits (440, 320, 1100, 700);
    setSize (520, 320);
}

void SaturAirAudioProcessorEditor::setupRotary (juce::Slider& slider, juce::Label& label,
                                                const juce::String& text, const juce::String& paramID)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18);
    slider.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffb9c4d4));
    addAndMakeVisible (label);

    attachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts,
                                                               paramID, slider));
}

void SaturAirAudioProcessorEditor::setupLinear (juce::Slider& slider, juce::Label& label,
                                                const juce::String& text, const juce::String& paramID)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 84, 18);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffb9c4d4));
    addAndMakeVisible (label);

    attachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts,
                                                               paramID, slider));
}

void SaturAirAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff232b36),
                                              bounds.getCentreX(), 0.0f,
                                              juce::Colour (0xff12161c),
                                              bounds.getCentreX(), bounds.getHeight(),
                                              false));
    g.fillAll();

    g.setColour (juce::Colour (0x338fb4ff));
    g.drawRect (getLocalBounds().toFloat(), 1.0f);
}

void SaturAirAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);

    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (6);

    auto knobRow = area.removeFromTop (168);
    const int knobWidth = knobRow.getWidth() / 3;

    auto placeKnob = [] (juce::Slider& slider, juce::Label& label, juce::Rectangle<int> bounds)
    {
        label.setBounds (bounds.removeFromTop (20));
        slider.setBounds (bounds);
    };

    placeKnob (highSlider,   highLabel,   knobRow.removeFromLeft (knobWidth));
    placeKnob (airSlider,    airLabel,    knobRow.removeFromLeft (knobWidth));
    placeKnob (outputSlider, outputLabel, knobRow);

    area.removeFromTop (10);

    auto placeLinear = [] (juce::Slider& slider, juce::Label& label, juce::Rectangle<int> bounds)
    {
        label.setBounds (bounds.removeFromLeft (130));
        slider.setBounds (bounds);
    };

    placeLinear (lowMidSlider, lowMidLabel, area.removeFromTop (28));
    area.removeFromTop (6);
    placeLinear (midHighSlider, midHighLabel, area.removeFromTop (28));
}
