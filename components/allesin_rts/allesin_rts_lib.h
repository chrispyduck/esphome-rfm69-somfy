// **********************************************************************************
// Allesin M130 roller blind RF replay, for the RFM69 (via components/rfm69_direct/)
// **********************************************************************************
// Unlike Somfy RTS (components/somfy_rts/), this is not a decoded frame format - no
// byte/command/checksum layout for this protocol has been published anywhere. It is
// a verbatim replay of one captured remote's RF signal, as run-length tone-duration
// data, captured off the air with a HackRF by forum user MartinsZB and published at:
//   https://community.home-assistant.io/t/allesin-m130-roller-blinds/697463/15
// See allesin_signals.h for the captured data itself and what is/isn't known about
// the signal's structure.
//
// The remote (AC2101-01A, driving an Allesin M130 motor) transmits plain static
// 2-FSK - no OOK, no rolling code/counter. Each button press is one continuous
// ~640ms waveform (a mandatory wake-up section the blind ignores everything without,
// followed by 11 repeats of the actual command) - the whole thing is captured in one
// run-length array per button, so sending a command here is just replaying that
// array once, unlike Somfy's three separately-built/sent frames.
//
// Since there's no known addressing in this signal, every configured allesin_rts
// cover sends the exact same RF bytes - see the README for what that means if you
// have more than one Allesin shade in range.
#pragma once
#include <Arduino.h>
#include "esphome/components/rfm69_direct/rfm69_direct.h"

namespace esphome {
namespace allesin_rts {

// Carrier/deviation as measured in the forum capture (433.9139-433.9143MHz center,
// +-19kHz tones). Untested against real hardware by this repo - see the README.
#define ALLESIN_FREQUENCY_MHZ 433.914f
#define ALLESIN_DEVIATION_HZ 19000

enum AllesinCommand : uint8_t {
  ALLESIN_CMD_UP = 0,
  ALLESIN_CMD_DOWN = 1,
  // STOP only has an effect while the blind is moving (per the forum capture notes);
  // pressed at rest it does nothing, same as on the real remote.
  ALLESIN_CMD_STOP = 2,
};

class AllesinRTS {
  public:
    void initRadio();
    void sendAllesin(AllesinCommand command);
    bool radioOk() const { return _radioOk; }  // false if the RFM69 didn't answer during init

    AllesinRTS() { _radioOk = false; initRadio(); }

  protected:
    bool _radioOk;
};

}  // namespace allesin_rts
}  // namespace esphome
