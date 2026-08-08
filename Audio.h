#pragma once

// 1. Core Language & Architecture Hooks (Must be absolute first)
#include <Arduino.h>
#include <cstdint>
#include <AudioStream.h>

// 2. Pico-Audio Library Headers
#include <synth_waveform.h>
#include <synth_dc.h>
#include <mixer.h>
#include <effect_delay.h>
#include <effect_envelope.h>
#include <AudioOutputI2S.h>

// 3. Your Local Project Audio Components
#include "AudioSynthSupersaw.h"
//#include "AudioFilterOBXa_OBXf.h"

// 4. Framework Call Stubs (Must be absolute last)
#include "AudioStubs.h"