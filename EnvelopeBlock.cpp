#include "EnvelopeBlock.h"

// --- Lifecycle
void EnvelopeBlock::noteOn() {
    _envelope.noteOn();
}

void EnvelopeBlock::noteOff() {
    _envelope.noteOff();
}

// --- Outputs
float EnvelopeBlock::getValue() {
    // If the envelope is running, return a snapshot state. 
    // AudioEffectEnvelope uses isActive() to check state. If it's idle, it's 0.0f.
    if (_envelope.isActive()) {
        // Return 1.0f as placeholder if your core code checks binary state,
        // or return the current target level depending on your EnvelopeBlock.h layout.
        return 1.0f; 
    }
    return 0.0f;
}

AudioStream& EnvelopeBlock::input() {
    return _envelope;
}

AudioStream& EnvelopeBlock::output() {
    return _envelope;
}

// --- Parameter Setters
void EnvelopeBlock::setAttackTime(float time) {
    _attackTime = time;
    // Teensy AudioEffectEnvelope accepts milliseconds
    _envelope.attack(time * 1000.0f); 
}

void EnvelopeBlock::setDecayTime(float time) {
    _decayTime = time;
    _envelope.decay(time * 1000.0f);
}

void EnvelopeBlock::setSustainLevel(float level) {
    _sustainLevel = level;
    _envelope.sustain(level);
}

void EnvelopeBlock::setReleaseTime(float time) {
    _releaseTime = time;
    _envelope.release(time * 1000.0f);
}

void EnvelopeBlock::setADSR(float attack, float decay, float sustain, float release) {
    setAttackTime(attack);
    setDecayTime(decay);
    setSustainLevel(sustain);
    setReleaseTime(release);
}