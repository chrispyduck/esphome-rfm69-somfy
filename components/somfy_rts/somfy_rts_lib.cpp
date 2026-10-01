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
//   - The global `radio` object below now takes explicit CS/DIO2 pins (see
//     SOMFY_RFM69_CS_PIN/SOMFY_RFM69_DIO2_PIN in somfy_rts_lib.h) instead of relying
//     on RFM69OOK's per-MCU default arguments, which had no ESP32 case.
//   - EEPROM.commit() is required on ESP32 too, not just ESP8266, or the rolling
//     code never actually reaches flash.
//   - Renamed from SomfyRTS.h/.cpp to somfy_rts_lib.h/.cpp to avoid a filename
//     collision with somfy_rts.h/.cpp (this component's own wrapper) on
//     case-insensitive filesystems (macOS, some Home Assistant setups).
//   - TRANSMIT_HIGH/LOW use digitalWrite() on ESP32 instead of the ESP8266-only
//     GPOS/GPOC fast register macros, which don't exist on ESP32's Arduino core.
//   - sendSomfy() used to wrap all three outgoing frames in one noInterrupts()/
//     interrupts() pair (~500ms total). ESP-IDF's interrupt watchdog resets the
//     chip (TG1WDT_SYS_RESET) if interrupts stay disabled past ~300ms, so each
//     frame now gets its own shorter pair instead - see the comment in
//     sendSomfy() below.

#include "somfy_rts_lib.h"
#include <EEPROM.h>
#include <SPI.h>
#include "RFM69OOK.h"
#include "RFM69OOKregisters.h"

#if defined(ESP8266)
  #define TRANSMIT_HIGH(pin) (GPOS = 1<<pin)
  #define TRANSMIT_LOW(pin) (GPOC = 1<<pin)
#elif defined(ESP32)
  // GPOS/GPOC (ESP8266's fast direct-register set/clear macros) don't exist on the
  // ESP32 Arduino core. The original author's own comment above already flagged
  // digitalWrite() as the portable fallback; its overhead (tens of ns) is
  // negligible against Somfy RTS's own timing (640us symbols), so there's no need
  // for direct register access here.
  #define TRANSMIT_HIGH(pin) (digitalWrite(pin, HIGH))
  #define TRANSMIT_LOW(pin) (digitalWrite(pin, LOW))
#else
  #define TRANSMIT_HIGH(pin) (PORTD |= 1<<pin)
  #define TRANSMIT_LOW(pin) (PORTD &= !(1<<pin))
#endif


// isRFM69HW is left false here on purpose: SomfyRTS::setHighPower() (called by the
// ESPHome hub right after construction, matching this library's documented usage)
// is what actually engages the PA1/PA2 high-power stages via RFM69OOK::setHighPower().
RFM69OOK radio(SOMFY_RFM69_CS_PIN, SOMFY_RFM69_DIO2_PIN, false, SOMFY_RFM69_DIO2_PIN);

void SomfyRTS::initRadio() {
  pinMode(_pinTx, OUTPUT);

  if (_transmitterType == TSR_RFM69)
  {

    radio.initialize();
    radio.transmitBegin();
    //radio.setFrequencyMHz(868.88);
    radio.setFrequencyMHz(433.42);
    radio.setPowerLevel(20);
  }
}

void SomfyRTS::configRTS(unsigned int EEPROM_address, unsigned long RTS_address) {
  _EEPROM_address = EEPROM_address;
  _RTS_address = RTS_address;
}

void SomfyRTS::setHighPower(bool onOFF){ //have to call it after initialize for RFM69HW
  radio.setHighPower(onOFF);
}

void SomfyRTS::buildFrameSomfy() {
  uint16_t Code;
  EEPROM.get(_EEPROM_address + 2 * _virtualRemoteNumber, Code);
  frame[0] = 0xA7; // Encryption key. Doesn't matter much
  frame[1] = _actionCommand << 4;  // Which button did  you press? The 4 LSB will be the checksum
  frame[2] = Code >> 8;    // Rolling code (big endian)
  frame[3] = Code;         // Rolling code
  frame[4] = _RTS_address + _virtualRemoteNumber >> 16; // Remote address
  frame[5] = _RTS_address + _virtualRemoteNumber >>  8; // Remote address
  frame[6] = _RTS_address + _virtualRemoteNumber;     // Remote address

  //Serial.print("Frame         : ");
  for (byte i = 0; i < 7; i++) {
    if (frame[i] >> 4 == 0) { //  Displays leading zero in case the most significant
      //Serial.print("0");     // nibble is a 0.
    }
    //Serial.print(frame[i],HEX); Serial.print(" ");
  }

  // Checksum calculation: a XOR of all the nibbles
  byte checksum = 0;
  for (byte i = 0; i < 7; i++) {
    checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
  }
  checksum &= 0b1111; // We keep the last 4 bits only


  //Checksum integration
  frame[1] |= checksum; //  If a XOR of all the nibbles is equal to 0, the blinds will
  // consider the checksum ok.

  //Serial.println(""); Serial.print("With checksum : ");
  for (byte i = 0; i < 7; i++) {
    if (frame[i] >> 4 == 0) {
      //Serial.print("0");
    }
    //Serial.print(frame[i],HEX); Serial.print(" ");
  }


  // Obfuscation: a XOR of all the bytes
  for (byte i = 1; i < 7; i++) {
    frame[i] ^= frame[i - 1];
  }

  //Serial.println(""); Serial.print("Obfuscated    : ");
  for (byte i = 0; i < 7; i++) {
    if (frame[i] >> 4 == 0) {
      //Serial.print("0");
    }
    //Serial.print(frame[i],HEX); Serial.print(" ");
  }
  //Serial.println("");
  //Serial.print("Rolling Code  : "); Serial.println(code);
  EEPROM.put(_EEPROM_address + 2 * _virtualRemoteNumber, ++Code); //  We store the value of the rolling code in the
  // EEPROM. It should take up to 2 adresses but the
  // Arduino function takes care of it.
  #if defined(ESP8266) || defined(ESP32)
  EEPROM.commit();
  #endif
}

void SomfyRTS::sendCommandSomfy(byte sync) {
  if (sync == 2) { // Only with the first frame.
    //Wake-up pulse & Silence
    TRANSMIT_HIGH(_pinTx);
    delayMicroseconds(9415);
    TRANSMIT_LOW(_pinTx);
    //delayMicroseconds(89565U);
    delay(89);
  }

  // Hardware sync: two sync for the first frame, seven for the following ones.
  for (int i = 0; i < sync; i++) {
    TRANSMIT_HIGH(_pinTx);
    delayMicroseconds(4 * SYMBOL);
    TRANSMIT_LOW(_pinTx);
    delayMicroseconds(4 * SYMBOL);
  }

  // Software sync
  TRANSMIT_HIGH(_pinTx);
  delayMicroseconds(4550);
  TRANSMIT_LOW(_pinTx);
  delayMicroseconds(SYMBOL);


  //Data: bits are sent one by one, starting with the MSB.
  for (byte i = 0; i < 56; i++) {
    if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
      TRANSMIT_LOW(_pinTx);
      delayMicroseconds(SYMBOL);
      TRANSMIT_HIGH(_pinTx);
      delayMicroseconds(SYMBOL);
    }
    else {
      TRANSMIT_HIGH(_pinTx);
      delayMicroseconds(SYMBOL);
      TRANSMIT_LOW(_pinTx);
      delayMicroseconds(SYMBOL);
    }
  }

  TRANSMIT_LOW(_pinTx);
  delayMicroseconds(30415); // Inter-frame silence
}

void SomfyRTS::sendSomfy(unsigned char virtualRemoteNumber, unsigned char actionCommand) {
  _virtualRemoteNumber = virtualRemoteNumber;
  _actionCommand = actionCommand;

  buildFrameSomfy();

  // Each sendCommandSomfy() call is wrapped in its own noInterrupts()/interrupts()
  // pair (rather than one pair around all three calls) so every individual
  // no-interrupts window stays under ESP-IDF's default 300ms interrupt-watchdog
  // threshold (TG1WDT_SYS_RESET) - the first frame alone (with its 89ms wake
  // pulse) runs ~216ms, and each following frame ~143ms, but back to back with
  // interrupts left disabled throughout they sum to ~500ms and reliably crash the
  // ESP32. The brief re-enable between calls falls inside the protocol's own
  // mandatory ~30ms inter-frame silence, so it costs nothing timing-wise.
  noInterrupts();
  sendCommandSomfy(2);
  interrupts();
  for (int i = 0; i < 2; i++) {
    noInterrupts();
    sendCommandSomfy(7);
    interrupts();
  }
}
