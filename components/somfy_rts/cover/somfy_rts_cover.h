#pragma once
#include "esphome/components/cover/cover.h"
#include "../somfy_rts.h"

namespace esphome {
namespace somfy_rts {

// Somfy RTS shade motors give no position or state feedback at all - this cover is
// deliberately an "assumed state" cover (open/close/stop buttons, no slider) rather
// than pretending to track a position it has no way to actually verify.
class SomfyRTSCover : public cover::Cover, public Component {
 public:
  void set_hub(SomfyRTSHub *hub) { hub_ = hub; }
  void set_remote_number(uint8_t remote_number) { remote_number_ = remote_number; }

  void setup() override;
  void dump_config() override;
  cover::CoverTraits get_traits() override;

 protected:
  void control(const cover::CoverCall &call) override;

  SomfyRTSHub *hub_{nullptr};
  uint8_t remote_number_{0};
};

}  // namespace somfy_rts
}  // namespace esphome
