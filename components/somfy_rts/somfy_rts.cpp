#include "somfy_rts.h"
#include "esphome/core/log.h"

namespace esphome {
namespace somfy_rts {

static const char *const TAG = "somfy_rts";

void SomfyRTSHub::setup() {
  // Constructing SomfyRTS runs the RFM69 SPI init (see vendor/SomfyRTS.cpp
  // initRadio()): standby, continuous-OOK mode, 433.42MHz, base power level.
  this->radio_ = new SomfyRTS(SOMFY_RFM69_DIO2_PIN, TSR_RFM69);
  if (!this->radio_->radioOk()) {
    ESP_LOGE(TAG,
             "RFM69 not responding (RegVersion=0x%02X, expected 0x24) - check the Wing's CS jumper and SPI "
             "wiring. Somfy commands will be ignored.",
             this->radio_->radioVersion());
    this->mark_failed();
    return;
  }
  // Must be called after construction (per the library's own contract) to engage
  // the RFM69HCW's PA1/PA2 high-power amplifier stages.
  this->radio_->setHighPower(true);
  ESP_LOGI(TAG, "RFM69 detected (RegVersion=0x%02X)", this->radio_->radioVersion());
}

void SomfyRTSHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy RTS hub:");
  ESP_LOGCONFIG(TAG, "  Radio: RFM69HCW, direct/continuous OOK @ 433.42MHz");
  ESP_LOGCONFIG(TAG, "  CS pin: GPIO%d", SOMFY_RFM69_CS_PIN);
  ESP_LOGCONFIG(TAG, "  DIO2 (data) pin: GPIO%d", SOMFY_RFM69_DIO2_PIN);
}

void SomfyRTSHub::send_command(uint8_t remote_number, uint8_t command) {
  if (this->radio_ == nullptr || !this->radio_->radioOk()) {
    ESP_LOGW(TAG, "Ignoring command for remote %u: RFM69 not initialized", remote_number);
    return;
  }
  ESP_LOGD(TAG, "Sending Somfy RTS frame: remote %u, command 0x%X", remote_number, command);
  this->radio_->sendSomfy(remote_number, command);
}

}  // namespace somfy_rts
}  // namespace esphome
