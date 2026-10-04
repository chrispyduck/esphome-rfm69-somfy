#include "allesin_rts_cover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace allesin_rts {

static const char *const TAG = "allesin_rts.cover";

void AllesinRTSCover::setup() {
  this->position = 0.5f;  // unknown - Allesin RTS has no position feedback
}

void AllesinRTSCover::dump_config() { ESP_LOGCONFIG(TAG, "Allesin RTS Cover:"); }

cover::CoverTraits AllesinRTSCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(false);
  traits.set_supports_toggle(false);
  traits.set_supports_stop(true);
  return traits;
}

void AllesinRTSCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    this->hub_->send_command(ALLESIN_CMD_STOP);
    this->publish_state();
    return;
  }
  if (call.get_position().has_value()) {
    float pos = *call.get_position();
    if (pos == cover::COVER_OPEN) {
      this->hub_->send_command(ALLESIN_CMD_UP);
      this->position = cover::COVER_OPEN;
    } else {
      this->hub_->send_command(ALLESIN_CMD_DOWN);
      this->position = cover::COVER_CLOSED;
    }
    this->publish_state();
  }
}

}  // namespace allesin_rts
}  // namespace esphome
