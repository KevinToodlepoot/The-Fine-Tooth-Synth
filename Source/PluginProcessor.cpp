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
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#if PPDHasSidechain
                       .withInput ("Sidechain", AudioChannelSet::stereo(), false)
#endif
                     #endif
                       ), apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    synth.addSound(new SynthSound());
    
    for (int i = 0; i < NUM_VOICES; ++i)
        synth.addVoice(new SynthVoice());
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
    return false;
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
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        
    noiseBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    
    /*
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
     */
}

void FineToothMIDIAudioProcessor::releaseResources()
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->reset();
        }
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FineToothMIDIAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = AudioChannelSet::mono();
    const auto stereo = AudioChannelSet::stereo();

    const auto mainSetIn = layouts.getMainInputChannelSet();
    const auto mainSetOut = layouts.getMainOutputChannelSet();

#if PPDHasSidechain
    const auto scSetIn = layouts.getChannelSet(true, 1);
    if(!scSetIn.isDisabled())
        if (scSetIn != mono && scSetIn != stereo)
            return false;
#endif
    if (mainSetOut != stereo)
        return false;

    return true;
}
#endif

void FineToothMIDIAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    setVoiceParams();
    
    if (! inputMode)
    {
        noiseBuffer.clear();
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
            for (int s = 0; s < numSamples; ++s)
                noiseBuffer.setSample(ch, s, random.nextFloat() * 0.5f);
    }
    else
    {
        noiseBuffer.clear();
        auto noiseBufferPtr = noiseBuffer.getArrayOfWritePointers();
        auto bufferPtr = buffer.getArrayOfReadPointers();
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
        {
            SIMD::copy(noiseBufferPtr[ch], bufferPtr[ch], numSamples);
        }
    }
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->fillBuffer(noiseBuffer, numSamples);
        }
    }
    
    buffer.clear();
    
    synth.renderNextBlock(buffer, midiMessages, 0, numSamples);
}

void FineToothMIDIAudioProcessor::setVoiceParams()
{
    auto settings = getChainSettings(apvts);
    
    inputMode = settings.inputMode;
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            auto& comb = voice->getCombProcessor();
            auto& adsr = voice->getADSR();
            
            CombProcessor::FreqOutOfBoundsMode mode;
            switch (settings.aliasMode)
            {
                case 0:
                    mode = CombProcessor::FreqOutOfBoundsMode::Ignore;
                    break;
                    
                case 1:
                    mode = CombProcessor::FreqOutOfBoundsMode::Wrap;
                    break;
                    
                case 2:
                    mode = CombProcessor::FreqOutOfBoundsMode::Fold;
                    break;
                    
                default:
                    mode = CombProcessor::FreqOutOfBoundsMode::Ignore;
                    break;
            }
            
            comb.updateParams(CombProcessor::Parameters(-1.0f, settings.resonance, settings.timbre, settings.curve, settings.spread, settings.glide, mode));
            
            if (! adsr.isActive())
                adsr.setParameters(ADSR::Parameters(settings.attack / 1000.0f, settings.decay / 1000.0f, settings.sustain, settings.release / 1000.0f));
        }
    }
}

void FineToothMIDIAudioProcessor::panic()
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            auto& adsr = voice->getADSR();
            
            adsr.reset();
        }
    }
}

void FineToothMIDIAudioProcessor::setInputMode(int state)
{
    apvts.getParameter("Input Mode")->setValue(state);
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
    juce::MemoryOutputStream mos(destData, true);
            apvts.state.writeToStream(mos);
}

void FineToothMIDIAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
        if ( tree.isValid() )
        {
            apvts.replaceState(tree);
            setVoiceParams();
        }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FineToothMIDIAudioProcessor();
}
