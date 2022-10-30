#ifndef FILTERDISPLAY_H
#define FILTERDISPLAY_H

#include <JuceHeader.h>
#include "../audio/CombProcessor.h"

//==============================================================================
/*
*/
class FilterDisplay  : public Component
{
public:
    class FreqBand : public Component
    {
    public:
        FreqBand(int _harm) :
            params(A440, RESONANCE_DEFAULT, TIMBRE_DEFAULT, CURVE_DEFAULT, SPREAD_DEFAULT, GLIDE_DEFAULT),
            harm(_harm)
        {
            
        }
        
        ~FreqBand()
        {
            
        }
        
        void paint(Graphics &g) override
        {
            updateFilterSettings();
            updateGain();
            
            if (gain > -36.0f)
            {
                int x, y, width, height;
                float resWidth;
                
                resWidth = mapFromLog10(resonance, RESONANCE_MAX, maxRes);
                width = round(jmap(1.0f - pow(resWidth - 1.0f, 2.0f), 5.0f, 1.0f));

                height = round(jlimit(0.5f, float(getHeight()), jmap(gain, -36.0f, 12.0f, 5.0f, float(getHeight()) - 5.0f)));
                
                x = round(getWidth() * mapFromLog10(freq, 20.0f, 20000.0f) - width / 2.0f);
                y = round(getHeight() - height);
                
                g.setColour(Colour(36, 130, 119));
                g.fillRect(x, y, width, height);
            }
        }
        
        void resized() override
        {
            
        }
        
        void updateParams(float resonance, float timbre, float curve, float spread, CombProcessor::FreqOutOfBoundsMode mode)
        {
            params.resonance = resonance;
            params.timbre = timbre;
            params.curve = curve;
            params.spread = spread;
            params.mode = mode;
            
            repaint();
        }
        
        bool updateFilterSettings()
        {
            freq = harm ? inFreq * pow(float(harm + 1), params.spread) : inFreq;
            resonance = params.resonance * (float(harm) / 2.f + 1);
            
            if (freq > 20000.0f)
            {
                return false;
            }
            
            return true;
        }
        
        void updateGain()
        {
            float oddGain, evenGain;
            
            oddGain = -params.timbre + 1.f;
            evenGain = params.timbre;
            
            if (harm == 0)
                gain = 1.0f;
            else if (harm % 2 == 1)
                gain = oddGain;
            else if (harm % 2 == 0)
                gain = evenGain;
            
            float curveGainDB;
            curveGainDB = params.curve * harm;
            
            gain *= Decibels::decibelsToGain(curveGainDB);
            
            gain = Decibels::gainToDecibels(gain);
        }
        
    private:
        Chain chain;
        
        float freq, resonance, gain;
        audio::CombProcessor::Parameters params;
        audio::CombProcessor::FreqOutOfBoundsMode mode;
        
        static constexpr float inFreq = 50.0f;
        static constexpr float maxRes = RESONANCE_MAX * (MAX_NUM_FILTERS / 2.0f);
        
        int harm;
    };
    
    
    FilterDisplay(FineToothMIDIAudioProcessor& p) : audioProcessor(p)
    {
        for (int harm = 0; harm < MAX_NUM_FILTERS; ++harm)
        {
            freqBands[harm] = std::make_unique<FreqBand>(harm);
            addAndMakeVisible(*freqBands[harm]);
        }
    }

    ~FilterDisplay() override
    {
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(Colour(86, 171, 145));
        g.fillRoundedRectangle(bounds, 10.0f);
        
        Array<float> freqs
        {
            20, 30, 40, 50, 100,
            200, 300, 400, 500, 1000,
            2000, 3000, 4000, 5000, 10000,
            20000
        };
        
        g.setColour(Colour(103, 185, 154));
        for (auto f : freqs)
        {
            auto normX = mapFromLog10(f, 10.0f, 20000.0f);
            
            g.drawVerticalLine(getWidth() * normX, 5.0f, getHeight() - 5.0f);
        }
        
        Array<float> gain
        {
            -36, -24, -12, 0, 12
        };
        
        for (auto gDb : gain)
        {
            auto y = jmap(gDb, -36.0f, 12.0f, float(getHeight()) - 5.0f, 5.0f);
            g.drawHorizontalLine(y, 5.0f, getWidth() - 5.0f);
        }
        
        g.setColour(Colour(36, 130, 119));
        g.drawRoundedRectangle(bounds.reduced(2.0f), 8.0f, 4.0f);
    }

    void resized() override
    {
        for (int harm = 0; harm < MAX_NUM_FILTERS; ++harm)
        {
            freqBands[harm]->setBounds(getLocalBounds());
        }
    }
    
    void updateAll()
    {
        auto settings = getChainSettings(audioProcessor.apvts);
        
        for (int harm = 0; harm < MAX_NUM_FILTERS; ++harm)
        {
            freqBands[harm]->updateParams(settings.resonance, settings.timbre, settings.curve, settings.spread, CombProcessor::FreqOutOfBoundsMode::Ignore);
        }
    }

private:
    std::array<std::unique_ptr<FreqBand>, MAX_NUM_FILTERS> freqBands;
    
    FineToothMIDIAudioProcessor& audioProcessor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterDisplay)
};

#endif // FILTERDISPLAY_H
