import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@chris"]

somfy_rts_ns = cg.esphome_ns.namespace("somfy_rts")
SomfyRTSHub = somfy_rts_ns.class_("SomfyRTSHub", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SomfyRTSHub),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # ESPHome's ESP32/Arduino build disables most Arduino-bundled libraries by
    # default to save flash; cg.add_library() is the supported way to re-enable
    # one (see esphome/components/fastled_base and .../ota for other examples).
    # The vendored Somfy RTS driver needs Arduino's SPI and EEPROM directly.
    cg.add_library("SPI", None)
    cg.add_library("EEPROM", None)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
