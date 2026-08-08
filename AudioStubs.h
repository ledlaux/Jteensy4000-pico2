#pragma once
#include <Arduino.h>
#include <cstdint>

inline void AudioNoInterrupts() {}
inline void AudioInterrupts() {}

#ifndef AUDIO_SAMPLE_RATE_EXACT
#define AUDIO_SAMPLE_RATE_EXACT 44100.0f
#endif

#ifndef WAVEFORM_ARBITRARY
#define WAVEFORM_ARBITRARY 100
#define WAVEFORM_SAWTOOTH  1
#define WAVEFORM_SUPERSAW  2
#define WAVEFORM_SINE      3
#define WAVEFORM_TRIANGLE  4
#define WAVEFORM_SQUARE    5
#define WAVEFORM_PULSE     6
#endif

// Mock Stub matching your exact Jteensy FXChainBlock API calls
class AudioEffectPlateReverb_i16 : public AudioStream {
public:
    AudioEffectPlateReverb_i16() : AudioStream(2, nullptr) {}
    void update(void) override {}
    void bypass_set(bool state) {}
    void mix(float mixValue) {}
    void size(float roomSize) {}
    void hidamp(float damp) {}
    void lodamp(float damp) {}
};