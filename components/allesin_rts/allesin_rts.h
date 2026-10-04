#pragma once
#include "esphome/components/rfm69_direct/rfm69_direct.h"
#include "esphome/core/component.h"
#include "allesin_rts_lib.h"

namespace esphome {
namespace allesin_rts {

// Dispatches Allesin RTS commands through the shared RFM69DirectHub radio (see
// components/rfm69_direct/) on behalf of every allesin_rts cover. There is only
// ever one instance of this in a config: the underlying driver is a single
// process-wide radio object, matching the one physical RFM69 module - the same one
// components/somfy_rts/ uses.
class AllesinRTSHub : public Component {
 public:
  void set_radio_hub(rfm69_direct::RFM69DirectHub *radio_hub) { radio_hub_ = radio_hub; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void send_command(AllesinCommand command);

 protected:
  rfm69_direct::RFM69DirectHub *radio_hub_{nullptr};
  AllesinRTS *radio_{nullptr};
};

}  // namespace allesin_rts
}  // namespace esphome
