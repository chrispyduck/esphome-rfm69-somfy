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
//   - Rolling-code storage no longer uses Arduino's EEPROM library at all. Several
//     rounds of shrinking the blob and raising ESP-IDF's interrupt-watchdog timeout
//     still wasn't enough - EEPROM.commit() (a synchronous NVS flash write, which
//     internally does reads too) was repeatedly observed crashing real hardware
//     (TG1WDT_SYS_RESET) from this hot path. Storage now goes through ESPHome's own
//     global_preferences system instead: save()/load() are pure in-RAM operations
//     with no flash I/O at all, and the actual flash write is deferred to ESPHome's
//     own IntervalSyncer component (default: every 60s, see the `preferences:` YAML
//     config) - the entire ESPHome ecosystem already relies on that exact mechanism
//     for small, infrequently-changing persisted state, so it's far more
//     battle-tested than a one-off EEPROM-commit scheme. See rollingCodePref() and
//     buildFrameSomfy() in somfy_rts_lib.cpp.
#ifndef SOMFY_RTS_H
#define SOMFY_RTS_H
#include <Arduino.h>
#include <map>
#include "esphome/core/preferences.h"

// RFM69 FeatherWing wiring on the ESP32-Pico (HUZZAH32) Feather. SCK/MOSI/MISO are
// fixed by the stacked-header SPI bus (GPIO5/18/19) and need no jumpers. The Wing
// has its own labeled jumper pads (A-F, RX/TX/SCL/SDA) next to IRQ/CS/RST for the
// other three - solder a wire directly between two pads on the Wing itself. Per
// Adafruit's EAGLE schematics for this Wing + the HUZZAH32 (see the README), pad
// "A" = GPIO27 and pad "B" = GPIO33 on this specific board: bridge Wing "IRQ" to
// Wing "A", and Wing "CS" to Wing "B". RST is left unbridged: this driver never
// issues a hardware reset, and the RFM69's RST line has an internal pulldown.
#define SOMFY_RFM69_CS_PIN    33
#define SOMFY_RFM69_DIO2_PIN  27

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
    void setHighPower(bool onOFF=true); //have to call it after initialize for RFM69HW

    SomfyRTS(byte pinTx, unsigned char transmitterType) {

      _pinTx = pinTx;
      _EEPROM_address = 0;
      _RTS_address = 0x121300;
      _actionCommand = STOP;
      _virtualRemoteNumber = 0;
      _transmitterType = transmitterType;

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
    std::map<unsigned char, esphome::ESPPreferenceObject> _rollingCodePrefs;

};

#endif
