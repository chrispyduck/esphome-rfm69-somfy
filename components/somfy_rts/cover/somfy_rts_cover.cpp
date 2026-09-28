#include "somfy_rts_cover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace somfy_rts {

static const char *const TAG = "somfy_rts.cover";

void SomfyRTSCover::setup() {
  this->position = 0.5f;  // unknown - Somfy RTS has no position feedback
}

void SomfyRTSCover::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy RTS Cover:");
  ESP_LOGCONFIG(TAG, "  Remote number: %u", this->remote_number_);
}

cover::CoverTraits SomfyRTSCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(false);
  traits.set_supports_toggle(false);
  traits.set_supports_stop(true);
  return traits;
}

void SomfyRTSCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    this->hub_->send_command(this->remote_number_, SOMFY_CMD_STOP);
    this->publish_state();
    return;
  }
  if (call.get_position().has_value()) {
    float pos = *call.get_position();
    if (pos == cover::COVER_OPEN) {
      this->hub_->send_command(this->remote_number_, SOMFY_CMD_UP);
      this->position = cover::COVER_OPEN;
    } else {
      this->hub_->send_command(this->remote_number_, SOMFY_CMD_DOWN);
      this->position = cover::COVER_CLOSED;
    }
    this->publish_state();
  }
}

}  // namespace somfy_rts
}  // namespace esphome
