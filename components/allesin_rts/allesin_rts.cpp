#include "allesin_rts.h"
#include "esphome/core/log.h"

namespace esphome {
namespace allesin_rts {

static const char *const TAG = "allesin_rts";

void AllesinRTSHub::setup() {
  // The shared RFM69 (components/rfm69_direct/) finishes its own setup() first
  // (it runs at BUS priority, this component at HARDWARE) so radio.version() is
  // already populated by the time this runs - see AllesinRTS::initRadio().
  this->radio_ = new AllesinRTS();
  if (this->radio_hub_ == nullptr || !this->radio_hub_->radio_ok()) {
    ESP_LOGE(TAG, "Shared RFM69 radio is not available - Allesin commands will be ignored.");
    this->mark_failed();
    return;
  }
}

void AllesinRTSHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Allesin RTS hub:");
  ESP_LOGCONFIG(TAG, "  Radio: shared RFM69 (see rfm69_direct), continuous FSK @ %.3fMHz", ALLESIN_FREQUENCY_MHZ);
  ESP_LOGCONFIG(TAG, "  Replaying one captured remote's RF signal - see this repo's README for what that means");
  ESP_LOGCONFIG(TAG, "  (no known per-shade addressing, no verified real-hardware test by this repo).");
}

void AllesinRTSHub::send_command(AllesinCommand command) {
  if (this->radio_ == nullptr || !this->radio_->radioOk()) {
    ESP_LOGW(TAG, "Ignoring Allesin command 0x%X: RFM69 not initialized", command);
    return;
  }
  ESP_LOGD(TAG, "Sending Allesin RTS signal: command 0x%X", command);
  this->radio_->sendAllesin(command);
}

}  // namespace allesin_rts
}  // namespace esphome
