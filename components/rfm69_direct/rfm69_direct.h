#pragma once
#include "esphome/core/component.h"
#include "RFM69OOK.h"

// Arduino.h (pulled in above via RFM69OOK.h) defines a pile of bare, unnamespaced
// macros from the target board's pins_arduino.h - on adafruit_feather_esp32_v2
// that includes `#define BUTTON 38`. Since this header (like every component
// header) ends up included from ESPHome's generated esphome.h *before*
// esphome/core/component_iterator.h, that macro textually replaces the `BUTTON`
// enumerator in that file's `enum class IteratorState`, which fails to compile.
// BUTTON isn't used by this driver, so it's safe to drop; if a future board's
// pins_arduino.h defines some other macro that collides with an ESPHome core
// identifier, it'll fail the same way and need the same treatment here.
#undef BUTTON

namespace esphome {
namespace rfm69_direct {

// RFM69 FeatherWing wiring on the Adafruit ESP32 Feather V2. SCK/MOSI/MISO are
// fixed by the stacked-header SPI bus (GPIO5/18/19) and need no jumpers. The Wing
// has its own labeled jumper pads (A-F, RX/TX/SCL/SDA) next to IRQ/CS/RST for the
// other three - solder a wire directly between two pads on the Wing itself. Per
// Adafruit's EAGLE schematics (see the README; CS/pad B confirmed on hardware), pad
// "A" = GPIO27 and pad "B" = GPIO33 on this specific board: bridge Wing "IRQ" to
// Wing "A", and Wing "CS" to Wing "B". RST is left unbridged: this driver never
// issues a hardware reset, and the RFM69's RST line has an internal pulldown.
//
// There is exactly one physical RFM69 chip per device, shared by every protocol
// driver (Somfy RTS, Allesin RTS, ...) through this one component - see `radio`
// below. To change pins, edit these two and re-flash.
constexpr uint8_t RFM69_DIRECT_CS_PIN = 33;
constexpr uint8_t RFM69_DIRECT_DIO2_PIN = 27;

// Fully-qualified so these expand correctly from any namespace (every protocol
// driver uses them - see components/somfy_rts/ and components/allesin_rts/).
#if defined(ESP8266)
#define RFM69_DIRECT_TRANSMIT_HIGH() (GPOS = 1 << esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN)
#define RFM69_DIRECT_TRANSMIT_LOW() (GPOC = 1 << esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN)
#elif defined(ESP32)
// GPOS/GPOC (ESP8266's fast direct-register set/clear macros) don't exist on the
// ESP32 Arduino core; digitalWrite()'s overhead (tens of ns) is negligible against
// both protocols' symbol timing (640us for Somfy, 25us for Allesin).
#define RFM69_DIRECT_TRANSMIT_HIGH() (digitalWrite(esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN, HIGH))
#define RFM69_DIRECT_TRANSMIT_LOW() (digitalWrite(esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN, LOW))
#else
#define RFM69_DIRECT_TRANSMIT_HIGH() (PORTD |= 1 << esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN)
#define RFM69_DIRECT_TRANSMIT_LOW() (PORTD &= !(1 << esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN))
#endif

// The single physical radio, defined in rfm69_direct.cpp. Protocol drivers
// (components/somfy_rts/somfy_rts_lib.cpp, components/allesin_rts/allesin_rts_lib.cpp)
// reconfigure it (setOokMode()/setFskMode(), setFrequencyMHz(), setPowerLevel()) right
// before every send and bit-bang RFM69_DIRECT_DIO2_PIN directly via the macros above,
// matching this repo's existing Somfy RTS timing code for minimal per-symbol overhead.
extern RFM69OOK radio;

// Owns the one-time RFM69 bring-up (SPI init, chip-present check, TX mode, high-power
// PA stages) that every protocol driver depends on. Must set up before any component
// that uses `radio` - see get_setup_priority().
class RFM69DirectHub : public Component {
 public:
  void setup() override;
  void dump_config() override;
  // BUS, not just HARDWARE: this is the shared radio every protocol hub (Somfy RTS,
  // Allesin RTS) depends on, so it must finish setup() before any of them run theirs,
  // regardless of YAML declaration order (ESPHome runs higher-priority setup() first).
  float get_setup_priority() const override { return setup_priority::BUS; }

  bool radio_ok() const { return radio_ok_; }

 protected:
  bool radio_ok_{false};
};

}  // namespace rfm69_direct
}  // namespace esphome
