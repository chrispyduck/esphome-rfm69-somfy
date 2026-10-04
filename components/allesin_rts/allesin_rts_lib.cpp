#include "allesin_rts_lib.h"
#include "allesin_signals.h"

namespace esphome {
namespace allesin_rts {

using rfm69_direct::radio;

void AllesinRTS::initRadio() {
  // The shared RFM69 (components/rfm69_direct/) owns the one-time SPI init and
  // chip-present check - this just reads back the result (see
  // SomfyRTS::initRadio() in components/somfy_rts/somfy_rts_lib.cpp for the same
  // pattern, and RFM69DirectHub::setup() for why radio.version() is already valid
  // by the time any protocol driver is constructed).
  _radioOk = (radio.version() == 0x24);
}

// Bit-bangs one captured run-length waveform: `runs[i]` is how long (in
// microseconds) to hold the tone that alternates starting from tone 0 (DIO2 LOW -
// carrier at f_c - deviation). See allesin_signals.h for why only durations need to
// be stored. Each noInterrupts()/interrupts() pair is kept well under ESP-IDF's
// ~300ms interrupt-watchdog threshold (unlike Somfy's three short frames, one
// Allesin button press is a single ~640ms waveform - see the file header comment in
// allesin_rts_lib.h - so it must be chunked rather than wrapped in one pair; a
// chunk boundary can only ever land mid-frame, not mid-press, since the signal
// doesn't expose frame boundaries to replay around, but the capture repeats its
// command 11 times, so losing part of one repeat to a brief jitter window is
// expected to be harmless - unverified against real hardware, see the README).
static void playRuns(const uint16_t *runs, size_t count) {
  constexpr uint32_t CHUNK_US = 150000;  // comfortably under the ~300ms watchdog
  bool tone_high = false;                // every capture starts on tone 0 (LOW)
  uint32_t chunk_start = micros();
  noInterrupts();
  for (size_t i = 0; i < count; i++) {
    if (tone_high) {
      RFM69_DIRECT_TRANSMIT_HIGH();
    } else {
      RFM69_DIRECT_TRANSMIT_LOW();
    }
    delayMicroseconds(runs[i]);
    tone_high = !tone_high;

    if (micros() - chunk_start > CHUNK_US) {
      interrupts();
      noInterrupts();
      chunk_start = micros();
    }
  }
  RFM69_DIRECT_TRANSMIT_LOW();
  interrupts();
}

void AllesinRTS::sendAllesin(AllesinCommand command) {
  if (!_radioOk)
    return;

  // The shared radio may have been left in Somfy RTS's continuous-OOK mode (or at
  // its frequency) by a previous send - put it back into what Allesin needs. Cheap
  // (a few SPI register writes); safe to redo on every send.
  radio.setFskMode(ALLESIN_DEVIATION_HZ);
  radio.setFrequencyMHz(ALLESIN_FREQUENCY_MHZ);
  radio.setPowerLevel(20);

  switch (command) {
    case ALLESIN_CMD_UP:
      playRuns(ALLESIN_UP_RUNS, sizeof(ALLESIN_UP_RUNS) / sizeof(ALLESIN_UP_RUNS[0]));
      break;
    case ALLESIN_CMD_DOWN:
      playRuns(ALLESIN_DOWN_RUNS, sizeof(ALLESIN_DOWN_RUNS) / sizeof(ALLESIN_DOWN_RUNS[0]));
      break;
    case ALLESIN_CMD_STOP:
      playRuns(ALLESIN_STOP_RUNS, sizeof(ALLESIN_STOP_RUNS) / sizeof(ALLESIN_STOP_RUNS[0]));
      break;
  }
}

}  // namespace allesin_rts
}  // namespace esphome
