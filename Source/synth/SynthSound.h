#ifndef SYNTHSOUND_H
#define SYNTHSOUND_H

#include <JuceHeader.h>

class SynthSound : public SynthesiserSound
{
public:
    bool appliesToNote (int midiNoteNumber) override { return true; }
    bool appliesToChannel (int midiChannel) override { return true; }
    
};

#endif // SYNTHSOUND_H
