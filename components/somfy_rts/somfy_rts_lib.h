// **********************************************************************************
// Somfy RTS Arduino library compatible with generic AM 433.42Mhz transmitter or RFM69
// **********************************************************************************
// Copyright Etienne Mouragnon (2020)
// Based on previous work:
// https://github.com/Nickduino/Somfy_Remote
// https://github.com/kobuki/RFM69OOK
// License: GPL-3.0, see http://www.gnu.org/licenses/gpl-3.0.txt
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
//
// Vendored from https://github.com/etimou/SomfyRTS for use as an ESPHome
// external_component. Patched for ESP32:
//   - Added SOMFY_RFM69_CS_PIN / SOMFY_RFM69_DIO2_PIN: the pins the RFM69 FeatherWing
//     must be wired to. These are fixed at compile time because the underlying
//     RFM69OOK driver is instantiated as a single process-wide global object in
//     somfy_rts_lib.cpp (there is only one physical radio, so this is fine) - to
//     change them, edit both this file and somfy_rts_lib.cpp and re-flash.
//   - Renamed from SomfyRTS.h to somfy_rts_lib.h to avoid a filename collision with
//     this component's own somfy_rts.h on case-insensitive filesystems.
//   - Rolling-code storage goes through ESPHome's own global_preferences system
//     instead of Arduino's EEPROM library (which would have committed a whole NVS
//     blob on every send): save()/load() are in-RAM operations, and the flash write
//     happens on ESPHome's own IntervalSyncer schedule (default every 60s, see the
//     `preferences:` YAML config). See rollingCodePref() and buildFrameSomfy() in
//     somfy_rts_lib.cpp.
//   - The physical RFM69 driver (RFM69OOK), its pin defines, and the one-time chip
//     init moved out to components/rfm69_direct/ so Allesin RTS (see
//     components/allesin_rts/) can share the same physical radio - there is only one
//     RFM69 chip per device. This file now gets SOMFY_RFM69_CS_PIN/_DIO2_PIN and the
//     `radio` object by including rfm69_direct.h instead of defining them itself.
#ifndef SOMFY_RTS_H
#define SOMFY_RTS_H
#include <Arduino.h>
#include <map>
#include "esphome/components/rfm69_direct/rfm69_direct.h"
#include "esphome/core/preferences.h"

// Aliases kept for this file's own readability; the actual pins live in
// components/rfm69_direct/rfm69_direct.h since they're no longer Somfy-specific.
#define SOMFY_RFM69_CS_PIN esphome::rfm69_direct::RFM69_DIRECT_CS_PIN
#define SOMFY_RFM69_DIO2_PIN esphome::rfm69_direct::RFM69_DIRECT_DIO2_PIN

#define SYMBOL 640
#define UP 0x2
#define STOP 0x1
#define DOWN 0x4
#define PROG 0x8

#define TSR_RFM69 1
#define TSR_AM 2

class SomfyRTS {
  public:
    void initRadio();
    void sendSomfy(unsigned char virtualRemoteNumber, unsigned char actionCommand);
    // EEPROM_address no longer addresses actual EEPROM bytes (see the file header
    // comment above) - it's now folded into each remote's preference-key hash, so
    // distinct SomfyRTS instances can still be given non-colliding storage by
    // configuring different values here, same as before.
    void configRTS(unsigned int EEPROM_address, unsigned long RTS_address);
    bool radioOk() const { return _radioOk; }  // false if the RFM69 didn't answer during init

    SomfyRTS(byte pinTx, unsigned char transmitterType) {

      _pinTx = pinTx;
      _EEPROM_address = 0;
      _RTS_address = 0x121300;
      _actionCommand = STOP;
      _virtualRemoteNumber = 0;
      _transmitterType = transmitterType;
      _radioOk = false;

      initRadio();
    }

  protected:
    void sendCommandSomfy(byte sync);
    void buildFrameSomfy();
    esphome::ESPPreferenceObject &rollingCodePref(unsigned char virtualRemoteNumber);

    byte _pinTx;
    unsigned int _EEPROM_address;
    unsigned long _RTS_address;
    unsigned char _actionCommand;
    unsigned char _virtualRemoteNumber;
    byte frame[7]; // frame for Somfy protocol
    unsigned char _transmitterType;
    bool _radioOk;
    std::map<unsigned char, esphome::ESPPreferenceObject> _rollingCodePrefs;

};

#endif
