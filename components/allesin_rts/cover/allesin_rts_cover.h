#pragma once
#include "esphome/components/cover/cover.h"
#include "../allesin_rts.h"

namespace esphome {
namespace allesin_rts {

// Allesin's motor, like Somfy's, gives no position or state feedback - this cover is
// deliberately an "assumed state" cover (open/close/stop buttons, no slider) rather
// than pretending to track a position it has no way to actually verify.
class AllesinRTSCover : public cover::Cover, public Component {
 public:
  void set_hub(AllesinRTSHub *hub) { hub_ = hub; }

  void setup() override;
  void dump_config() override;
  cover::CoverTraits get_traits() override;

 protected:
  void control(const cover::CoverCall &call) override;

  AllesinRTSHub *hub_{nullptr};
};

}  // namespace allesin_rts
}  // namespace esphome
