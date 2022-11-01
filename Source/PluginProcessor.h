/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "params.h"
#include "config.h"
#include "audio/CombProcessor.h"
#include "synth/SynthVoice.h"
#include "synth/SynthSound.h"

using namespace audio;

//==============================================================================
/**
*/
class FineToothMIDIAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    FineToothMIDIAudioProcessor();
    ~FineToothMIDIAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void panic();
    void setInputMode (int state);
    
    APVTS apvts;
    
//    std::array<ADSR, NUM_VOICES> adsr;
//    std::array<std::unique_ptr<CombProcessor>, NUM_VOICES> processor;

private:
//    void updateAll ();
//    void updateVoice (int voice);
//    void updateFilter ();
    void setVoiceParams ();
    Random random;
    
    Synthesiser synth;
    
    AudioBuffer<float> noiseBuffer;
    
    /*
    std::array<AudioBuffer<float>, NUM_VOICES> voiceBuffers;
    std::array<int, NUM_VOICES> currentNoteOn, currentVelocity, lastNoteOn;
    
    AudioBuffer<float> outBuffer;
    */
    
    int inputMode; //, numActiveVoices;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FineToothMIDIAudioProcessor)
};
