#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>              
#include <I2S.h>               
#include "Audio.h"             
#include "SynthEngine.h"
#include "BPMClockManager.h"
#include "pico/util/queue.h"   // RP2350 SDK thread-safe queue

#ifndef USB_PRODUCT
#define USB_PRODUCT "JTeensy RP2350 Synth"
#endif

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

SynthEngine synth;
BPMClockManager bpmClock;
I2S i2sOutput(OUTPUT);

const int SAMPLE_RATE = 22050;
int16_t audioBuffer[AUDIO_BLOCK_SAMPLES * 2]; 

volatile bool audioReady = false;

// Thread-safe event structure updated to include channel
enum MidiEventType { EVENT_NOTE_ON, EVENT_NOTE_OFF, EVENT_CC, EVENT_PROGRAM_CHANGE };
struct MidiEvent {
  MidiEventType type;
  uint8_t channel;
  uint8_t data1;
  uint8_t data2;
};

queue_t midiEventQueue;

class I2SBridgeNode : public AudioStream {
public:
  I2SBridgeNode() : AudioStream(1, inputQueueArray) {}
  
  void updateGraph() {
    update_all();
  }

  virtual void update(void) override {
    audio_block_t *block = receiveReadOnly(0);

    if (block) {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
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
  // Initialize thread-safe queue for up to 32 pending MIDI events
  queue_init(&midiEventQueue, sizeof(MidiEvent), 32);

  AudioMemory(60);

  usb_midi.setStringDescriptor(USB_PRODUCT);
  
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  Serial.begin(115200);

  // Core 0 MIDI Callbacks: Capture channel, data1, and data2 safely into the queue
  MIDI.setHandleNoteOn([](byte ch, byte note, byte vel){
    MidiEvent ev = {EVENT_NOTE_ON, ch, note, vel};
    queue_try_add(&midiEventQueue, &ev);
  });
  
  MIDI.setHandleNoteOff([](byte ch, byte note, byte vel){
    MidiEvent ev = {EVENT_NOTE_OFF, ch, note, vel};
    queue_try_add(&midiEventQueue, &ev);
  });

  MIDI.setHandleProgramChange([](byte ch, byte program){
    MidiEvent ev = {EVENT_PROGRAM_CHANGE, ch, 0, program};
    queue_try_add(&midiEventQueue, &ev);
  });
  
  MIDI.setHandleControlChange([](byte ch, byte num, byte val){
    MidiEvent ev = {EVENT_CC, ch, num, val};
    queue_try_add(&midiEventQueue, &ev);
  });

  MIDI.begin(MIDI_CHANNEL_OMNI);
  delay(100);
  
  synth.setOsc1Mix(0.2f);
  synth.setOsc2Mix(0.2f);
  synth.setFilterResonance(0.0f); 
  synth.handleControlChange(1, 65, 0);
  synth.handleControlChange(1, 5, 0); // Disable glide/portamento

  audioReady = true;
}

void loop() {
  tud_task();      
  MIDI.read();     
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


void __not_in_flash_func(loop1)() {
  MidiEvent ev;
  while (queue_try_remove(&midiEventQueue, &ev)) {
    switch (ev.type) {
      case EVENT_NOTE_ON:
        synth.noteOn(ev.data1, ev.data2 / 127.0f);
        break;
      case EVENT_NOTE_OFF:
        synth.noteOff(ev.data1);
        break;
      case EVENT_PROGRAM_CHANGE:
        synth.handleProgramChange(ev.channel, ev.data2);
        break;
      case EVENT_CC:
        synth.handleControlChange(ev.channel, ev.data1, ev.data2);
        break;
    }
  }

  if (i2sOutput.availableForWrite() >= sizeof(audioBuffer)) {
    synth.update();
    bridgeNode.updateGraph();
    i2sOutput.write((uint8_t*)audioBuffer, sizeof(audioBuffer));
  }
}
