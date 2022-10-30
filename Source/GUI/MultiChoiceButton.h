#ifndef MULTICHOICEBUTTON_H
#define MULTICHOICEBUTTON_H

#include <JuceHeader.h>
#include "FineToothLNF.h"

//==============================================================================
/*
*/
class MultiChoiceButton  : public Component
{
public:
    MultiChoiceButton(StringArray buttonTexts, FineToothMIDIAudioProcessor& p) : audioProcessor(p)
    {
        for (auto& button : buttons)
        {
            button.setRadioGroupId (293847);
            button.setClickingTogglesState (true);
        }
        
        auto settings = getChainSettings(audioProcessor.apvts);
        
        buttons[0].setConnectedEdges (Button::ConnectedOnRight);
        buttons[1].setConnectedEdges (3);
        buttons[2].setConnectedEdges (Button::ConnectedOnLeft);
        
        for (int i = 0; i < 3; ++i)
        {
            buttons[i].setButtonText (buttonTexts[i]);
            buttons[i].setToggleState (settings.aliasMode == i, dontSendNotification);
            buttons[i].setLookAndFeel (&lookAndFeel);
        }

        buttons[0].onStateChange = [this] { buttonChanged(0); };
        buttons[1].onStateChange = [this] { buttonChanged(1); };
        buttons[2].onStateChange = [this] { buttonChanged(2); };

        for (auto& button : buttons)
            addAndMakeVisible (button);
    }

    ~MultiChoiceButton() override
    {
        for (int i = 0; i < 3; ++i)
            buttons[i].setLookAndFeel(nullptr);
    }

    void paint (juce::Graphics& g) override
    {
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        buttons[0].setBounds(bounds.removeFromLeft(bounds.getWidth() * 0.33f));
        buttons[1].setBounds(bounds.removeFromLeft(bounds.getWidth() * 0.5f));
        buttons[2].setBounds(bounds);
    }
    
    void buttonChanged(int i)
    {
        auto buttonState = buttons[i].getToggleState();
        
        if (buttonState)
        {
            auto settings = getChainSettings(audioProcessor.apvts);
            switch (i)
            {
                case 0:
                    buttons[1].setToggleState(! buttonState, dontSendNotification);
                    buttons[2].setToggleState(! buttonState, dontSendNotification);
                    break;
                    
                case 1:
                    buttons[0].setToggleState(! buttonState, dontSendNotification);
                    buttons[2].setToggleState(! buttonState, dontSendNotification);
                    break;
                    
                case 2:
                    buttons[0].setToggleState(! buttonState, dontSendNotification);
                    buttons[1].setToggleState(! buttonState, dontSendNotification);
                    break;
                    
                default:
                    break;
            }
            
            settings.aliasMode = i;
        }
    }

private:
    TextButton buttons[3];
    
    FineToothLNF lookAndFeel;
    
    FineToothMIDIAudioProcessor& audioProcessor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiChoiceButton)
};

#endif // MULTICHOICEBUTTON_H
