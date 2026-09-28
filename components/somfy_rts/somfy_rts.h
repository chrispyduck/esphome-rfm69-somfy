#pragma once
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

// Owns the single RFM69 radio (see SOMFY_RFM69_CS_PIN / SOMFY_RFM69_DIO2_PIN in
// somfy_rts_lib.h for the wiring) and dispatches Somfy RTS frames on its behalf.
// There is only ever one instance of this in a config: the underlying driver is a
// single process-wide radio object, matching the one physical RFM69 module.
class SomfyRTSHub : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // remote_number distinguishes each virtual remote (one per cover) sharing this
  // radio - it selects both the RTS "address" the shade motor sees and the EEPROM
  // slot its rolling code is stored in. command is one of the SOMFY_CMD_* above.
  void send_command(uint8_t remote_number, uint8_t command);

 protected:
  SomfyRTS *radio_{nullptr};
};

}  // namespace somfy_rts
}  // namespace esphome
