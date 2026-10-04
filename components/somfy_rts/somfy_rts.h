#pragma once
#include "esphome/components/rfm69_direct/rfm69_direct.h"
#include "esphome/core/component.h"
#include "somfy_rts_lib.h"

namespace esphome {
namespace somfy_rts {

// Scoped aliases for the vendored library's bare UP/DOWN/STOP/PROG macros, so
// nothing outside this component's own files is exposed to those generic names.
static const uint8_t SOMFY_CMD_STOP = STOP;
static const uint8_t SOMFY_CMD_UP = UP;
static const uint8_t SOMFY_CMD_DOWN = DOWN;
static const uint8_t SOMFY_CMD_PROG = PROG;

#undef UP
#undef DOWN
#undef STOP
#undef PROG

// Dispatches Somfy RTS frames through the shared RFM69DirectHub radio (see
// components/rfm69_direct/) on behalf of every somfy_rts cover. There is only ever
// one instance of this in a config: the underlying driver is a single
// process-wide radio object, matching the one physical RFM69 module.
class SomfyRTSHub : public Component {
 public:
  void set_radio_hub(rfm69_direct::RFM69DirectHub *radio_hub) { radio_hub_ = radio_hub; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // remote_number distinguishes each virtual remote (one per cover) sharing this
  // radio - it selects both the RTS "address" the shade motor sees and the EEPROM
  // slot its rolling code is stored in. command is one of the SOMFY_CMD_* above.
  void send_command(uint8_t remote_number, uint8_t command);

 protected:
  rfm69_direct::RFM69DirectHub *radio_hub_{nullptr};
  SomfyRTS *radio_{nullptr};
};

}  // namespace somfy_rts
}  // namespace esphome
