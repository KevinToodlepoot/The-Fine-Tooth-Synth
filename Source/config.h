#ifndef CONFIG_H
#define CONFIG_H

// GENERAL DEFINES
#define A440                440.0f
#define MAX_NUM_FILTERS     100
#define SMOOTH_SEC          0.01f
#define NUM_VOICES          8

// PARAM DEFINES
#define ATTACK_MIN          0.0f
#define ATTACK_MAX          500.0f
#define ATTACK_DEFAULT      0.0f
#define DECAY_MIN           0.0f
#define DECAY_MAX           500.0f
#define DECAY_DEFAULT       10.0f
#define SUSTAIN_MIN         0.0f
#define SUSTAIN_MAX         1.0f
#define SUSTAIN_DEFAULT     1.0f
#define RELEASE_MIN         0.0f
#define RELEASE_MAX         500.0f
#define RELEASE_DEFAULT     0.0f
#define RESONANCE_MIN       10.0f
#define RESONANCE_MAX       100.0f
#define RESONANCE_DEFAULT   55.0f
#define TIMBRE_MIN          0.0f
#define TIMBRE_MAX          1.0f
#define TIMBRE_DEFAULT      0.5f
#define CURVE_MIN           -6.0f
#define CURVE_MAX           0.5f
#define CURVE_DEFAULT       0.0f
#define SPREAD_MIN          0.5f
#define SPREAD_MAX          2.0f
#define SPREAD_DEFAULT      1.0f
#define GLIDE_MIN           0.01f
#define GLIDE_MAX           1.0f
#define GLIDE_DEFAULT       0.01f

// NAMESPACE
namespace audio
{
    using SIMD = FloatVectorOperations;
    using APVTS = AudioProcessorValueTreeState;

    static float Pi = MathConstants<float>::pi;
    static float Tau = MathConstants<float>::twoPi;

    template<typename Float>
    inline float msInSamples(Float ms, Float Fs) noexcept
    {
        return ms * Fs * static_cast<Float>(.001);
    }

    template<typename Float>
    inline float pitchToScalar(Float pitch) noexcept
    {
        return pow(2.f, pitch / 12.f);
    }

    inline float midiToFreq(int midiNote) noexcept
    {
        return A440 * pow(2.0f, float( midiNote - 69.0f ) / 12.0f);
    }
}

#endif // CONFIG_H
