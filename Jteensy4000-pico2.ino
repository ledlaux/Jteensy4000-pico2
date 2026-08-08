#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>              
#include <I2S.h>               
#include "Audio.h"             
#include "SynthEngine.h"
#include "BPMClockManager.h"

#ifndef USB_PRODUCT
#define USB_PRODUCT "JTeensy RP2350 Synth"
#endif

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

SynthEngine synth;
BPMClockManager bpmClock;

I2S i2sOutput(OUTPUT);

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 64;
int16_t audioBuffer[BUFFER_SIZE * 2]; 

volatile bool audioReady = false;

class I2SBridgeNode : public AudioStream {
public:
  I2SBridgeNode() : AudioStream(1, inputQueueArray) {}
  
  void updateGraph() {
    update_all();
  }

  virtual void update(void) override {
    audio_block_t *block = receiveReadOnly(0);

    if (block) {
      for (int i = 0; i < BUFFER_SIZE; i++) {
        audioBuffer[i * 2]     = block->data[i];
        audioBuffer[i * 2 + 1] = block->data[i];
      }
      release(block);
    } else {
      memset(audioBuffer, 0, sizeof(audioBuffer));
    }
  }

private:
  audio_block_t *inputQueueArray[1];
};

I2SBridgeNode bridgeNode;

AudioConnection patchCord1(synth.getVoiceMixer(), 0, bridgeNode, 0);

void SynthEngine::handleProgramChange(uint8_t channel, uint8_t program) {
    Presets::presets_loadByGlobalIndex(*this, program, channel);
}

#include "hardware/vreg.h"
#include "hardware/clocks.h"

void setup() {

  vreg_set_voltage(VREG_VOLTAGE_1_30);
  set_sys_clock_khz(330000, true);
  AudioMemory(60);

  usb_midi.setStringDescriptor(USB_PRODUCT);
  
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  Serial.begin(115200);

  MIDI.setHandleNoteOn([](byte ch, byte note, byte vel){
    Serial.print("CORE0 NoteOn Recv -> Note: ");
    Serial.print(note);
    Serial.print(" Vel: ");
    Serial.println(vel);
    __dmb();
    synth.noteOn(note, vel / 127.0f);
    __dmb();
  });
  
  MIDI.setHandleNoteOff([](byte ch, byte note, byte vel){
    Serial.println("CORE0 NoteOff Recv");
    __dmb();
    synth.noteOff(note);
    __dmb();
  });

  MIDI.setHandleProgramChange([](byte ch, byte program){
    __dmb();
    synth.handleProgramChange(ch - 1, program);
    __dmb();
  });
  
  MIDI.setHandleControlChange([](byte ch, byte num, byte val){
    __dmb();
    synth.handleControlChange(ch - 1, num, val);
    __dmb();
  });

  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  delay(100);
  
  synth.setOsc1Mix(0.2f);
  synth.setOsc2Mix(0.2f);
  synth.setFilterResonance(0.0f); 

  audioReady = true;
}

void loop() {
  tud_task();      
  MIDI.read();     
  synth.update(); 
}

void setup1() {
  while (!audioReady) {
    delay(10);
  }

  i2sOutput.setBCLK(27);
  i2sOutput.setDATA(26);
  i2sOutput.setBitsPerSample(16); 
  
  if (!i2sOutput.begin(SAMPLE_RATE)) {
    while (1) {
      delay(100);
    }
  }
}

void loop1() {
  __dmb();
  bridgeNode.updateGraph();
  // Only write when the I2S hardware actually has room for this block
  if (i2sOutput.availableForWrite() >= sizeof(audioBuffer)) {
    i2sOutput.write((uint8_t*)audioBuffer, sizeof(audioBuffer));
  }
}