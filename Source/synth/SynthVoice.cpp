/*
  ==============================================================================

    SynthVoice.cpp
    Created: 31 Oct 2022 12:35:13pm
    Author:  Kevin Kopczynski

  ==============================================================================
*/

#include "SynthVoice.h"


bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition)
{
    comb.setFrequency(audio::midiToFreq(midiNoteNumber));
    
//    adsr.reset();
    adsr.noteOn();
}

void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
    adsr.noteOff();
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    ;
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    ;
}

void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
{
    // initialize adsr
    adsr.setSampleRate(sampleRate);
    
    // initialize comb processor
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = 1;
    
    comb.prepare(spec);
    
    // initialize comb buffer
    combBuffer.setSize(outputChannels, samplesPerBlock);
    
    isPrepared = true;
}

void SynthVoice::reset()
{
//    comb.reset();
    adsr.reset();
}

void SynthVoice::renderNextBlock(AudioBuffer<float> &outputBuffer, int startSample, int numSamples)
{
    jassert(isPrepared);
    
    if (! isVoiceActive())
    {
        return;
    }
    
    auto buffer = combBuffer.getArrayOfWritePointers();
    
    comb.process(combBuffer, numSamples);
    
    adsr.applyEnvelopeToBuffer(combBuffer, 0, numSamples);
//    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
//        for (int s = 0; s < numSamples; ++s)
//            buffer[ch][s] *= adsr.getNextSample();
    
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom(ch, 0, buffer[ch], numSamples);
    
    if (! adsr.isActive())
        clearCurrentNote();
}

void SynthVoice::fillBuffer(AudioBuffer<float> &buffer, int numSamples)
{
    auto inBuffer = buffer.getArrayOfReadPointers();
    auto outBuffer = combBuffer.getArrayOfWritePointers();
    
    for (int ch = 0; ch < 2; ++ch)
        audio::SIMD::copy(outBuffer[ch], inBuffer[ch], numSamples);
}
