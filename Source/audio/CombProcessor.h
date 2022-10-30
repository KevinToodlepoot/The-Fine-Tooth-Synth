/*
  ==============================================================================

    CombProcessor.h
    Created: 7 Oct 2022 2:54:57pm
    Author:  Kevin Kopczynski

  ==============================================================================
*/

#ifndef COMBPROCESSOR_H
#define COMBPROCESSOR_H

#include <JuceHeader.h>
#include "../config.h"

using SVFilter = dsp::StateVariableTPTFilter<float>;
using Gain = dsp::Gain<float>;
using Chain = dsp::ProcessorChain<SVFilter, Gain, Gain>;

enum ChainPositions
{
    Filter,
    Timbre,
    Curve
};

namespace audio
{

class CombProcessor
{
public:
    enum class FreqOutOfBoundsMode
    {
        Ignore,
        Wrap,
        Fold
    };
    
    struct Parameters
    {
        Parameters(float freq, float resonance, float timbre, float curve, float spread, float glide, FreqOutOfBoundsMode mode = FreqOutOfBoundsMode::Ignore)
        {
            this->freq = freq;
            this->resonance = resonance;
            this->timbre = timbre;
            this->curve = curve;
            this->spread = spread;
            this->mode = mode;
            this->glide = glide;
        }
        
        float freq, resonance, timbre, curve, spread, glide;
        FreqOutOfBoundsMode mode;
    };
    
    CombProcessor(unsigned int _maxNumFilters, int _numChannels);
    ~CombProcessor() {;}
    
    void prepare(const dsp::ProcessSpec& spec);
    void reset();
    void process(AudioBuffer<float> &buffer, int numSamples);
    void updateParams(Parameters params);
    Parameters& getParams();
    
private:
    bool updateFilterSettings(float curFreq, float curQ, float curSpread, int i);
    void updateTimbre(float curTimbre, int i);
    void updateCurve(float curCurve, float curQ, int i);
    void updateParamsObject(float freq, float resonance, float timbre, float curve, float spread);
    
    std::vector<std::array<Chain, MAX_NUM_FILTERS>> chain;
    
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> freq, q, spread;
    SmoothedValue<float, ValueSmoothingTypes::Linear> timbre, curve;
    FreqOutOfBoundsMode mode;
    Parameters curParams;
    float lastGlide = GLIDE_DEFAULT, glide = GLIDE_DEFAULT;
    
    AudioBuffer<float> tempBuffer, outBuffer;
    unsigned int maxNumFilters;
    int numChannels, numFilters;
    double sampleRate;
};

}

#endif
