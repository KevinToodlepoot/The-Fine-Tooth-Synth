/*
  ==============================================================================

    ADSRDisplay.h
    Created: 29 Oct 2022 9:05:57pm
    Author:  Kevin Kopczynski

  ==============================================================================
*/

#ifndef ADSRDISPLAY_H
#define ADSRDISPLAY_H

#include <JuceHeader.h>

//==============================================================================
/*
*/
class ADSRDisplay  : public juce::Component
{
public:
    ADSRDisplay(FineToothMIDIAudioProcessor& p) : audioProcessor(p)
    {
        attack = ATTACK_DEFAULT;
        decay = DECAY_DEFAULT;
        sustain = SUSTAIN_DEFAULT;
        release = RELEASE_DEFAULT;
    }

    ~ADSRDisplay() override
    {
    }

    void paint (juce::Graphics& g) override
    {
        // 2 second display
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(Colour(86, 171, 145));
        g.fillRoundedRectangle(bounds, 10.0f);
        
        // draw grid lines
        Array<float> ms
        {
            0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500
        };
        g.setColour(Colour(103, 185, 154));
        for (auto s : ms)
        {
            auto x = jmap(s, 50.0f, 500.0f, 8.0f, getWidth() - 8.0f);
            g.drawVerticalLine(x, 0, getHeight());
        }
        
        // draw adsr lines
        g.setColour(Colour(36, 130, 119));
        
        float attackWidth = (attack / maxMs) * getWidth();
        float decayWidth = (decay / maxMs) * getWidth();
        float sustainHeight = (getHeight()) - sustain * (getHeight() - 10.0f);
        float releaseWidth = (release / maxMs) * getWidth();
        float sustainWidth = (50 / maxMs) * getWidth();
        
        Point<float> attackStart(8.0f, getHeight());
        Point<float> attackEnd(attackStart.getX() + attackWidth, 10.0f);
        Point<float> decayEnd(attackEnd.getX() + decayWidth, sustainHeight);
        Point<float> releaseStart(decayEnd.getX() + sustainWidth, sustainHeight);
        Point<float> releaseEnd(releaseStart.getX() + releaseWidth, getHeight());
        
        Line<float> attackLine(attackStart, attackEnd);
        Line<float> decayLine(attackEnd, decayEnd);
        Line<float> sustainLine(decayEnd, releaseStart);
        Line<float> releaseLine(releaseStart, releaseEnd);
        
        g.drawLine(attackLine, 2.0f);
        g.drawLine(decayLine, 2.0f);
        g.drawLine(sustainLine, 2.0f);
        g.drawLine(releaseLine, 2.0f);
        
        g.drawRoundedRectangle(bounds.reduced(2.0f), 8.0f, 4.0f);
    }

    void resized() override
    {
    }
    
    void updateAll()
    {
        auto settings = getChainSettings(audioProcessor.apvts);
        
        attack = settings.attack;
        decay = settings.decay;
        sustain = settings.sustain;
        release = settings.release;
        
        repaint();
    }

private:
    float attack, decay, sustain, release;
    FineToothMIDIAudioProcessor& audioProcessor;
    
    static constexpr float maxMs = 550.0f;//ATTACK_MAX + DECAY_MAX + RELEASE_MAX + 100.0f - 16.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSRDisplay)
};

#endif // ADSRDISPLAY_H
