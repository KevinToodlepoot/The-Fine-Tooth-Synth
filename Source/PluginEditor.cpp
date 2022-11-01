/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FineToothMIDIAudioProcessorEditor::FineToothMIDIAudioProcessorEditor (FineToothMIDIAudioProcessor& p)
    : AudioProcessorEditor (&p),
    filterDisplay(p),
    adsrDisplay(p),
    aliasModeButton(StringArray("Ignore", "Wrap", "Fold"), p),
    attackAttachment(p.apvts, "Attack", attack),
    decayAttachment(p.apvts, "Decay", decay),
    sustainAttachment(p.apvts, "Sustain", sustain),
    releaseAttachment(p.apvts, "Release", release),
    resonanceAttachment(p.apvts, "Resonance", resonance),
    timbreAttachment(p.apvts, "Timbre", timbre),
    curveAttachment(p.apvts, "Curve", curve),
    spreadAttachment(p.apvts, "Spread", spread),
    glideAttachment(p.apvts, "Glide", glide),
    inputModeAttachment(p.apvts, "Input Mode", sourceButtons[1]),
    clear("Clear", Colours::red, Colours::darkred, Colours::white),
    audioProcessor (p)
{
    startTimerHz(60);
    
    for ( auto* comp : getComps() )
        addAndMakeVisible(comp);
    
    curve.setSkewFactor(2.0f);
    
    Path circle;
    circle.addEllipse(0, 0, 20, 20);
    clear.setShape(circle, true, true, false);
    clear.onClick = [&] (void)
    {
        audioProcessor.panic();
    };
    
    for (auto& button : sourceButtons)
    {
        button.setRadioGroupId (293847);
        button.setClickingTogglesState (true);
    }

    sourceButtons[0].setButtonText ("Noise");
    sourceButtons[1].setButtonText ("Ext");

    sourceButtons[0].setConnectedEdges (Button::ConnectedOnRight);
    sourceButtons[1].setConnectedEdges (Button::ConnectedOnLeft);
    
    sourceButtons[0].setLookAndFeel(&customLNF);
    sourceButtons[1].setLookAndFeel(&customLNF);

    // Set the initial value.
    auto settings = getChainSettings(audioProcessor.apvts);
    sourceButtons[0].setToggleState (! settings.inputMode, dontSendNotification);

    sourceButtons[0].onClick = [this] { inputButtonClicked(0); };
    sourceButtons[1].onClick = [this] { inputButtonClicked(1); };
    
    setSize (800, 600);
}

FineToothMIDIAudioProcessorEditor::~FineToothMIDIAudioProcessorEditor()
{
    sourceButtons[0].setLookAndFeel(nullptr);
    sourceButtons[1].setLookAndFeel(nullptr);
}

//==============================================================================
void FineToothMIDIAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(3, 102, 102));
    
    g.setColour(Colour(86, 171, 145));

    auto bounds = getLocalBounds().reduced(20);
    auto controlBounds = bounds.removeFromTop(bounds.getHeight() * 0.1);
    auto mainBounds = bounds.removeFromTop(bounds.getHeight() * 0.66);
    mainBounds.removeFromTop(mainBounds.getHeight() * 0.66); // spectrumBounds
    auto mainLabelBounds = mainBounds.removeFromTop(mainBounds.getHeight() * 0.2);

    controlBounds.removeFromLeft(controlBounds.getWidth() * 0.33);
    
    g.setFont(36.0f);
    g.drawText("The Fine Tooth", controlBounds.removeFromLeft(controlBounds.getWidth() * 0.5), Justification::centredTop);
    
    g.setFont(18.0f);
    g.drawText("Resonance", mainLabelBounds.removeFromLeft(mainLabelBounds.getWidth() * 0.25f), Justification::centredBottom);
    g.drawText("Timbre", mainLabelBounds.removeFromLeft(mainLabelBounds.getWidth() * 0.33f), Justification::centredBottom);
    g.drawText("Curve", mainLabelBounds.removeFromLeft(mainLabelBounds.getWidth() * 0.5f), Justification::centredBottom);
    g.drawText("Spread", mainLabelBounds, Justification::centredBottom);

//    g.fillRoundedRectangle(spectrumBounds.reduced(10).toFloat(), 10.0f);
    
    auto adsrBounds = bounds.removeFromLeft(bounds.getWidth() * 0.5f);
    adsrBounds.removeFromTop(adsrBounds.getHeight() * 0.66f); // displayBounds
    auto adsrLabelBounds = adsrBounds.removeFromTop(adsrBounds.getHeight() * 0.2);
    
    g.setFont(14.0f);
    g.drawText("Attack", adsrLabelBounds.removeFromLeft(adsrLabelBounds.getWidth() * 0.25f), Justification::centredBottom);
    g.drawText("Decay", adsrLabelBounds.removeFromLeft(adsrLabelBounds.getWidth() * 0.33f), Justification::centredBottom);
    g.drawText("Sustain", adsrLabelBounds.removeFromLeft(adsrLabelBounds.getWidth() * 0.5f), Justification::centredBottom);
    g.drawText("Release", adsrLabelBounds, Justification::centredBottom);
    
    bounds.removeFromBottom(bounds.getHeight() * 0.1f);
    auto glideAndInputBounds = bounds.removeFromTop(bounds.getHeight() * 0.66f);
    auto glideAndInputLabelBounds = glideAndInputBounds.removeFromTop(glideAndInputBounds.getHeight() * 0.33f);
    
    g.setFont(18.0f);
    g.drawText("Input Source", glideAndInputLabelBounds.removeFromRight(glideAndInputLabelBounds.getWidth() * 0.5f), Justification::centredBottom);
    g.drawText("Glide", glideAndInputLabelBounds, Justification::centredBottom);
    
    auto aliasModeLabelBounds = bounds.removeFromTop(bounds.getHeight() * 0.33f);
    g.drawText("Alias Mode", aliasModeLabelBounds, Justification::centred);
    
}

void FineToothMIDIAudioProcessorEditor::resized()
{
    clear.setBounds(getWidth() - 15, 5, 10, 10);
    
    auto bounds = getLocalBounds().reduced(20);
    auto controlBounds = bounds.removeFromTop(bounds.getHeight() * 0.1);
    auto mainBounds = bounds.removeFromTop(bounds.getHeight() * 0.66);
    auto spectrumBounds = mainBounds.removeFromTop(mainBounds.getHeight() * 0.66);
    mainBounds.removeFromTop(mainBounds.getHeight() * 0.2); // mainLabelBounds
    
    filterDisplay.setBounds(spectrumBounds);
    
    resonance.setBounds(mainBounds.removeFromLeft(mainBounds.getWidth() * 0.25f));
    timbre.setBounds(mainBounds.removeFromLeft(mainBounds.getWidth() * 0.33f));
    curve.setBounds(mainBounds.removeFromLeft(mainBounds.getWidth() * 0.5));
    spread.setBounds(mainBounds);
    
    auto adsrBounds = bounds.removeFromLeft(bounds.getWidth() * 0.5f);
    auto displayBounds = adsrBounds.removeFromTop(adsrBounds.getHeight() * 0.66f);
    adsrBounds.removeFromTop(adsrBounds.getHeight() * 0.2); // adsrLabelBounds
    
    adsrDisplay.setBounds(displayBounds.reduced(10));
    
    attack.setBounds(adsrBounds.removeFromLeft(adsrBounds.getWidth() * 0.25f));
    decay.setBounds(adsrBounds.removeFromLeft(adsrBounds.getWidth() * 0.33f));
    sustain.setBounds(adsrBounds.removeFromLeft(adsrBounds.getWidth() * 0.5f));
    release.setBounds(adsrBounds);
    
    bounds.removeFromBottom(bounds.getHeight() * 0.1f);
    
    auto glideAndInputBounds = bounds.removeFromTop(bounds.getHeight() * 0.66f);
    glideAndInputBounds.removeFromTop(glideAndInputBounds.getHeight() * 0.33f); // glideAndInputLabelBounds
    glide.setBounds(glideAndInputBounds.removeFromLeft(glideAndInputBounds.getWidth() * 0.5f));
    
    glideAndInputBounds.reduce(10.0f, 20.0f);
    sourceButtons[0].setBounds(glideAndInputBounds.removeFromLeft(glideAndInputBounds.getWidth() * 0.5f));
    sourceButtons[1].setBounds(glideAndInputBounds);
    
    bounds.removeFromTop(bounds.getHeight() * 0.33f); // aliasModeLabelBounds
    aliasModeButton.setBounds(bounds.reduced(10.0f, 5.0f));
}

void FineToothMIDIAudioProcessorEditor::timerCallback()
{
    filterDisplay.updateAll();
    adsrDisplay.updateAll();
}

//==============================================================================
void FineToothMIDIAudioProcessorEditor::inputButtonClicked (int button)
{
    auto buttonState = sourceButtons[button].getToggleState();
    
    sourceButtons[1 - button].setToggleState(! buttonState, dontSendNotification);
    
//    if (button && buttonState)
//        audioProcessor.setInputMode(1);
//    else if ( (!button) && buttonState)
//        audioProcessor.setInputMode(0);
}

std::vector<Component*> FineToothMIDIAudioProcessorEditor::getComps()
{
    return
    {
        &attack,
        &decay,
        &sustain,
        &release,
        &resonance,
        &timbre,
        &curve,
        &spread,
        &glide,
        &clear,
        &filterDisplay,
        &adsrDisplay,
        &sourceButtons[0],
        &sourceButtons[1],
        &aliasModeButton
    };
}
