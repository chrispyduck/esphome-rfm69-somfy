#include "rfm69_direct.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rfm69_direct {

static const char *const TAG = "rfm69_direct";

// isRFM69HW is left false here on purpose, same as the original SomfyRTS.cpp this
// was vendored from: RFM69DirectHub::setup() calls radio.setHighPower(true) right
// after initialize() succeeds, which is what actually engages the PA1/PA2
// high-power stages via RFM69OOK::setHighPower().
RFM69OOK radio(RFM69_DIRECT_CS_PIN, RFM69_DIRECT_DIO2_PIN, false, RFM69_DIRECT_DIO2_PIN);

void RFM69DirectHub::setup() {
  // Puts the RFM69 into standby and leaves modulation/frequency at initialize()'s
  // OOK/433.42MHz defaults - each protocol driver sets what it actually needs
  // (setOokMode()/setFskMode(), setFrequencyMHz()) immediately before it transmits.
  this->radio_ok_ = radio.initialize();
  if (!this->radio_ok_) {
    ESP_LOGE(TAG,
             "RFM69 not responding (RegVersion=0x%02X, expected 0x24) - check the Wing's CS jumper and SPI "
             "wiring. Somfy/Allesin commands will be ignored.",
             radio.version());
    this->mark_failed();
    return;
  }
  radio.transmitBegin();
  // Must be called after initialize() (per the underlying driver's own contract) to
  // engage the RFM69HCW's PA1/PA2 high-power amplifier stages.
  radio.setHighPower(true);
  ESP_LOGI(TAG, "RFM69 detected (RegVersion=0x%02X)", radio.version());
}

void RFM69DirectHub::dump_config() {
  ESP_LOGCONFIG(TAG, "RFM69 direct-modulation radio:");
  ESP_LOGCONFIG(TAG, "  CS pin: GPIO%d", RFM69_DIRECT_CS_PIN);
  ESP_LOGCONFIG(TAG, "  DIO2 (data) pin: GPIO%d", RFM69_DIRECT_DIO2_PIN);
}

}  // namespace rfm69_direct
}  // namespace esphome
