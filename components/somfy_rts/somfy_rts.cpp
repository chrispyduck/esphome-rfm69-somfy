#include "somfy_rts.h"
#include "esphome/core/log.h"

namespace esphome {
namespace somfy_rts {

static const char *const TAG = "somfy_rts";

void SomfyRTSHub::setup() {
  // The shared RFM69 (components/rfm69_direct/) finishes its own setup() first
  // (it runs at BUS priority, this component at HARDWARE) so radio.version() is
  // already populated by the time this runs - see SomfyRTS::initRadio().
  this->radio_ = new SomfyRTS(rfm69_direct::RFM69_DIRECT_DIO2_PIN, TSR_RFM69);
  if (this->radio_hub_ == nullptr || !this->radio_hub_->radio_ok()) {
    ESP_LOGE(TAG, "Shared RFM69 radio is not available - Somfy commands will be ignored.");
    this->mark_failed();
    return;
  }
}

void SomfyRTSHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy RTS hub:");
  ESP_LOGCONFIG(TAG, "  Radio: shared RFM69 (see rfm69_direct), continuous OOK @ 433.42MHz");
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
