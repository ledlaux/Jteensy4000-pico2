#pragma once
#include "Audio.h"
#include "effect_envelope.h"

// EnvelopeBlock handles ADSR envelope generation via AudioEffectEnvelope delegation
class EnvelopeBlock {
public:
    // --- Lifecycle
    void noteOn();
    void noteOff();

    // --- Outputs
    float getValue(); // Resolves: no declaration matches 'float EnvelopeBlock::getValue()'
    AudioStream& input();
    AudioStream& output();

    // --- Parameter Setters
    void setAttackTime(float time);
    void setDecayTime(float time);
    void setSustainLevel(float level);
    void setReleaseTime(float time);
    void setADSR(float attack, float decay, float sustain, float release);

    // --- Parameter Getters
    float getAttackTime() const { return _attackTime; }
    float getDecayTime() const { return _decayTime; }
    float getSustainLevel() const { return _sustainLevel; }
    float getReleaseTime() const { return _releaseTime; }

private:
    AudioEffectEnvelope _envelope;
    
    float _attackTime = 0.01f;
    float _decayTime = 0.1f;
    float _sustainLevel = 0.8f;
    float _releaseTime = 0.2f;
};