#ifndef FINETOOTHLNF_H
#define FINETOOTHLNF_H

#include <JuceHeader.h>

struct FineToothLNF : LookAndFeel_V4
{    
    void drawRotarySlider(Graphics &g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, Slider &slider) override
    {
        auto outline = Colour(53, 143, 128);
        auto fill    = Colour(120, 198, 163);

        auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (10);

        auto radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + (sliderPos + 0.00033333) * (rotaryEndAngle - rotaryStartAngle);
        toAngle = jlimit (0.0f, rotaryStartAngle * (rotaryEndAngle - rotaryStartAngle), float(toAngle));
        auto lineW = jmin (10.0f, radius * 0.25f);
        auto arcRadius = radius - lineW * 0.5f;

        Path backgroundArc;
        backgroundArc.addCentredArc (bounds.getCentreX(),
                                     bounds.getCentreY(),
                                     arcRadius,
                                     arcRadius,
                                     0.0f,
                                     rotaryStartAngle,
                                     rotaryEndAngle,
                                     true);

        g.setColour (outline);
        g.strokePath (backgroundArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            Path valueArc;
            valueArc.addCentredArc (bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    toAngle,
                                    true);

            g.setColour (fill);
            g.strokePath (valueArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
        }
        
        auto startPoint = bounds.getCentre();
        
        Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - MathConstants<float>::halfPi),
                                 bounds.getCentreY() + arcRadius * std::sin (toAngle - MathConstants<float>::halfPi));
        
        Line<float> thumbLine (startPoint, thumbPoint);
        
        
        
        g.setColour (fill);
        g.drawLine (thumbLine, lineW);
        g.fillEllipse (startPoint.getX() - lineW / 2.0f, startPoint.getY() - lineW / 2.0f, lineW, lineW);
    }
    
    void drawButtonBackground(Graphics &g, Button &button, const Colour &backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto cornerSize = 6.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        auto defaultColour = Colour(70, 157, 137);
        auto downColour = Colour(36, 130, 119);
        auto hoverDownColour = downColour;
        auto hoverColour = Colour(86, 171, 145);
        auto buttonState = button.getToggleState();
        
        Colour baseColour;
        
        if (buttonState)
            baseColour = downColour;
        else
            baseColour = defaultColour;

        if (buttonState && shouldDrawButtonAsHighlighted)
            baseColour = hoverDownColour;
        else if (buttonState)
            baseColour = downColour;
        else if (shouldDrawButtonAsHighlighted)
            baseColour = hoverColour;

        g.setColour (baseColour);

        auto flatOnLeft   = button.isConnectedOnLeft();
        auto flatOnRight  = button.isConnectedOnRight();
        auto flatOnTop    = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
        {
            Path path;
            path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                      bounds.getWidth(), bounds.getHeight(),
                                      cornerSize, cornerSize,
                                      ! (flatOnLeft  || flatOnTop),
                                      ! (flatOnRight || flatOnTop),
                                      ! (flatOnLeft  || flatOnBottom),
                                      ! (flatOnRight || flatOnBottom));

            g.fillPath (path);

            g.setColour (Colour(36, 130, 119));
            g.strokePath (path, PathStrokeType (1.0f));
        }
        else
        {
            g.fillRoundedRectangle (bounds, cornerSize);

            g.setColour (Colour(36, 130, 119));
            g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
        }
    }
    
    void drawButtonText (Graphics& g, TextButton& button,
                        bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        Font font (getTextButtonFont (button, button.getHeight()));
        g.setFont (font);
        g.setColour (Colour(153, 226, 180));

        const int yIndent = jmin (4, button.proportionOfHeight (0.3f));
        const int cornerSize = jmin (button.getHeight(), button.getWidth()) / 2;

        const int fontHeight = roundToInt (font.getHeight() * 0.6f);
        const int leftIndent  = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
        const int rightIndent = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        const int textWidth = button.getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText (button.getButtonText(),
                              leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
                              Justification::centred, 2);
    }
    
};

#endif // FINETOOTHLNF_H
