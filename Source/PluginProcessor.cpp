/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FineToothMIDIAudioProcessor::FineToothMIDIAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
//                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
//                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

FineToothMIDIAudioProcessor::~FineToothMIDIAudioProcessor()
{
}

//==============================================================================
const juce::String FineToothMIDIAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FineToothMIDIAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FineToothMIDIAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FineToothMIDIAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FineToothMIDIAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FineToothMIDIAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FineToothMIDIAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FineToothMIDIAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FineToothMIDIAudioProcessor::getProgramName (int index)
{
    return {};
}

void FineToothMIDIAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FineToothMIDIAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    
    // initialize comb processor
    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    
    for (int voice = 0; voice < NUM_VOICES; ++voice)
    {
        // construct comb processor
        processor[voice] = std::make_unique<CombProcessor>(MAX_NUM_FILTERS, getTotalNumOutputChannels());
        processor[voice]->prepare(spec);
        
        // initialize adsr
        adsr[voice].setSampleRate(sampleRate);
        
        // initialize buffers
        voiceBuffers[voice].setSize(getTotalNumOutputChannels(), samplesPerBlock);
        outBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
        
        // initialize notes and velocities
        currentNoteOn[voice] = A440;
        currentVelocity[voice] = 0;
    }
    
    updateAll();
}

void FineToothMIDIAudioProcessor::releaseResources()
{
    for (int voice = 0; voice < NUM_VOICES; ++voice)
    {
        adsr[voice].reset();
        if (processor[voice])
            processor[voice]->reset();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FineToothMIDIAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FineToothMIDIAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    int numSamples = buffer.getNumSamples();
    float gainVal;
    numActiveVoices = 0;
    outBuffer.clear();
    updateFilter();
    
    // read midi data
    for (const auto message : midiMessages)
    {
        auto currentMessage = message.getMessage();
        // if message is note on, start adsr and tune filter
        if (currentMessage.isNoteOn())
        {
            for (int voice = 0; voice < NUM_VOICES; ++voice)
            {
                if (! adsr[voice].isActive())
                {
//                    DBG(currentMessage.getDescription());
                    adsr[voice].reset();
                    currentNoteOn[voice] = currentMessage.getNoteNumber();
                    currentVelocity[voice] = currentMessage.getVelocity();
                    updateVoice(voice);
                    adsr[voice].noteOn();
                    break;
                }
            }
        }
        
        if (currentMessage.isNoteOff())
        {
            int note = currentMessage.getNoteNumber();
            
            for (int voice = 0; voice < NUM_VOICES; ++voice)
            {
                if (currentNoteOn[voice] == note)
                {
//                    DBG(currentMessage.getDescription());
                    adsr[voice].noteOff();
                    break;
                }
            }
        }
    }
        
    if (! inputMode)
    {
        // fill buffer with noise
        buffer.clear();
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
            for (int s = 0; s < numSamples; ++s)
                buffer.setSample(ch, s, random.nextFloat());
    }
    
    
    // copy contents of input buffer to voice buffers
    float** bufferPtr = buffer.getArrayOfWritePointers();
    float** outBufferPtr = outBuffer.getArrayOfWritePointers();
    for (int voice = 0; voice < NUM_VOICES; ++voice)
    {
        float** voicePtr = voiceBuffers[voice].getArrayOfWritePointers();
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
        {
            SIMD::copy(voicePtr[ch], bufferPtr[ch], numSamples);
        }
        
        // process voice buffer
        if (adsr[voice].isActive())
        {
            processor[voice]->process(voiceBuffers[voice], numSamples);
            numActiveVoices++;
        }
        
        // apply adsr
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
        {
            gainVal = adsr[voice].getNextSample();
            
            for (int s = 0; s < numSamples; ++s)
                voicePtr[ch][s] *= gainVal;
        }
        
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
            SIMD::add(outBufferPtr[ch], voicePtr[ch], numSamples);
    }
    
//    outBuffer.applyGain(1.0f / float(numActiveVoices));
    
    for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
        SIMD::copy(bufferPtr[ch], outBufferPtr[ch], numSamples);
}

void FineToothMIDIAudioProcessor::updateAll()
{
    auto settings = getChainSettings(apvts);
    
    CombProcessor::FreqOutOfBoundsMode aliasMode;
    switch (settings.aliasMode)
    {
        case 0:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Ignore;
            break;
            
        case 1:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Wrap;
            break;
            
        case 2:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Fold;
            break;
            
        default:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Ignore;
            DBG("alias mode is being funky");
            break;
    }
    
    inputMode = settings.inputMode;
    
    float attack, decay, sustain, release, freq;
    attack = settings.attack / 1000.0f;
    decay = settings.decay / 1000.0f;
    sustain = settings.sustain;
    release = settings.release / 1000.0f;
    
    for (int voice = 0; voice < NUM_VOICES; ++voice)
    {
        if (! adsr[voice].isActive())
            adsr[voice].setParameters(ADSR::Parameters(attack, decay, sustain, release));
        
        freq = midiToFreq(currentNoteOn[voice]);
            
        processor[voice]->updateParams(CombProcessor::Parameters(freq, settings.resonance, settings.timbre, settings.curve, settings.spread, settings.glide, aliasMode));
    }
}

void FineToothMIDIAudioProcessor::updateVoice(int voice)
{
    auto settings = getChainSettings(apvts);
    
    CombProcessor::FreqOutOfBoundsMode aliasMode;
    switch (settings.aliasMode)
    {
        case 0:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Ignore;
            break;
            
        case 1:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Wrap;
            break;
            
        case 2:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Fold;
            break;
            
        default:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Ignore;
            DBG("alias mode is being funky");
            break;
    }
    
    inputMode = settings.inputMode;
    
    if (! adsr[voice].isActive())
    {
        float attack, decay, sustain, release;
        attack = settings.attack / 1000.0f;
        decay = settings.decay / 1000.0f;
        sustain = settings.sustain;
        release = settings.release / 1000.0f;
        
        adsr[voice].setParameters(ADSR::Parameters(attack, decay, sustain, release));
    }
    
    float freq;
    freq = midiToFreq(currentNoteOn[voice]);
        
    processor[voice]->updateParams(CombProcessor::Parameters(freq, settings.resonance, settings.timbre, settings.curve, settings.spread, settings.glide, aliasMode));
}

void FineToothMIDIAudioProcessor::updateFilter()
{
    auto settings = getChainSettings(apvts);
    float freq;
    
    CombProcessor::FreqOutOfBoundsMode aliasMode;
    switch (settings.aliasMode)
    {
        case 0:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Ignore;
            break;
            
        case 1:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Wrap;
            break;
            
        case 2:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Fold;
            break;
            
        default:
            aliasMode = CombProcessor::FreqOutOfBoundsMode::Ignore;
            DBG("alias mode is being funky");
            break;
    }
    
    
    for (int voice = 0; voice < NUM_VOICES; ++voice)
    {
        freq = midiToFreq(currentNoteOn[voice]);
        processor[voice]->updateParams(CombProcessor::Parameters(freq, settings.resonance, settings.timbre, settings.curve, settings.spread, settings.glide, aliasMode));
    }
}

//==============================================================================
bool FineToothMIDIAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FineToothMIDIAudioProcessor::createEditor()
{
    return new FineToothMIDIAudioProcessorEditor (*this);
//    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
void FineToothMIDIAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FineToothMIDIAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FineToothMIDIAudioProcessor();
}
