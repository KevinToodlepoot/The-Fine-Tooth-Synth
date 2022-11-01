#ifndef SYNTHVOICE_H
#define SYNTHVOICE_H

#include <JuceHeader.h>
#include "SynthSound.h"
#include "../audio/CombProcessor.h"
#include "../config.h"

class SynthVoice : public SynthesiserVoice
{
public:
    bool canPlaySound(SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound *sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void reset();
    void renderNextBlock(AudioBuffer<float> &outputBuffer, int startSample, int numSamples) override;
    void fillBuffer(AudioBuffer<float> &buffer, int numSamples);
    
    audio::CombProcessor& getCombProcessor() { return comb; }
    ADSR& getADSR() { return adsr; }
    
private:
    audio::CombProcessor comb{MAX_NUM_FILTERS};
    ADSR adsr;
    
    AudioBuffer<float> combBuffer;
    
    bool isPrepared = false;
};

#endif
