#ifndef PARAMS_H
#define PARAMS_H

#include "config.h"

using namespace audio;

struct ChainSettings
{
    // SYNTH SECTION
    float attack {0};
    float decay {0};
    float sustain {0};
    float release {0};
    
    // FILTER BANK SECTION
    float resonance {0};
    float timbre {0};
    float curve {0};
    float spread {0};
    
    float glide {0};
    
    int inputMode {0};
    int aliasMode {0};
};

inline ChainSettings getChainSettings(APVTS& apvts)
{
    ChainSettings settings;
    
    //==============================================================================
    settings.attack = apvts.getRawParameterValue("Attack")->load();
    settings.decay = apvts.getRawParameterValue("Decay")->load();
    settings.sustain = apvts.getRawParameterValue("Sustain")->load();
    settings.release = apvts.getRawParameterValue("Release")->load();
    
    //==============================================================================
    settings.resonance = apvts.getRawParameterValue("Resonance")->load();
    settings.timbre = apvts.getRawParameterValue("Timbre")->load();
    settings.curve = apvts.getRawParameterValue("Curve")->load();
    settings.spread = apvts.getRawParameterValue("Spread")->load();
    
    settings.glide = apvts.getRawParameterValue("Glide")->load();
    
    settings.inputMode = apvts.getRawParameterValue("Input Mode")->load();
    settings.aliasMode = apvts.getRawParameterValue("Alias Mode")->load();
    
    return settings;
}

static APVTS::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    
    auto pAttack = std::make_unique<AudioParameterFloat>
        (ParameterID ("Attack", 1), "Attack", ATTACK_MIN, ATTACK_MAX, ATTACK_DEFAULT);
    auto pDecay = std::make_unique<AudioParameterFloat>
        (ParameterID ("Decay", 1), "Decay", DECAY_MIN, DECAY_MAX, DECAY_DEFAULT);
    auto pSustain = std::make_unique<AudioParameterFloat>
        (ParameterID ("Sustain", 1), "Sustain", SUSTAIN_MIN, SUSTAIN_MAX, SUSTAIN_DEFAULT);
    auto pRelease = std::make_unique<AudioParameterFloat>
        (ParameterID ("Release", 1), "Release", RELEASE_MIN, RELEASE_MAX, RELEASE_DEFAULT);
    
    auto pResonance = std::make_unique<AudioParameterFloat>
        (ParameterID ("Resonance", 1), "Resonance", RESONANCE_MIN, RESONANCE_MAX, RESONANCE_DEFAULT);
    auto pTimbre = std::make_unique<AudioParameterFloat>
        (ParameterID ("Timbre", 1), "Timbre", TIMBRE_MIN, TIMBRE_MAX, TIMBRE_DEFAULT);
    auto pCurve = std::make_unique<AudioParameterFloat>
        (ParameterID ("Curve", 1), "Curve", CURVE_MIN, CURVE_MAX, CURVE_DEFAULT);
    auto pSpread = std::make_unique<AudioParameterFloat>
        (ParameterID ("Spread", 1), "Spread", SPREAD_MIN, SPREAD_MAX, SPREAD_DEFAULT);
    
    auto pGlide = std::make_unique<AudioParameterFloat>
        (ParameterID ("Glide", 1), "Glide", GLIDE_MIN, GLIDE_MAX, GLIDE_DEFAULT);
    
    auto pInputMode = std::make_unique<AudioParameterChoice>
        (ParameterID ("Input Mode", 1), "Input Mode", StringArray("Noise", "Ext"), 0);
    auto pAliasMode = std::make_unique<AudioParameterChoice>
        (ParameterID ("Alias Mode", 1), "Alias Mode", StringArray("Ignore", "Wrap", "Fold"), 0);
        
    
    params.push_back(std::move(pAttack));
    params.push_back(std::move(pDecay));
    params.push_back(std::move(pSustain));
    params.push_back(std::move(pRelease));
    params.push_back(std::move(pResonance));
    params.push_back(std::move(pTimbre));
    params.push_back(std::move(pCurve));
    params.push_back(std::move(pSpread));
    params.push_back(std::move(pGlide));
    params.push_back(std::move(pInputMode));
    params.push_back(std::move(pAliasMode));
    
    return { params.begin(), params.end() };
}

#endif // PARAMS_H
