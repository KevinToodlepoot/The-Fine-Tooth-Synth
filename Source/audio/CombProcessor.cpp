/*
  ==============================================================================

    CombProcessor.cpp
    Created: 7 Oct 2022 2:54:57pm
    Author:  Kevin Kopczynski

  ==============================================================================
*/

#include "CombProcessor.h"

namespace audio
{

CombProcessor::CombProcessor(unsigned int _maxNumFilters) :
    curParams(A440, RESONANCE_DEFAULT, TIMBRE_DEFAULT, CURVE_DEFAULT, SPREAD_DEFAULT, GLIDE_DEFAULT),
    maxNumFilters(_maxNumFilters),
    numFilters(_maxNumFilters)
{
    freq.setCurrentAndTargetValue(A440);
    q.setCurrentAndTargetValue(RESONANCE_DEFAULT);
    timbre.setCurrentAndTargetValue(TIMBRE_DEFAULT);
    curve.setCurrentAndTargetValue(CURVE_DEFAULT);
    spread.setCurrentAndTargetValue(SPREAD_DEFAULT);
}

void CombProcessor::prepare(const dsp::ProcessSpec& spec)
{
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < maxNumFilters; ++i)
        {
            chain[ch][i].prepare(spec);
            chain[ch][i].get<ChainPositions::Filter>().setType(dsp::StateVariableTPTFilterType::bandpass);
        }
    }
    
    sampleRate = spec.sampleRate;
    tempBuffer.setSize(numChannels, spec.maximumBlockSize);
    outBuffer.setSize(numChannels, spec.maximumBlockSize);
    
    freq.reset(sampleRate, glide);
    q.reset(sampleRate, SMOOTH_SEC);
    timbre.reset(sampleRate, SMOOTH_SEC);
    curve.reset(sampleRate, SMOOTH_SEC);
    spread.reset(sampleRate, SMOOTH_SEC);
}

void CombProcessor::reset()
{
    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < maxNumFilters; ++i)
            chain[ch][i].reset();
    
}

void CombProcessor::process(AudioBuffer<float> &buffer, int numSamples, int startSample)
{
    auto inWrite = buffer.getArrayOfWritePointers();
    auto tempWrite = tempBuffer.getArrayOfWritePointers();
    auto outWrite = outBuffer.getArrayOfWritePointers();
    
    float curFreq = freq.getNextValue();
    float curQ = q.getNextValue();
    float curTimbre = timbre.getNextValue();
    float curCurve = curve.getNextValue();
    float curSpread = spread.getNextValue();
    
    DBG(curFreq);
    
    updateParamsObject(curFreq, curQ, curTimbre, curCurve, curSpread);
    
    // clear output buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        SIMD::fill(outWrite[ch], 0.0f, numSamples);
    }
    
    // process bandpass filter for each harmonic
    for (int i = 0; i < numFilters; ++i)
    {
        // update processors
        if (! updateFilterSettings(curFreq, curQ, curSpread, i))
        {
            break;
        }
        
        updateTimbre(curTimbre, i);
        updateCurve(curCurve, curQ, i);
            
        // copy contents of input buffer to temp buffer
        tempBuffer.clear();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto inWritePtr = buffer.getReadPointer(ch, startSample);
            SIMD::copy(tempWrite[ch], inWritePtr, numSamples);
        }
        
        // process the temp buffer and add it to output buffer
        dsp::AudioBlock<float> block(tempBuffer);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto channelBlock = block.getSingleChannelBlock(ch);
            dsp::ProcessContextReplacing<float> context(channelBlock);
            chain[ch][i].process(context);
            
            // add to output buffer
            SIMD::add(outWrite[ch], tempWrite[ch], numSamples);
        }
    }
     
    // write contents of output buffer to input buffer
    for (int ch = 0; ch < numChannels; ++ch)
        SIMD::copy(inWrite[ch], outWrite[ch], numSamples);
    
    // skip remaining samples for params
    freq.skip(numSamples - 1);
    q.skip(numSamples - 1);
    timbre.skip(numSamples - 1);
    curve.skip(numSamples - 1);
    spread.skip(numSamples - 1);
}

void CombProcessor::updateParams(Parameters params)
{
    if (params.freq != -1.0f)
        freq.setTargetValue(params.freq);
    
    q.setTargetValue(params.resonance);
    timbre.setTargetValue(params.timbre);
    curve.setTargetValue(params.curve);
    spread.setTargetValue(params.spread);
    glide = params.glide;
    mode = params.mode;
    
    if (glide != lastGlide)
    {
        freq.reset(sampleRate, glide);
        lastGlide = glide;
    }
}

CombProcessor::Parameters& CombProcessor::getParams()
{
    return curParams;
}

void CombProcessor::setFrequency(float freq)
{
    this->freq.setTargetValue(freq);
}

void CombProcessor::setCurveOffset(float offset)
{
    curve.setTargetValue(curParams.curve + offset);
}

void CombProcessor::updateParamsObject(float freq, float resonance, float timbre, float curve, float spread)
{
    curParams.freq = freq;
    curParams.resonance = resonance;
    curParams.timbre = timbre;
    curParams.curve = curve;
    curParams.spread = spread;
    curParams.glide = glide;
    curParams.mode = mode;
}




bool CombProcessor::updateFilterSettings(float curFreq, float curQ, float curSpread, int i)
{
    float harmFreq, harmQ, nyquist = sampleRate / 2.0f;
    
    harmFreq = i ? curFreq * pow(float(i + 1), curSpread) : curFreq;
    harmQ = curQ * (float(i) / 2.f + 1);
    
    if (harmFreq > nyquist)
    {
        switch (mode) {
            case FreqOutOfBoundsMode::Ignore:
                return false;
                break;
                
            case FreqOutOfBoundsMode::Wrap:
                while (harmFreq >= nyquist)
                    harmFreq -= nyquist;
                
                harmFreq += 20.0f;
                break;
                
            case FreqOutOfBoundsMode::Fold:
                while (harmFreq >= nyquist || harmFreq < 20.0f)
                    harmFreq = (harmFreq - nyquist) - nyquist;
                break;
                
            default:
                DBG("No mode specified");
                return false;
                break;
        }
    }
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        chain[ch][i].get<ChainPositions::Filter>().setCutoffFrequency(harmFreq);
        chain[ch][i].get<ChainPositions::Filter>().setResonance(harmQ);
    }
    
    return true;
}

void CombProcessor::updateTimbre(float curTimbre, int i)
{
    float oddGain, evenGain;
    
    oddGain = -curTimbre + 1.f;
    evenGain = curTimbre;
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (i == 0)
            chain[ch][i].get<ChainPositions::Timbre>().setGainLinear(1.f);
        else if (i % 2 == 1)
            chain[ch][i].get<ChainPositions::Timbre>().setGainLinear(oddGain);
        else if (i % 2 == 0)
            chain[ch][i].get<ChainPositions::Timbre>().setGainLinear(evenGain);
    }
}

/*
void CombProcessor::updateCurve(float curCurve, float curQ, int i)
{
    float gain, normCurveGain, curveQ;
    
    curveQ = curQ * (float(i) / 2.f + 1);
    
    // compensate gain for tighter q values because nonlinear filter
    normCurveGain = pow((1.f/curveQ), 0.6);
    
    gain = pow((-1.f / float(MAX_NUM_FILTERS)) * float(i) + 1.f, (1.f / curCurve));
    gain *= normCurveGain;
    
    for (int ch = 0; ch < numChannels; ++ch)
        chain[ch][i].get<ChainPositions::Curve>().setGainLinear(gain);
}
 */

void CombProcessor::updateCurve(float curCurve, float curQ, int i)
{
    float gain, curveQ;
    
    curveQ = curQ * (float(i) / 2.f + 1);
    gain = Decibels::decibelsToGain( curCurve * i );
    gain *= pow((1.f/curveQ), 0.6);
    
    for (int ch = 0; ch < numChannels; ++ch)
        chain[ch][i].get<ChainPositions::Curve>().setGainLinear(gain);
}

}
