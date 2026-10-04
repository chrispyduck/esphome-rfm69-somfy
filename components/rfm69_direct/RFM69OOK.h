// **********************************************************************************
// Driver definition for HopeRF RFM69W/RFM69HW/RFM69CW/RFM69HCW, Semtech SX1231/1231H
// **********************************************************************************
// Copyright Felix Rusu (2014), felix@lowpowerlab.com
// http://lowpowerlab.com/
// **********************************************************************************
// License: GPL-3.0, see http://www.gnu.org/licenses/gpl-3.0.txt
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
//
// Vendored from https://github.com/etimou/SomfyRTS (kobuki/RFM69OOK lineage) for use
// as an ESPHome external_component. Patched for ESP32 support:
//   - Added an ESP32 branch to the IRQ pin/number #ifdef ladder below (the original
//     ifdef ladder had no ESP32 case at all, so this header failed to compile on
//     ESP32 - RF69OOK_IRQ_PIN/RF69OOK_IRQ_NUM were simply undefined identifiers).
//     The values here are unused defaults: SomfyRTS.cpp always constructs the global
//     `radio` object with explicit pins (see SOMFY_RFM69_CS_PIN/SOMFY_RFM69_DIO2_PIN
//     in SomfyRTS.h), but the default-argument expressions in the constructor below
//     are still part of the class declaration and must resolve to *something* at
//     compile time.
//   - Moved out of components/somfy_rts/ into this standalone components/rfm69_direct/
//     component, and given setOokMode()/setFskMode() (despite the class's now-dated
//     "OOK" name): both Somfy RTS (continuous-OOK) and Allesin RTS (continuous-FSK,
//     see components/allesin_rts/) direct-modulate the single physical RFM69 through
//     this one driver instance - DIO2 is just a data line either way, and which tone
//     a given digitalWrite(HIGH/LOW) produces on air depends only on REG_DATAMODUL's
//     modulation-type bit, which these two methods flip. Callers re-apply the mode
//     they need immediately before transmitting rather than relying on whatever the
//     other protocol left it in.
#ifndef RFM69OOK_h
#define RFM69OOK_h
#include <Arduino.h>            //assumes Arduino IDE v1.0 or greater

#define RF69OOK_SPI_CS  SS // SS is the SPI slave select pin, for instance D10 on atmega328

// INT0 on AVRs should be connected to RFM69's DIO0 (ex on Atmega328 it's D2, on Atmega644/1284 it's D2)
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega88) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
  #define RF69OOK_IRQ_PIN          3
  #define RF69OOK_IRQ_NUM          1
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
  #define RF69OOK_IRQ_PIN          10
  #define RF69OOK_IRQ_NUM          0
#elif defined(__AVR_ATmega32U4__)
  #define RF69OOK_IRQ_PIN          3
  #define RF69OOK_IRQ_NUM          0
#elif defined(ESP8266)
  #define RF69OOK_IRQ_PIN          D1
  #define RF69OOK_IRQ_NUM          0
#elif defined(ESP32)
  // Unused fallback (see comment at top of file) - Arduino-ESP32's attachInterrupt()
  // takes a GPIO number directly, so pin and "interrupt number" are the same value.
  #define RF69OOK_IRQ_PIN          27
  #define RF69OOK_IRQ_NUM          27
#endif

// SPI bus pins for ESP32. Arduino-ESP32's SPI.begin() with no arguments uses the
// generic-variant defaults (SCK=18, MISO=19, MOSI=23), which don't match any
// Feather - and on the Adafruit ESP32 Feather V2 calling it that way hangs
// forever, so these are always passed explicitly. Values are the Feather V2's
// stacked-header SPI pins (SCK=5, MISO=21, MOSI=19); override by defining them
// before this header is included.
#if defined(ESP32)
  #ifndef RF69OOK_SPI_SCK
    #define RF69OOK_SPI_SCK   5
    #define RF69OOK_SPI_MISO  21
    #define RF69OOK_SPI_MOSI  19
  #endif
#endif

#define RF69OOK_MODE_SLEEP       0 // XTAL OFF
#define RF69OOK_MODE_STANDBY     1 // XTAL ON
#define RF69OOK_MODE_SYNTH       2 // PLL ON
#define RF69OOK_MODE_RX          3 // RX MODE
#define RF69OOK_MODE_TX          4 // TX MODE

#define null                  0
#define COURSE_TEMP_COEF    -90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#define RF69OOK_FSTEP 61.03515625 // == FXOSC/2^19 = 32mhz/2^19 (p13 in DS)

class RFM69OOK {
  public:
    static volatile int RSSI; //most accurate RSSI during reception (closest to the reception)
    static volatile byte _mode; //should be protected?

    RFM69OOK(byte slaveSelectPin=RF69OOK_SPI_CS, byte interruptPin=RF69OOK_IRQ_PIN, bool isRFM69HW=false, byte interruptNum=RF69OOK_IRQ_NUM) {
      _slaveSelectPin = slaveSelectPin;
      _interruptPin = interruptPin;
      _interruptNum = interruptNum;
      _mode = RF69OOK_MODE_STANDBY;
      _powerLevel = 31;
      _isRFM69HW = isRFM69HW;
      _version = 0;
    }

    // Returns false (without hanging) if the radio doesn't answer on SPI.
    bool initialize();
    byte version() const { return _version; }  // RegVersion read during initialize(); 0x24 on a healthy RFM69
    uint32_t getFrequency();
    void setFrequency(uint32_t freqHz);
    void setFrequencyMHz(float f);
    void setCS(byte newSPISlaveSelect);
    int8_t readRSSI(bool forceTrigger=false);
    void setHighPower(bool onOFF=true); //have to call it after initialize for RFM69HW
    void setPowerLevel(byte level); //reduce/increase transmit power level
    void sleep();
    byte readTemperature(byte calFactor=0); //get CMOS temperature (8bit)
    void rcCalibration(); //calibrate the internal RC oscillator for use in wide temperature variations - see datasheet section [4.3.5. RC Timer Accuracy]

    // allow hacking registers by making these public
    byte readReg(byte addr);
    void writeReg(byte addr, byte val);
    void readAllRegs();

    // functions related to OOK mode
    void receiveBegin();
    void receiveEnd();
    void transmitBegin();
    void transmitEnd();
    bool poll();
    void send(bool signal);
    void attachUserInterrupt(void (*function)());
	void setBandwidth(uint8_t bw);
    void setBitrate(uint32_t bitrate);
	void setRSSIThreshold(int8_t rssi);
	void setFixedThreshold(uint8_t threshold);
	void setSensitivityBoost(uint8_t value);

    // Switches REG_DATAMODUL between continuous OOK and continuous FSK direct
    // modulation, with no bit-sync, no shaping - DIO2 stays a plain data line in
    // both cases (see the file header comment above). Safe to call right before
    // every transmission: each is just a couple of SPI register writes.
    void setOokMode();
    void setFskMode(uint32_t deviationHz);

    void select();
    void unselect();

  protected:
    static void isr0();
    void virtual interruptHandler();

    static RFM69OOK* selfPointer;
    byte _slaveSelectPin;
    byte _interruptPin;
    byte _interruptNum;
    byte _powerLevel;
    bool _isRFM69HW;
    byte _version;
    byte _SPCR;
    byte _SPSR;

    void setMode(byte mode);
    void setHighPowerRegs(bool onOff);

    // functions related to OOK mode
    void (*userInterrupt)();
    void ookInterruptHandler();
};

#endif
