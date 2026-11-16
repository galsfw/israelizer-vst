#include "PluginEditor.h"

//==============================================================================
// Visual proprio (LookAndFeel): knobs claros, pill e botoes estilo Fresh Air.
//==============================================================================
void SaturAirAudioProcessorEditor::AirLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    const auto& props = slider.getProperties();

    const float r  = juce::jmin (w, h) * 0.5f - 2.0f;
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto accent = juce::Colour ((juce::uint32) (juce::int64)
                          props.getWithDefault ("accent", (juce::int64) 0));
    const juce::String caption = props.getWithDefault ("caption", "").toString();

    // sombra macia
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillEllipse (cx - r, cy - r + 3.0f, r * 2.0f, r * 2.0f);

    // corpo do knob (branco no topo -> azul bem claro embaixo)
    juce::ColourGradient body (juce::Colours::white.withAlpha (0.98f), cx, cy - r,
                               juce::Colour (0xffdfe9f1), cx, cy + r, false);
    g.setGradientFill (body);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colour (0x28000000));
    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

    // arco de valor (cor de destaque) colado na borda, do inicio ao valor atual
    if (sliderPos > 0.001f && accent.getAlpha() > 0.0f)
    {
        juce::Path arc;
        const float ar = r - 1.5f;
        arc.addCentredArc (cx, cy, ar, ar, 0.0f,
                           rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (arc, juce::PathStrokeType (3.0f,
                         juce::PathStrokeType::curved,
                         juce::PathStrokeType::rounded));
    }

    // marca (tick) da posicao atual
    {
        const float t0 = r * 0.62f, t1 = r * 0.88f;
        g.setColour (juce::Colour (0xff9aa9b6));
        g.drawLine (cx + std::cos (angle) * t0, cy + std::sin (angle) * t0,
                    cx + std::cos (angle) * t1, cy + std::sin (angle) * t1,
                    2.0f);
    }

    // valor no centro
    g.setColour (juce::Colour (0xff6d7f8f));
    g.setFont (juce::Font (juce::FontOptions (juce::jmax (9.0f, r * 0.52f))));
    g.drawText (slider.getTextFromValue (slider.getValue()),
                juce::Rectangle<float> (cx - r, cy - r * 0.58f, r * 2.0f, r * 0.62f),
                juce::Justification::centred, false);

    // legenda embaixo do valor (ex.: "MID AIR")
    if (caption.isNotEmpty())
    {
        g.setColour (juce::Colour (0xff8fa0ae));
        g.setFont (juce::Font (juce::FontOptions (juce::jmax (7.0f, r * 0.15f))));
        g.drawText (caption,
                    juce::Rectangle<float> (cx - r, cy + r * 0.06f, r * 2.0f, r * 0.34f),
                    juce::Justification::centred, false);
    }
}

void SaturAirAudioProcessorEditor::AirLookAndFeel::drawLinearSlider (
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float, float,
    juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = y + h * 0.5f;
    const float trackH = 5.0f;

    g.setColour (juce::Colour (0xffc3d2de));
    g.fillRoundedRectangle ((float) x, cy - trackH * 0.5f, (float) w, trackH, trackH * 0.5f);

    g.setColour (juce::Colour (0xff63c6de));
    g.fillRoundedRectangle ((float) x, cy - trackH * 0.5f,
                            juce::jmax (trackH, sliderPos - (float) x), trackH,
                            trackH * 0.5f);

    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.fillEllipse (sliderPos - 7.0f, cy - 7.0f + 1.5f, 14.0f, 14.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (sliderPos - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    g.setColour (juce::Colour (0xffa9bccb));
    g.drawEllipse (sliderPos - 7.0f, cy - 7.0f, 14.0f, 14.0f, 1.2f);
}

void SaturAirAudioProcessorEditor::AirLookAndFeel::drawComboBox (
    juce::Graphics& g, int width, int height, bool,
    int, int, int, int, juce::ComboBox& box)
{
    const float h = (float) height;
    juce::Rectangle<float> bounds (1.0f, 1.0f, (float) width - 2.0f, h - 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.12f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), h * 0.5f);

    g.setColour (juce::Colour (0xfff4f8fb));
    g.fillRoundedRectangle (bounds, h * 0.5f);
    g.setColour (juce::Colour (0xffc9d6e0));
    g.drawRoundedRectangle (bounds, h * 0.5f, 1.0f);

    const int selId = box.getSelectedId();
    const juce::String text = selId > 0 ? box.getItemText (selId)
                                        : box.getTextWhenNothingSelected();

    g.setColour (juce::Colour (0xff6d7f8f));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText (text, bounds.toNearestInt().reduced (10, 0),
                juce::Justification::centred, false);

    // chevron simples indicando dropdown
    g.setColour (juce::Colour (0xff9aa9b6));
    const float ax = bounds.getRight() - 14.0f, ay = bounds.getCentreY();
    juce::Path p;
    p.startNewSubPath (ax, ay - 2.5f);
    p.lineTo (ax + 4.0f, ay + 1.5f);
    p.lineTo (ax + 8.0f, ay - 2.5f);
    g.strokePath (p, juce::PathStrokeType (1.4f));
}

void SaturAirAudioProcessorEditor::AirLookAndFeel::drawButtonBackground (
    juce::Graphics& g, juce::Button& button, const juce::Colour&,
    bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (highlighted ? 0xfff7fafc : 0xffeef3f7));
    g.fillRoundedRectangle (bounds, bounds.getHeight() * 0.5f);
    g.setColour (juce::Colour (0xffc9d6e0));
    g.drawRoundedRectangle (bounds, bounds.getHeight() * 0.5f, 1.0f);
}

void SaturAirAudioProcessorEditor::AirLookAndFeel::drawToggleButton (
    juce::Graphics& g, juce::ToggleButton& button, bool, bool)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const auto centre = bounds.getCentre();
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const bool on = button.getToggleState();

    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.fillEllipse (centre.x - r, centre.y - r + 2.0f, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colour (on ? 0xfff2fafd : 0xfff4f7fa));
    g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colour (on ? 0xff63c6de : 0xffc9d6e0));
    g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, on ? 1.8f : 1.0f);

    // glifo de power: arco com gap no topo + haste vertical
    const auto col = juce::Colour (on ? 0xff2fbfd8 : 0xff9aa9b6);
    g.setColour (col);

    const float gr = r * 0.52f;
    const float from = juce::degreesToRadians (280.0f);
    const float to   = juce::degreesToRadians (620.0f);  // 340 graus -> gap no topo

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, gr, gr, 0.0f, from, to, true);
    g.strokePath (arc, juce::PathStrokeType (1.8f,
                     juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));

    g.drawLine (centre.x, centre.y - gr - 2.0f, centre.x, centre.y - gr * 0.15f, 1.8f);
}

//==============================================================================
SaturAirAudioProcessorEditor::~SaturAirAudioProcessorEditor()
{
    setLookAndFeel (nullptr);   // airLnf e membro: limpa antes da destruicao
}

SaturAirAudioProcessorEditor::SaturAirAudioProcessorEditor (SaturAirAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&airLnf);

    titleLabel.setText ("S A T U R   A I R", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (17.0f)));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8d2dc));
    addAndMakeVisible (titleLabel);

    // Knobs grandes
    setupKnob (highSlider, highLabel, "MID AIR", "high");
    setupKnob (airSlider,  airLabel,  "HIGH AIR", "air");
    highSlider.getProperties().set ("accent", (juce::int64) 0xff5ec8e0);
    airSlider.getProperties().set  ("accent", (juce::int64) 0xff7fe0b8);

    for (auto* l : { &highRangeMin, &highRangeMax, &airRangeMin, &airRangeMax })
    {
        l->setText (l == &highRangeMin || l == &airRangeMin ? "0" : "100",
                    juce::dontSendNotification);
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::Font (juce::FontOptions (10.0f)));
        l->setColour (juce::Label::textColourId, juce::Colour (0xff93a4b3));
        addAndMakeVisible (*l);
    }

    // Crossovers
    setupCrossSlider (lowMidSlider,  lowMidLabel,  "Low/Mid Xover",  "lowMidHz");
    setupCrossSlider (midHighSlider, midHighLabel, "Mid/High Xover", "midHighHz");

    // Pill = Shape
    shapeLabel.setText ("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    shapeLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    shapeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff93a4b3));
    addAndMakeVisible (shapeLabel);

    shapeBox.setTextWhenNothingSelected ("Shape");
    const char* shapeNames[] = { "Tanh", "ArcTan", "Cubic", "Rational", "Valve", "Hard" };
    for (int i = 0; i < (int) (sizeof (shapeNames) / sizeof (shapeNames[0])); ++i)
        shapeBox.addItem (shapeNames[i], i + 1);
    addAndMakeVisible (shapeBox);
    shapeAttachment = std::make_unique<ComboBoxAttachment> (processorRef.apvts, "shape", shapeBox);

    // POWER
    powerLabel.setText ("POWER", juce::dontSendNotification);
    powerLabel.setJustificationType (juce::Justification::centred);
    powerLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    powerLabel.setColour (juce::Label::textColourId, juce::Colour (0xff93a4b3));
    addAndMakeVisible (powerLabel);

    powerButton.setClickingTogglesState (true);
    addAndMakeVisible (powerButton);
    powerAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, "power", powerButton);

    // TRIM (ganho de saida)
    outputLabel.setText ("TRIM", juce::dontSendNotification);
    outputLabel.setJustificationType (juce::Justification::centred);
    outputLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    outputLabel.setColour (juce::Label::textColourId, juce::Colour (0xff93a4b3));
    addAndMakeVisible (outputLabel);

    setupKnob (outputSlider, outputLabel, "", "output");
    outputLabel.setText ("TRIM", juce::dontSendNotification);   // setupKnob troca o texto
    outputSlider.setNumDecimalPlacesToDisplay (1);
    outputSlider.getProperties().set ("accent", (juce::int64) 0xff5ec8e0);
    outputSlider.getProperties().set ("caption", "");

    startTimerHz (30);   // medidor do arco a 30 Hz

    setResizable (true, true);
    setResizeLimits (470, 360, 1100, 800);
    setSize (560, 400);
}

void SaturAirAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label,
                                              const juce::String& text,
                                              const juce::String& paramID)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::degreesToRadians (135.0f),
                                juce::degreesToRadians (405.0f), true);
    slider.getProperties().set ("caption", "");   // nome fica no label de baixo
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (10.0f)));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff8fa0ae));
    addAndMakeVisible (label);

    attachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts,
                                                               paramID, slider));
    if (paramID == "high" || paramID == "air")
        slider.setNumDecimalPlacesToDisplay (0);
}

void SaturAirAudioProcessorEditor::setupCrossSlider (juce::Slider& slider, juce::Label& label,
                                                     const juce::String& text,
                                                     const juce::String& paramID)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::Font (juce::FontOptions (10.0f)));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff93a4b3));
    addAndMakeVisible (label);

    attachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts,
                                                               paramID, slider));
}

void SaturAirAudioProcessorEditor::timerCallback()
{
    // legenda dos crossovers acompanha o valor (Hz arredondado)
    const auto lowText  = juce::String ((int) std::round (lowMidSlider.getValue()))  + " Hz";
    const auto highText = juce::String ((int) std::round (midHighSlider.getValue())) + " Hz";

    if (! lowMidLabel.getText().endsWith (lowText))
        lowMidLabel.setText ("Low/Mid Xover   " + lowText, juce::dontSendNotification);
    if (! midHighLabel.getText().endsWith (highText))
        midHighLabel.setText ("Mid/High Xover   " + highText, juce::dontSendNotification);

    repaint (arcArea.expanded (4));   // so o arco do medidor
}

//==============================================================================
void SaturAirAudioProcessorEditor::paint (juce::Graphics& g)
{
    // --- Painel superior escuro ---
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0c1014), 0.0f, 0.0f,
                                             juce::Colour (0xff05070a), 0.0f,
                                             (float) darkPanel.getBottom(), false));
    g.fillRect (darkPanel);

    // brilho esverdeado sutil nas laterais (como a referencia)
    g.setColour (juce::Colour (0x141fae7a));
    g.fillEllipse ((float) darkPanel.getX() - 60.0f,
                   (float) darkPanel.getBottom() - 90.0f, 150.0f, 120.0f);
    g.setColour (juce::Colour (0x1018d0b0));
    g.fillEllipse ((float) darkPanel.getRight() - 90.0f,
                   (float) darkPanel.getBottom() - 100.0f, 150.0f, 130.0f);

    // --- Painel inferior claro ---
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9f0f5), 0.0f,
                                             (float) lightPanel.getY(),
                                             juce::Colour (0xffd3dfe8), 0.0f,
                                             (float) lightPanel.getBottom(), false));
    g.fillRect (lightPanel);

    g.setColour (juce::Colour (0x30ffffff));
    g.drawHorizontalLine (lightPanel.getY(), (float) lightPanel.getX(),
                          (float) lightPanel.getRight());

    // --- Arco do medidor de saida ---
    if (arcArea.getWidth() > 40 && arcArea.getHeight() > 40)
    {
        // Raio limitado pela largura (42%) e pela altura (58%) disponivel;
        // centro ancorado perto da base para a "domo" caber no painel.
        const float radius = juce::jmin (arcArea.getWidth() * 0.42f,
                                         arcArea.getHeight() * 0.58f);
        const auto centre  = juce::Point<float> ((float) arcArea.getCentreX(),
                                                 (float) arcArea.getBottom() - radius * 0.62f);

        // faixa de angulos: 165 graus (esq) -> 375 graus (dir), passando pelo topo
        const float startAngle = juce::degreesToRadians (165.0f);
        const float endAngle   = juce::degreesToRadians (375.0f);

        auto angleFor = [startAngle, endAngle] (float t)
        {
            return startAngle + (endAngle - startAngle) * t;
        };

        // tracos da escala (-96..0 dB em posicoes nao lineares, como a referencia)
        struct Tick { float db; const char* label; float t; };
        const Tick ticks[] = {
            { -96.0f, "-96", 0.00f }, { -48.0f, "-48", 0.26f },
            { -24.0f, "-24", 0.50f }, { -12.0f, "-12", 0.70f },
            {  -6.0f, "-6",  0.84f }, {  -3.0f, "-3",  0.92f },
            {   0.0f, "0",   1.00f }
        };

        // dB -> posicao no arco: interpolacao linear ENTRE os marcos acima,
        // para que o marcador bate exatamente com os rotulos da escala.
        auto dbToT = [&ticks] (float db)
        {
            if (db <= ticks[0].db)
                return 0.0f;

            for (int i = 1; i < 7; ++i)
                if (db <= ticks[i].db)
                {
                    const float f = (db - ticks[i - 1].db)
                                  / (ticks[i].db - ticks[i - 1].db);
                    return ticks[i - 1].t + f * (ticks[i].t - ticks[i - 1].t);
                }
            return 1.0f;
        };

        // nivel atual (0..1) a partir do pico de saida
        const float peak = juce::jmax (processorRef.getOutputPeak (0),
                                       processorRef.getOutputPeak (1));
        const float db   = peak > 1.0e-6f
                             ? juce::Decibels::gainToDecibels (peak, -96.0f)
                             : -96.0f;
        const float level = juce::jlimit (0.0f, 1.0f, dbToT (db));   // escala da referencia

        // arcos: pintado como segmentos curtos coloridos (azul -> vermelho)
        const int numSegs = 84;
        for (int i = 0; i < numSegs; ++i)
        {
            const float t  = (float) i / (float) (numSegs - 1);
            const float a0 = angleFor (t);
            const float a1 = angleFor ((float) (i + 1) / (float) (numSegs - 1));

            juce::Path seg;
            seg.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                               a0, a1 + 0.012f, true);

            const bool lit = t <= level;
            auto col = juce::Colour (0xff3f7fb8);   // azul apagado

            if (lit)
            {
                // azul -> ciano -> amarelo -> vermelho conforme aproxima do 0 dB
                if      (t < 0.55f) col = juce::Colour (0xff4fc3e8);
                else if (t < 0.78f) col = juce::Colour (0xff7fd9a8);
                else if (t < 0.90f) col = juce::Colour (0xffe8d76a);
                else                col = juce::Colour (0xfff06a8a);
            }
            else
            {
                col = col.withAlpha (0.45f);
            }

            g.setColour (col);
            g.strokePath (seg, juce::PathStrokeType (3.0f,
                             juce::PathStrokeType::butt,
                             juce::PathStrokeType::rounded));
        }

        // rotulos da escala (inclinados como na referencia)
        g.setFont (juce::Font (juce::FontOptions (8.5f)));
        for (const auto& tk : ticks)
        {
            const float a = angleFor (tk.t);
            const float lx = centre.x + std::cos (a) * (radius - 16.0f);
            const float ly = centre.y + std::sin (a) * (radius - 16.0f);

            juce::Graphics::ScopedSaveState save (g);
            g.translate (lx, ly);

            // rotacao tangencial ao arco, normalizada para o texto nunca
            // ficar de cabeca para baixo (lado direito do arco)
            float rot = a + juce::MathConstants<float>::halfPi;
            while (rot >  juce::MathConstants<float>::pi) rot -= juce::MathConstants<float>::twoPi;
            while (rot < -juce::MathConstants<float>::pi) rot += juce::MathConstants<float>::twoPi;
            if      (rot >  juce::MathConstants<float>::halfPi) rot -= juce::MathConstants<float>::pi;
            else if (rot < -juce::MathConstants<float>::halfPi) rot += juce::MathConstants<float>::pi;

            g.rotate (rot);
            g.setColour (juce::Colour (0xff7d8b99));
            g.drawText (tk.label, -12, -6, 24, 12, juce::Justification::centred, false);
        }

        // marcador do nivel (bolha com brilho)
        {
            const float a = angleFor (level);
            const float mx = centre.x + std::cos (a) * radius;
            const float my = centre.y + std::sin (a) * radius;

            g.setColour (juce::Colour (0x5546e0ff));
            g.fillEllipse (mx - 7.0f, my - 7.0f, 14.0f, 14.0f);
            g.setColour (juce::Colour (0xff5ee7ff));
            g.fillEllipse (mx - 4.0f, my - 4.0f, 8.0f, 8.0f);
        }

        // "x" vermelho alem do 0 dB
        {
            const float a = angleFor (1.0f);
            const float xx = centre.x + std::cos (a) * (radius + 13.0f);
            const float yy = centre.y + std::sin (a) * (radius + 13.0f);
            g.setColour (juce::Colour (0xffe05570));
            g.drawLine (xx - 3.5f, yy - 3.5f, xx + 3.5f, yy + 3.5f, 1.6f);
            g.drawLine (xx + 3.5f, yy - 3.5f, xx - 3.5f, yy + 3.5f, 1.6f);
        }
    }
}

void SaturAirAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    const int darkH = (int) (area.getHeight() * 0.45f);
    darkPanel  = area.removeFromTop (darkH);
    lightPanel = area;

    // titulo no topo do painel escuro; darkPanel guarda o painel INTEIRO
    // (o paint o preenche), entao o layout do arco usa uma copia local.
    titleLabel.setBounds (darkPanel.removeFromTop (34).reduced (0, 4));
    darkPanel.removeFromTop (6);

    // area do arco: faixa central do painel escuro
    arcArea = darkPanel;
    darkPanel = getLocalBounds().removeFromTop (darkH);   // painel INTEIRO p/ pintar

    // ---------- painel claro ----------
    auto light = lightPanel.reduced (18);

    // Linha dos crossovers PRIMEIRO (largura cheia - senao ficaria espremida
    // no terco central depois das colunas dos knobs).
    {
        auto row = light.removeFromBottom (50);

        auto first = row.removeFromTop (24);
        lowMidLabel.setBounds (first.removeFromLeft (150));
        lowMidSlider.setBounds (first.reduced (0, 7));

        row.removeFromTop (2);
        auto second = row;
        midHighLabel.setBounds (second.removeFromLeft (150));
        midHighSlider.setBounds (second.reduced (0, 7));
    }

    light.removeFromBottom (6);

    const int knobSize = juce::jmin (150, light.getHeight() * 58 / 100);
    const int colW     = light.getWidth() / 3;

    // coluna esquerda: knob MID AIR + faixa 0..100
    {
        auto col = light.removeFromLeft (colW);
        auto range = col.removeFromBottom (14);
        highRangeMin.setBounds (range.removeFromLeft (range.getWidth() / 2));
        highRangeMax.setBounds (range);

        highLabel.setBounds (col.removeFromBottom (14));
        highSlider.setBounds (col.withSizeKeepingCentre (knobSize, knobSize));
    }

    // coluna direita: knob HIGH AIR + faixa 0..100
    {
        auto col = light.removeFromRight (colW);
        auto range = col.removeFromBottom (14);
        airRangeMin.setBounds (range.removeFromLeft (range.getWidth() / 2));
        airRangeMax.setBounds (range);

        airLabel.setBounds (col.removeFromBottom (14));
        airSlider.setBounds (col.withSizeKeepingCentre (knobSize, knobSize));
    }

    // coluna central: pill do Shape em cima, TRIM/POWER embaixo
    {
        auto centre = light;

        shapeLabel.setBounds (centre.removeFromTop (14));
        centre.removeFromTop (2);

        shapeBox.setBounds (centre.removeFromTop (26).withSizeKeepingCentre (96, 26));

        centre.removeFromBottom (10);

        // linha de baixo: POWER (esq) e TRIM (dir)
        auto bottom = centre.removeFromBottom (54);

        auto left = bottom.removeFromLeft (bottom.getWidth() / 2);
        powerLabel.setBounds (left.removeFromTop (13));
        powerButton.setBounds (left.removeFromTop (34).removeFromLeft (34));

        auto right = bottom;
        outputLabel.setBounds (right.removeFromTop (13));
        outputSlider.setBounds (right.removeFromTop (40)
                                    .removeFromRight (40)
                                    .withSizeKeepingCentre (40, 40));
    }
}
